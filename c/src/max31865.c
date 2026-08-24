/*
 * Implementation basis: Analog Devices (2015), MAX31865 data sheet; see
 * docs/REFERENCES.md.
 */
#include "rtd_acquire/max31865.h"

#include <math.h>
#include <stdint.h>

#define RTD_ACQUIRE_MAX31865_MIN_REFERENCE_OHMS 350.0F
#define RTD_ACQUIRE_MAX31865_MAX_REFERENCE_OHMS 10000.0F
#define RTD_ACQUIRE_MAX31865_ADC_SCALE 32768.0F
#define RTD_ACQUIRE_MAX31865_MAX_ADC_CODE 32767U
#define RTD_ACQUIRE_MAX31865_CONFIG_THREE_WIRE 0x10U
#define RTD_ACQUIRE_MAX31865_CONFIG_FILTER_50HZ 0x01U

typedef struct {
    uint8_t mask;
    rtd_acquire_diagnostic_code_t code;
    rtd_acquire_diagnostic_severity_t severity;
    const char *identifier;
    const char *message;
} rtd_acquire_max31865_fault_mapping_t;

static const rtd_acquire_max31865_fault_mapping_t
    RTD_ACQUIRE_MAX31865_FAULT_MAPPINGS[] = {
        {
            0x80U,
            RTD_ACQUIRE_DIAGNOSTIC_CODE_RESISTANCE_HIGH_THRESHOLD,
            RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_WARNING,
            "D7",
            "RTD High Threshold",
        },
        {
            0x40U,
            RTD_ACQUIRE_DIAGNOSTIC_CODE_RESISTANCE_LOW_THRESHOLD,
            RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_WARNING,
            "D6",
            "RTD Low Threshold",
        },
        {
            0x20U,
            RTD_ACQUIRE_DIAGNOSTIC_CODE_REFERENCE_INPUT_ABOVE_THRESHOLD,
            RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT,
            "D5",
            "REFIN- > 0.85 x VBIAS",
        },
        {
            0x10U,
            RTD_ACQUIRE_DIAGNOSTIC_CODE_REFERENCE_INPUT_BELOW_THRESHOLD,
            RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT,
            "D4",
            "REFIN- < 0.85 x VBIAS (FORCE- open)",
        },
        {
            0x08U,
            RTD_ACQUIRE_DIAGNOSTIC_CODE_RTD_INPUT_BELOW_THRESHOLD,
            RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT,
            "D3",
            "RTDIN- < 0.85 x VBIAS (FORCE- open)",
        },
        {
            0x04U,
            RTD_ACQUIRE_DIAGNOSTIC_CODE_INPUT_VOLTAGE_FAULT,
            RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT,
            "D2",
            "Overvoltage/undervoltage fault",
        },
    };

_Static_assert(
    sizeof(RTD_ACQUIRE_MAX31865_FAULT_MAPPINGS)
        / sizeof(RTD_ACQUIRE_MAX31865_FAULT_MAPPINGS[0])
        == RTD_ACQUIRE_MAX31865_MAX_DIAGNOSTICS,
    "MAX31865 diagnostic capacity must match native fault mappings"
);
_Static_assert(
    RTD_ACQUIRE_MAX31865_MAX_NATIVE_EVIDENCE
        == RTD_ACQUIRE_MAX31865_MAX_DIAGNOSTICS,
    "MAX31865 native evidence capacity must match diagnostic capacity"
);

static bool rtd_acquire_max31865_measurement_storage_is_usable(
    const rtd_acquire_measurement_t *measurement,
    size_t required_diagnostics,
    size_t required_native_evidence
)
{
    if (measurement == NULL) {
        return false;
    }
    if (measurement->diagnostic_capacity > 0U
        && measurement->diagnostics == NULL) {
        return false;
    }
    if (measurement->native_evidence_capacity > 0U
        && measurement->native_evidence == NULL) {
        return false;
    }

    return measurement->diagnostic_capacity >= required_diagnostics
        && measurement->native_evidence_capacity >= required_native_evidence;
}

static bool rtd_acquire_max31865_threshold_is_valid(
    rtd_acquire_real_t threshold_ohms,
    rtd_acquire_real_t reference_resistance_ohms,
    bool allow_zero
)
{
    if (!isfinite(threshold_ohms)) {
        return false;
    }
    if (allow_zero) {
        if (threshold_ohms < 0.0F) {
            return false;
        }
    } else if (threshold_ohms <= 0.0F) {
        return false;
    }

    return threshold_ohms < reference_resistance_ohms;
}

static bool rtd_acquire_max31865_encode_high_threshold(
    const rtd_acquire_max31865_config_t *config,
    uint16_t *encoded
)
{
    rtd_acquire_real_t scaled;
    uint32_t code;

    if (!config->has_high_fault_threshold) {
        *encoded = UINT16_MAX;
        return true;
    }

    scaled = config->high_fault_threshold_ohms
        / config->reference_resistance_ohms
        * RTD_ACQUIRE_MAX31865_ADC_SCALE;
    code = (uint32_t)scaled;
    if ((rtd_acquire_real_t)code < scaled) {
        ++code;
    }
    if (code > RTD_ACQUIRE_MAX31865_MAX_ADC_CODE) {
        return false;
    }

    *encoded = (uint16_t)(code << 1U);
    return true;
}

static void rtd_acquire_max31865_encode_low_threshold(
    const rtd_acquire_max31865_config_t *config,
    uint16_t *encoded
)
{
    rtd_acquire_real_t scaled;
    uint32_t code;

    if (!config->has_low_fault_threshold) {
        *encoded = 0U;
        return;
    }

    scaled = config->low_fault_threshold_ohms
        / config->reference_resistance_ohms
        * RTD_ACQUIRE_MAX31865_ADC_SCALE;
    code = (uint32_t)scaled;
    *encoded = (uint16_t)(code << 1U);
}

bool rtd_acquire_max31865_config_is_valid(
    const rtd_acquire_max31865_config_t *config
)
{
    uint16_t ignored_high;

    if (config == NULL) {
        return false;
    }
    if (!isfinite(config->reference_resistance_ohms)
        || config->reference_resistance_ohms
            < RTD_ACQUIRE_MAX31865_MIN_REFERENCE_OHMS
        || config->reference_resistance_ohms
            > RTD_ACQUIRE_MAX31865_MAX_REFERENCE_OHMS) {
        return false;
    }
    if (config->wire_count != 2U
        && config->wire_count != 3U
        && config->wire_count != 4U) {
        return false;
    }
    if (config->filter_frequency_hz != 50U
        && config->filter_frequency_hz != 60U) {
        return false;
    }
    if (config->has_low_fault_threshold
        && !rtd_acquire_max31865_threshold_is_valid(
            config->low_fault_threshold_ohms,
            config->reference_resistance_ohms,
            true
        )) {
        return false;
    }
    if (config->has_high_fault_threshold
        && !rtd_acquire_max31865_threshold_is_valid(
            config->high_fault_threshold_ohms,
            config->reference_resistance_ohms,
            false
        )) {
        return false;
    }
    if (config->has_low_fault_threshold
        && config->has_high_fault_threshold
        && config->low_fault_threshold_ohms
            >= config->high_fault_threshold_ohms) {
        return false;
    }

    return rtd_acquire_max31865_encode_high_threshold(
        config,
        &ignored_high
    );
}

bool rtd_acquire_max31865_base_config_byte(
    const rtd_acquire_max31865_config_t *config,
    uint8_t *value
)
{
    uint8_t result = 0U;

    if (value == NULL || !rtd_acquire_max31865_config_is_valid(config)) {
        return false;
    }

    if (config->wire_count == 3U) {
        result |= RTD_ACQUIRE_MAX31865_CONFIG_THREE_WIRE;
    }
    if (config->filter_frequency_hz == 50U) {
        result |= RTD_ACQUIRE_MAX31865_CONFIG_FILTER_50HZ;
    }

    *value = result;
    return true;
}

bool rtd_acquire_max31865_encode_threshold_registers(
    const rtd_acquire_max31865_config_t *config,
    uint16_t *high_threshold_register,
    uint16_t *low_threshold_register
)
{
    uint16_t high;
    uint16_t low;

    if (high_threshold_register == NULL || low_threshold_register == NULL) {
        return false;
    }
    if (!rtd_acquire_max31865_config_is_valid(config)) {
        return false;
    }
    if (!rtd_acquire_max31865_encode_high_threshold(config, &high)) {
        return false;
    }
    rtd_acquire_max31865_encode_low_threshold(config, &low);

    *high_threshold_register = high;
    *low_threshold_register = low;
    return true;
}

bool rtd_acquire_max31865_measurement_from_registers(
    const rtd_acquire_max31865_config_t *config,
    uint16_t rtd_register,
    uint8_t fault_status_register,
    rtd_acquire_measurement_t *measurement
)
{
    size_t mapping_index;
    size_t diagnostic_count = 0U;
    bool has_fault = false;

    if (!rtd_acquire_max31865_config_is_valid(config)) {
        return false;
    }

    for (mapping_index = 0U;
         mapping_index < sizeof(RTD_ACQUIRE_MAX31865_FAULT_MAPPINGS)
            / sizeof(RTD_ACQUIRE_MAX31865_FAULT_MAPPINGS[0]);
         ++mapping_index) {
        if ((fault_status_register
             & RTD_ACQUIRE_MAX31865_FAULT_MAPPINGS[mapping_index].mask)
            != 0U) {
            ++diagnostic_count;
        }
    }

    if (!rtd_acquire_max31865_measurement_storage_is_usable(
        measurement,
        diagnostic_count,
        diagnostic_count
    )) {
        return false;
    }

    rtd_acquire_measurement_reset(measurement);

    for (mapping_index = 0U;
         mapping_index < sizeof(RTD_ACQUIRE_MAX31865_FAULT_MAPPINGS)
            / sizeof(RTD_ACQUIRE_MAX31865_FAULT_MAPPINGS[0]);
         ++mapping_index) {
        const rtd_acquire_max31865_fault_mapping_t *mapping =
            &RTD_ACQUIRE_MAX31865_FAULT_MAPPINGS[mapping_index];
        rtd_acquire_diagnostic_t *diagnostic;
        rtd_acquire_native_evidence_t *evidence;

        if ((fault_status_register & mapping->mask) == 0U) {
            continue;
        }

        diagnostic = &measurement->diagnostics[measurement->diagnostic_count];
        evidence = &measurement->native_evidence[
            measurement->native_evidence_count
        ];

        diagnostic->code = mapping->code;
        diagnostic->severity = mapping->severity;
        diagnostic->native_evidence_offset = measurement->native_evidence_count;
        diagnostic->native_evidence_count = 1U;

        evidence->identifier = mapping->identifier;
        evidence->message = mapping->message;

        ++measurement->diagnostic_count;
        ++measurement->native_evidence_count;
        if (mapping->severity == RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT) {
            has_fault = true;
        }
    }

    if (!has_fault) {
        const uint16_t adc_code = (uint16_t)(rtd_register >> 1U);

        measurement->has_resistance = true;
        measurement->resistance_ohms =
            (rtd_acquire_real_t)adc_code
            / RTD_ACQUIRE_MAX31865_ADC_SCALE
            * config->reference_resistance_ohms;
    }

    return rtd_acquire_measurement_is_valid(measurement);
}
