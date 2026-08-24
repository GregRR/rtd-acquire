#include <assert.h>
#include <math.h>
#include <stdint.h>

#include "rtd_acquire/max31865.h"

static rtd_acquire_max31865_config_t make_config(void)
{
    return (rtd_acquire_max31865_config_t){
        .reference_resistance_ohms = 430.0F,
        .wire_count = 3U,
        .filter_frequency_hz = 60U,
        .has_low_fault_threshold = false,
        .low_fault_threshold_ohms = 0.0F,
        .has_high_fault_threshold = false,
        .high_fault_threshold_ohms = 0.0F,
    };
}

static void test_valid_configuration_and_base_byte(void)
{
    rtd_acquire_max31865_config_t config = make_config();
    uint8_t value = 0xFFU;

    assert(rtd_acquire_max31865_config_is_valid(&config));
    assert(rtd_acquire_max31865_base_config_byte(&config, &value));
    assert(value == 0x10U);

    config.wire_count = 4U;
    config.filter_frequency_hz = 50U;
    assert(rtd_acquire_max31865_base_config_byte(&config, &value));
    assert(value == 0x01U);

    config.wire_count = 2U;
    config.filter_frequency_hz = 60U;
    assert(rtd_acquire_max31865_base_config_byte(&config, &value));
    assert(value == 0x00U);
}

static void test_reference_resistance_validation(void)
{
    rtd_acquire_max31865_config_t config = make_config();

    config.reference_resistance_ohms = 349.0F;
    assert(!rtd_acquire_max31865_config_is_valid(&config));

    config.reference_resistance_ohms = 10001.0F;
    assert(!rtd_acquire_max31865_config_is_valid(&config));

    config.reference_resistance_ohms = NAN;
    assert(!rtd_acquire_max31865_config_is_valid(&config));

    config.reference_resistance_ohms = INFINITY;
    assert(!rtd_acquire_max31865_config_is_valid(&config));
}

static void test_wire_and_filter_validation(void)
{
    rtd_acquire_max31865_config_t config = make_config();

    config.wire_count = 1U;
    assert(!rtd_acquire_max31865_config_is_valid(&config));
    config.wire_count = 3U;

    config.filter_frequency_hz = 55U;
    assert(!rtd_acquire_max31865_config_is_valid(&config));
}

static void test_threshold_validation(void)
{
    rtd_acquire_max31865_config_t config = make_config();

    config.has_low_fault_threshold = true;
    config.low_fault_threshold_ohms = 0.0F;
    assert(rtd_acquire_max31865_config_is_valid(&config));

    config.low_fault_threshold_ohms = -1.0F;
    assert(!rtd_acquire_max31865_config_is_valid(&config));

    config = make_config();
    config.has_high_fault_threshold = true;
    config.high_fault_threshold_ohms = 0.0F;
    assert(!rtd_acquire_max31865_config_is_valid(&config));

    config.high_fault_threshold_ohms = 430.0F;
    assert(!rtd_acquire_max31865_config_is_valid(&config));

    config = make_config();
    config.has_low_fault_threshold = true;
    config.low_fault_threshold_ohms = 100.0F;
    config.has_high_fault_threshold = true;
    config.high_fault_threshold_ohms = 100.0F;
    assert(!rtd_acquire_max31865_config_is_valid(&config));
}

static void test_threshold_encoding(void)
{
    rtd_acquire_max31865_config_t config = make_config();
    uint16_t high = 0U;
    uint16_t low = UINT16_MAX;

    assert(rtd_acquire_max31865_encode_threshold_registers(
        &config,
        &high,
        &low
    ));
    assert(high == UINT16_MAX);
    assert(low == 0U);

    config.reference_resistance_ohms = 400.0F;
    config.has_low_fault_threshold = true;
    config.low_fault_threshold_ohms = 50.001F;
    config.has_high_fault_threshold = true;
    config.high_fault_threshold_ohms = 100.001F;
    assert(rtd_acquire_max31865_encode_threshold_registers(
        &config,
        &high,
        &low
    ));
    assert(high == 16386U);
    assert(low == 8192U);

    config.low_fault_threshold_ohms = 0.0F;
    config.high_fault_threshold_ohms = 100.0F;
    assert(rtd_acquire_max31865_encode_threshold_registers(
        &config,
        &high,
        &low
    ));
    assert(high == 16384U);
    assert(low == 0U);
}

static void test_high_threshold_top_band(void)
{
    rtd_acquire_max31865_config_t config = make_config();
    uint16_t high = 0U;
    uint16_t low = 0U;

    config.has_high_fault_threshold = true;
    config.high_fault_threshold_ohms = 429.98687744140625F;
    assert(rtd_acquire_max31865_encode_threshold_registers(
        &config,
        &high,
        &low
    ));
    assert(high == 65534U);

    config.high_fault_threshold_ohms = 429.99F;
    assert(!rtd_acquire_max31865_config_is_valid(&config));
    assert(!rtd_acquire_max31865_encode_threshold_registers(
        &config,
        &high,
        &low
    ));
}

static void test_null_arguments_are_rejected(void)
{
    rtd_acquire_max31865_config_t config = make_config();
    uint8_t byte = 0U;
    uint16_t high = 0U;
    uint16_t low = 0U;

    assert(!rtd_acquire_max31865_config_is_valid(NULL));
    assert(!rtd_acquire_max31865_base_config_byte(NULL, &byte));
    assert(!rtd_acquire_max31865_base_config_byte(&config, NULL));
    assert(!rtd_acquire_max31865_encode_threshold_registers(
        NULL,
        &high,
        &low
    ));
    assert(!rtd_acquire_max31865_encode_threshold_registers(
        &config,
        NULL,
        &low
    ));
    assert(!rtd_acquire_max31865_encode_threshold_registers(
        &config,
        &high,
        NULL
    ));
}

int main(void)
{
    test_valid_configuration_and_base_byte();
    test_reference_resistance_validation();
    test_wire_and_filter_validation();
    test_threshold_validation();
    test_threshold_encoding();
    test_high_threshold_top_band();
    test_null_arguments_are_rejected();
    return 0;
}
