#include <assert.h>
#include <stddef.h>
#include <string.h>

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

static rtd_acquire_measurement_t make_measurement(
    rtd_acquire_diagnostic_t *diagnostics,
    size_t diagnostic_capacity,
    rtd_acquire_native_evidence_t *native_evidence,
    size_t native_evidence_capacity
)
{
    rtd_acquire_measurement_t measurement;

    rtd_acquire_measurement_init(
        &measurement,
        diagnostics,
        diagnostic_capacity,
        native_evidence,
        native_evidence_capacity
    );
    return measurement;
}

static void test_quarter_scale_measurement(void)
{
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_diagnostic_t diagnostics[1];
    rtd_acquire_native_evidence_t evidence[1];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        1U,
        evidence,
        1U
    );

    assert(
        rtd_acquire_max31865_measurement_from_registers(
            &config,
            0x4000U,
            0x00U,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_OK
    );
    assert(measurement.has_resistance);
    assert(measurement.resistance_ohms == 107.5F);
    assert(!measurement.has_standard_uncertainty);
    assert(measurement.diagnostic_count == 0U);
    assert(measurement.native_evidence_count == 0U);
    assert(rtd_acquire_measurement_is_valid(&measurement));
    assert(
        rtd_acquire_measurement_status(&measurement)
        == RTD_ACQUIRE_MEASUREMENT_STATUS_OK
    );
}

static void test_high_threshold_warning_preserves_resistance(void)
{
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_diagnostic_t diagnostics[1];
    rtd_acquire_native_evidence_t evidence[1];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        1U,
        evidence,
        1U
    );

    assert(
        rtd_acquire_max31865_measurement_from_registers(
            &config,
            0x4001U,
            0x80U,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_OK
    );
    assert(measurement.has_resistance);
    assert(measurement.resistance_ohms == 107.5F);
    assert(measurement.diagnostic_count == 1U);
    assert(measurement.native_evidence_count == 1U);
    assert(
        diagnostics[0].code
        == RTD_ACQUIRE_DIAGNOSTIC_CODE_RESISTANCE_HIGH_THRESHOLD
    );
    assert(
        diagnostics[0].severity
        == RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_WARNING
    );
    assert(diagnostics[0].native_evidence_offset == 0U);
    assert(diagnostics[0].native_evidence_count == 1U);
    assert(strcmp(evidence[0].identifier, "D7") == 0);
    assert(strcmp(evidence[0].message, "RTD High Threshold") == 0);
    assert(rtd_acquire_measurement_is_valid(&measurement));
    assert(
        rtd_acquire_measurement_status(&measurement)
        == RTD_ACQUIRE_MEASUREMENT_STATUS_WARNING
    );
}

static void test_fault_removes_resistance(void)
{
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_diagnostic_t diagnostics[1];
    rtd_acquire_native_evidence_t evidence[1];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        1U,
        evidence,
        1U
    );

    assert(
        rtd_acquire_max31865_measurement_from_registers(
            &config,
            0x4001U,
            0x04U,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_OK
    );
    assert(!measurement.has_resistance);
    assert(!measurement.has_standard_uncertainty);
    assert(measurement.diagnostic_count == 1U);
    assert(
        diagnostics[0].code
        == RTD_ACQUIRE_DIAGNOSTIC_CODE_INPUT_VOLTAGE_FAULT
    );
    assert(
        diagnostics[0].severity
        == RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT
    );
    assert(strcmp(evidence[0].identifier, "D2") == 0);
    assert(strcmp(evidence[0].message, "Overvoltage/undervoltage fault") == 0);
    assert(rtd_acquire_measurement_is_valid(&measurement));
    assert(
        rtd_acquire_measurement_status(&measurement)
        == RTD_ACQUIRE_MEASUREMENT_STATUS_FAULT
    );
}

static void test_all_known_fault_bits_keep_datasheet_order(void)
{
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_diagnostic_t diagnostics[
        RTD_ACQUIRE_MAX31865_MAX_DIAGNOSTICS
    ];
    rtd_acquire_native_evidence_t evidence[
        RTD_ACQUIRE_MAX31865_MAX_NATIVE_EVIDENCE
    ];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        RTD_ACQUIRE_MAX31865_MAX_DIAGNOSTICS,
        evidence,
        RTD_ACQUIRE_MAX31865_MAX_NATIVE_EVIDENCE
    );

    assert(
        rtd_acquire_max31865_measurement_from_registers(
            &config,
            0x4001U,
            0xFCU,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_OK
    );
    assert(!measurement.has_resistance);
    assert(measurement.diagnostic_count == 6U);
    assert(measurement.native_evidence_count == 6U);
    assert(
        diagnostics[0].code
        == RTD_ACQUIRE_DIAGNOSTIC_CODE_RESISTANCE_HIGH_THRESHOLD
    );
    assert(
        diagnostics[1].code
        == RTD_ACQUIRE_DIAGNOSTIC_CODE_RESISTANCE_LOW_THRESHOLD
    );
    assert(
        diagnostics[2].code
        == RTD_ACQUIRE_DIAGNOSTIC_CODE_REFERENCE_INPUT_ABOVE_THRESHOLD
    );
    assert(
        diagnostics[3].code
        == RTD_ACQUIRE_DIAGNOSTIC_CODE_REFERENCE_INPUT_BELOW_THRESHOLD
    );
    assert(
        diagnostics[4].code
        == RTD_ACQUIRE_DIAGNOSTIC_CODE_RTD_INPUT_BELOW_THRESHOLD
    );
    assert(
        diagnostics[5].code
        == RTD_ACQUIRE_DIAGNOSTIC_CODE_INPUT_VOLTAGE_FAULT
    );
    assert(rtd_acquire_measurement_is_valid(&measurement));
}

static void test_reserved_fault_bits_are_ignored(void)
{
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_measurement_t measurement = make_measurement(
        NULL,
        0U,
        NULL,
        0U
    );

    assert(
        rtd_acquire_max31865_measurement_from_registers(
            &config,
            0x0000U,
            0x03U,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_OK
    );
    assert(measurement.has_resistance);
    assert(measurement.resistance_ohms == 0.0F);
    assert(measurement.diagnostic_count == 0U);
    assert(rtd_acquire_measurement_is_valid(&measurement));
}

static void test_reserved_fault_bits_are_ignored_with_warning(void)
{
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_diagnostic_t diagnostics[1];
    rtd_acquire_native_evidence_t evidence[1];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        1U,
        evidence,
        1U
    );

    assert(
        rtd_acquire_max31865_measurement_from_registers(
            &config,
            0x4001U,
            0x83U,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_OK
    );
    assert(measurement.has_resistance);
    assert(measurement.resistance_ohms == 107.5F);
    assert(measurement.diagnostic_count == 1U);
    assert(
        diagnostics[0].code
        == RTD_ACQUIRE_DIAGNOSTIC_CODE_RESISTANCE_HIGH_THRESHOLD
    );
    assert(rtd_acquire_measurement_is_valid(&measurement));
}

static void test_insufficient_storage_is_rejected_without_reset(void)
{
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_diagnostic_t diagnostics[1];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        1U,
        NULL,
        0U
    );

    measurement.has_resistance = true;
    measurement.resistance_ohms = 12.5F;

    assert(
        rtd_acquire_max31865_measurement_from_registers(
            &config,
            0x4001U,
            0x80U,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_INSUFFICIENT_STORAGE
    );
    assert(measurement.has_resistance);
    assert(measurement.resistance_ohms == 12.5F);
    assert(measurement.diagnostic_count == 0U);
}

static void test_unusable_storage_is_invalid_argument(void)
{
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_measurement_t measurement = make_measurement(
        NULL,
        1U,
        NULL,
        0U
    );

    assert(
        rtd_acquire_max31865_measurement_from_registers(
            &config,
            0x4000U,
            0x00U,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_INVALID_ARGUMENT
    );
}

static void test_invalid_arguments_are_rejected(void)
{
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_measurement_t measurement = make_measurement(
        NULL,
        0U,
        NULL,
        0U
    );

    assert(
        rtd_acquire_max31865_measurement_from_registers(
            NULL,
            0x4000U,
            0x00U,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_INVALID_ARGUMENT
    );
    assert(
        rtd_acquire_max31865_measurement_from_registers(
            &config,
            0x4000U,
            0x00U,
            NULL
        ) == RTD_ACQUIRE_MAX31865_RESULT_INVALID_ARGUMENT
    );

    config.reference_resistance_ohms = 100.0F;
    assert(
        rtd_acquire_max31865_measurement_from_registers(
            &config,
            0x4000U,
            0x00U,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR
    );
}

int main(void)
{
    test_quarter_scale_measurement();
    test_high_threshold_warning_preserves_resistance();
    test_fault_removes_resistance();
    test_all_known_fault_bits_keep_datasheet_order();
    test_reserved_fault_bits_are_ignored();
    test_reserved_fault_bits_are_ignored_with_warning();
    test_insufficient_storage_is_rejected_without_reset();
    test_unusable_storage_is_invalid_argument();
    test_invalid_arguments_are_rejected();
    return 0;
}
