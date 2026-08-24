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
