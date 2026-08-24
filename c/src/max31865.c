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
#define RTD_ACQUIRE_MAX31865_MAX_SPI_CLOCK_HZ 5000000U
#define RTD_ACQUIRE_MAX31865_CONFIG_WRITE 0x80U
#define RTD_ACQUIRE_MAX31865_RTD_READ 0x01U
#define RTD_ACQUIRE_MAX31865_HIGH_THRESHOLD_WRITE 0x83U
#define RTD_ACQUIRE_MAX31865_FAULT_STATUS_READ 0x07U
#define RTD_ACQUIRE_MAX31865_CONFIG_BIAS 0x80U
#define RTD_ACQUIRE_MAX31865_CONFIG_ONE_SHOT 0x20U
#define RTD_ACQUIRE_MAX31865_CONFIG_AUTO_FAULT_CYCLE 0x04U
#define RTD_ACQUIRE_MAX31865_CONFIG_CLEAR_FAULTS 0x02U
#define RTD_ACQUIRE_MAX31865_FAULT_CYCLE_MAX_US 600U
#define RTD_ACQUIRE_MAX31865_CONVERSION_50HZ_US 66000U
#define RTD_ACQUIRE_MAX31865_CONVERSION_60HZ_US 55000U

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

static rtd_acquire_max31865_result_t
rtd_acquire_max31865_measurement_storage_result(
    const rtd_acquire_measurement_t *measurement,
    size_t required_diagnostics,
    size_t required_native_evidence
)
{
    if (measurement == NULL) {
        return RTD_ACQUIRE_MAX31865_RESULT_INVALID_ARGUMENT;
    }
    if (measurement->diagnostic_capacity > 0U
        && measurement->diagnostics == NULL) {
        return RTD_ACQUIRE_MAX31865_RESULT_INVALID_ARGUMENT;
    }
    if (measurement->native_evidence_capacity > 0U
        && measurement->native_evidence == NULL) {
        return RTD_ACQUIRE_MAX31865_RESULT_INVALID_ARGUMENT;
    }
    if (measurement->diagnostic_capacity < required_diagnostics
        || measurement->native_evidence_capacity < required_native_evidence) {
        return RTD_ACQUIRE_MAX31865_RESULT_INSUFFICIENT_STORAGE;
    }

    return RTD_ACQUIRE_MAX31865_RESULT_OK;
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

rtd_acquire_max31865_result_t rtd_acquire_max31865_base_config_byte(
    const rtd_acquire_max31865_config_t *config,
    uint8_t *value
)
{
    uint8_t result = 0U;

    if (config == NULL || value == NULL) {
        return RTD_ACQUIRE_MAX31865_RESULT_INVALID_ARGUMENT;
    }
    if (!rtd_acquire_max31865_config_is_valid(config)) {
        return RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR;
    }

    if (config->wire_count == 3U) {
        result |= RTD_ACQUIRE_MAX31865_CONFIG_THREE_WIRE;
    }
    if (config->filter_frequency_hz == 50U) {
        result |= RTD_ACQUIRE_MAX31865_CONFIG_FILTER_50HZ;
    }

    *value = result;
    return RTD_ACQUIRE_MAX31865_RESULT_OK;
}

rtd_acquire_max31865_result_t rtd_acquire_max31865_encode_threshold_registers(
    const rtd_acquire_max31865_config_t *config,
    uint16_t *high_threshold_register,
    uint16_t *low_threshold_register
)
{
    uint16_t high;
    uint16_t low;

    if (config == NULL || high_threshold_register == NULL
        || low_threshold_register == NULL) {
        return RTD_ACQUIRE_MAX31865_RESULT_INVALID_ARGUMENT;
    }
    if (!rtd_acquire_max31865_config_is_valid(config)) {
        return RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR;
    }
    if (!rtd_acquire_max31865_encode_high_threshold(config, &high)) {
        return RTD_ACQUIRE_MAX31865_RESULT_INTERNAL_ERROR;
    }
    rtd_acquire_max31865_encode_low_threshold(config, &low);

    *high_threshold_register = high;
    *low_threshold_register = low;
    return RTD_ACQUIRE_MAX31865_RESULT_OK;
}

rtd_acquire_max31865_result_t rtd_acquire_max31865_measurement_from_registers(
    const rtd_acquire_max31865_config_t *config,
    uint16_t rtd_register,
    uint8_t fault_status_register,
    rtd_acquire_measurement_t *measurement
)
{
    size_t mapping_index;
    size_t diagnostic_count = 0U;
    bool has_fault = false;
    rtd_acquire_max31865_result_t storage_result;

    if (config == NULL || measurement == NULL) {
        return RTD_ACQUIRE_MAX31865_RESULT_INVALID_ARGUMENT;
    }
    if (!rtd_acquire_max31865_config_is_valid(config)) {
        return RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR;
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

    storage_result = rtd_acquire_max31865_measurement_storage_result(
        measurement,
        diagnostic_count,
        diagnostic_count
    );
    if (storage_result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
        return storage_result;
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

    if (!rtd_acquire_measurement_is_valid(measurement)) {
        return RTD_ACQUIRE_MAX31865_RESULT_INTERNAL_ERROR;
    }
    return RTD_ACQUIRE_MAX31865_RESULT_OK;
}

static bool rtd_acquire_max31865_spi_settings_are_compatible(
    const rtd_acquire_spi_settings_t *settings
)
{
    if (settings == NULL) {
        return false;
    }

    return settings->clock_polarity <= 1U
        && settings->clock_phase == 1U
        && settings->clock_frequency_hz > 0U
        && settings->clock_frequency_hz <= RTD_ACQUIRE_MAX31865_MAX_SPI_CLOCK_HZ
        && settings->bit_order == RTD_ACQUIRE_SPI_MSB_FIRST
        && settings->bits_per_word == 8U
        && settings->chip_select_active_low;
}

static rtd_acquire_max31865_result_t rtd_acquire_max31865_timing_delays(
    const rtd_acquire_max31865_timing_t *timing,
    uint32_t *bias_settle_us,
    uint32_t *post_fault_settle_us
)
{
    uint32_t tau;
    uint32_t half_tau_rounded_up;

    if (timing == NULL || bias_settle_us == NULL
        || post_fault_settle_us == NULL) {
        return RTD_ACQUIRE_MAX31865_RESULT_INVALID_ARGUMENT;
    }

    tau = timing->input_filter_time_constant_us;
    half_tau_rounded_up = tau / 2U + tau % 2U;

    if (half_tau_rounded_up > UINT32_MAX - 1000U
        || tau > (UINT32_MAX - 1000U - half_tau_rounded_up) / 10U
        || tau > (UINT32_MAX - 1000U) / 5U) {
        return RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR;
    }

    *bias_settle_us = 10U * tau + half_tau_rounded_up + 1000U;
    *post_fault_settle_us = 5U * tau + 1000U;
    return RTD_ACQUIRE_MAX31865_RESULT_OK;
}

static rtd_acquire_max31865_result_t rtd_acquire_max31865_spi_transfer(
    const rtd_acquire_spi_t *spi,
    const uint8_t *tx,
    uint8_t *rx,
    size_t length
)
{
    if (spi->transfer(spi->context, tx, rx, length) != RTD_ACQUIRE_SPI_OK) {
        return RTD_ACQUIRE_MAX31865_RESULT_SPI_IO_ERROR;
    }
    return RTD_ACQUIRE_MAX31865_RESULT_OK;
}

static rtd_acquire_max31865_result_t rtd_acquire_max31865_write_config(
    const rtd_acquire_spi_t *spi,
    uint8_t value
)
{
    const uint8_t tx[2] = {RTD_ACQUIRE_MAX31865_CONFIG_WRITE, value};
    uint8_t rx[2];

    return rtd_acquire_max31865_spi_transfer(spi, tx, rx, sizeof(tx));
}

static void rtd_acquire_max31865_best_effort_bias_off(
    const rtd_acquire_spi_t *spi,
    uint8_t base_config
)
{
    (void)rtd_acquire_max31865_write_config(spi, base_config);
}

static rtd_acquire_max31865_result_t rtd_acquire_max31865_delay(
    const rtd_acquire_delay_t *delay,
    uint32_t duration_us
)
{
    if (delay->delay_us(delay->context, duration_us) != RTD_ACQUIRE_DELAY_OK) {
        return RTD_ACQUIRE_MAX31865_RESULT_DELAY_ERROR;
    }
    return RTD_ACQUIRE_MAX31865_RESULT_OK;
}

rtd_acquire_max31865_result_t rtd_acquire_max31865_read(
    const rtd_acquire_spi_t *spi,
    const rtd_acquire_delay_t *delay,
    const rtd_acquire_max31865_config_t *config,
    const rtd_acquire_max31865_timing_t *timing,
    rtd_acquire_measurement_t *measurement
)
{
    rtd_acquire_max31865_result_t result;
    uint8_t base_config;
    uint16_t high_threshold;
    uint16_t low_threshold;
    uint32_t bias_settle_us;
    uint32_t post_fault_settle_us;
    uint32_t conversion_us;
    uint8_t threshold_tx[5];
    uint8_t threshold_rx[5];
    uint8_t rtd_tx[3] = {RTD_ACQUIRE_MAX31865_RTD_READ, 0U, 0U};
    uint8_t rtd_rx[3];
    uint8_t fault_tx[2] = {RTD_ACQUIRE_MAX31865_FAULT_STATUS_READ, 0U};
    uint8_t fault_rx[2];
    uint16_t rtd_register;
    uint8_t fault_status_register = 0U;

    if (spi == NULL || delay == NULL || config == NULL || timing == NULL
        || measurement == NULL || spi->transfer == NULL
        || delay->delay_us == NULL) {
        return RTD_ACQUIRE_MAX31865_RESULT_INVALID_ARGUMENT;
    }
    if (!rtd_acquire_max31865_spi_settings_are_compatible(&spi->settings)) {
        return RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR;
    }

    result = rtd_acquire_max31865_measurement_storage_result(
        measurement,
        0U,
        0U
    );
    if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
        return result;
    }
    result = rtd_acquire_max31865_base_config_byte(config, &base_config);
    if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
        return result;
    }
    result = rtd_acquire_max31865_encode_threshold_registers(
        config,
        &high_threshold,
        &low_threshold
    );
    if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
        return result;
    }
    result = rtd_acquire_max31865_timing_delays(
        timing,
        &bias_settle_us,
        &post_fault_settle_us
    );
    if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
        return result;
    }

    conversion_us = config->filter_frequency_hz == 50U
        ? RTD_ACQUIRE_MAX31865_CONVERSION_50HZ_US
        : RTD_ACQUIRE_MAX31865_CONVERSION_60HZ_US;

    threshold_tx[0] = RTD_ACQUIRE_MAX31865_HIGH_THRESHOLD_WRITE;
    threshold_tx[1] = (uint8_t)(high_threshold >> 8U);
    threshold_tx[2] = (uint8_t)high_threshold;
    threshold_tx[3] = (uint8_t)(low_threshold >> 8U);
    threshold_tx[4] = (uint8_t)low_threshold;
    result = rtd_acquire_max31865_spi_transfer(
        spi,
        threshold_tx,
        threshold_rx,
        sizeof(threshold_tx)
    );
    if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
        return result;
    }

    result = rtd_acquire_max31865_write_config(
        spi,
        (uint8_t)(base_config | RTD_ACQUIRE_MAX31865_CONFIG_BIAS
            | RTD_ACQUIRE_MAX31865_CONFIG_CLEAR_FAULTS)
    );
    if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
        rtd_acquire_max31865_best_effort_bias_off(spi, base_config);
        return result;
    }

    result = rtd_acquire_max31865_delay(delay, bias_settle_us);
    if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
        rtd_acquire_max31865_best_effort_bias_off(spi, base_config);
        return result;
    }
    result = rtd_acquire_max31865_write_config(
        spi,
        (uint8_t)(base_config | RTD_ACQUIRE_MAX31865_CONFIG_BIAS
            | RTD_ACQUIRE_MAX31865_CONFIG_AUTO_FAULT_CYCLE)
    );
    if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
        rtd_acquire_max31865_best_effort_bias_off(spi, base_config);
        return result;
    }
    result = rtd_acquire_max31865_delay(
        delay,
        RTD_ACQUIRE_MAX31865_FAULT_CYCLE_MAX_US
    );
    if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
        rtd_acquire_max31865_best_effort_bias_off(spi, base_config);
        return result;
    }
    result = rtd_acquire_max31865_delay(delay, post_fault_settle_us);
    if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
        rtd_acquire_max31865_best_effort_bias_off(spi, base_config);
        return result;
    }
    result = rtd_acquire_max31865_write_config(
        spi,
        (uint8_t)(base_config | RTD_ACQUIRE_MAX31865_CONFIG_BIAS
            | RTD_ACQUIRE_MAX31865_CONFIG_ONE_SHOT)
    );
    if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
        rtd_acquire_max31865_best_effort_bias_off(spi, base_config);
        return result;
    }
    result = rtd_acquire_max31865_delay(delay, conversion_us);
    if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
        rtd_acquire_max31865_best_effort_bias_off(spi, base_config);
        return result;
    }
    result = rtd_acquire_max31865_spi_transfer(
        spi,
        rtd_tx,
        rtd_rx,
        sizeof(rtd_tx)
    );
    if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
        rtd_acquire_max31865_best_effort_bias_off(spi, base_config);
        return result;
    }

    rtd_register = (uint16_t)((uint16_t)rtd_rx[1] << 8U) | rtd_rx[2];
    if ((rtd_register & 0x0001U) != 0U) {
        result = rtd_acquire_max31865_spi_transfer(
            spi,
            fault_tx,
            fault_rx,
            sizeof(fault_tx)
        );
        if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
            rtd_acquire_max31865_best_effort_bias_off(spi, base_config);
            return result;
        }
        fault_status_register = fault_rx[1];
    }

    result = rtd_acquire_max31865_write_config(spi, base_config);
    if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
        return result;
    }

    return rtd_acquire_max31865_measurement_from_registers(
        config,
        rtd_register,
        fault_status_register,
        measurement
    );
}
