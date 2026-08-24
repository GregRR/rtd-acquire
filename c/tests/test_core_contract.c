#include <assert.h>
#include <math.h>
#include <stddef.h>

#include "rtd_acquire/core.h"

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

static void test_ok_measurement(void)
{
    rtd_acquire_diagnostic_t diagnostics[2];
    rtd_acquire_native_evidence_t evidence[2];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        2U,
        evidence,
        2U
    );

    measurement.has_resistance = true;
    measurement.resistance_ohms = 109.73F;
    measurement.has_standard_uncertainty = true;
    measurement.standard_uncertainty_ohms = 0.02F;

    assert(rtd_acquire_measurement_is_valid(&measurement));
    assert(
        rtd_acquire_measurement_status(&measurement)
        == RTD_ACQUIRE_MEASUREMENT_STATUS_OK
    );

    measurement.resistance_ohms = 0.0F;
    measurement.standard_uncertainty_ohms = 0.0F;
    assert(rtd_acquire_measurement_is_valid(&measurement));
}

static void test_warning_measurement_with_native_evidence(void)
{
    rtd_acquire_diagnostic_t diagnostics[2];
    rtd_acquire_native_evidence_t evidence[2];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        2U,
        evidence,
        2U
    );

    measurement.has_resistance = true;
    measurement.resistance_ohms = 140.0F;
    measurement.diagnostic_count = 1U;
    measurement.native_evidence_count = 1U;
    diagnostics[0] = (rtd_acquire_diagnostic_t){
        .code = RTD_ACQUIRE_DIAGNOSTIC_CODE_RESISTANCE_HIGH_THRESHOLD,
        .severity = RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_WARNING,
        .native_evidence_offset = 0U,
        .native_evidence_count = 1U,
    };
    evidence[0] = (rtd_acquire_native_evidence_t){
        .identifier = "D7",
        .message = NULL,
    };

    assert(rtd_acquire_measurement_is_valid(&measurement));
    assert(
        rtd_acquire_measurement_status(&measurement)
        == RTD_ACQUIRE_MEASUREMENT_STATUS_WARNING
    );
}

static void test_fault_measurement(void)
{
    rtd_acquire_diagnostic_t diagnostics[1];
    rtd_acquire_native_evidence_t evidence[1];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        1U,
        evidence,
        1U
    );

    measurement.diagnostic_count = 1U;
    diagnostics[0] = (rtd_acquire_diagnostic_t){
        .code = RTD_ACQUIRE_DIAGNOSTIC_CODE_SENSOR_INPUT_FAULT,
        .severity = RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT,
        .native_evidence_offset = 0U,
        .native_evidence_count = 0U,
    };

    assert(rtd_acquire_measurement_is_valid(&measurement));
    assert(
        rtd_acquire_measurement_status(&measurement)
        == RTD_ACQUIRE_MEASUREMENT_STATUS_FAULT
    );
}

static void test_fault_status_wins_over_warning(void)
{
    rtd_acquire_diagnostic_t diagnostics[2];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        2U,
        NULL,
        0U
    );

    measurement.diagnostic_count = 2U;
    diagnostics[0] = (rtd_acquire_diagnostic_t){
        .code = RTD_ACQUIRE_DIAGNOSTIC_CODE_RESISTANCE_HIGH_THRESHOLD,
        .severity = RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_WARNING,
        .native_evidence_offset = 0U,
        .native_evidence_count = 0U,
    };
    diagnostics[1] = (rtd_acquire_diagnostic_t){
        .code = RTD_ACQUIRE_DIAGNOSTIC_CODE_SENSOR_INPUT_FAULT,
        .severity = RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT,
        .native_evidence_offset = 0U,
        .native_evidence_count = 0U,
    };

    assert(rtd_acquire_measurement_is_valid(&measurement));
    assert(
        rtd_acquire_measurement_status(&measurement)
        == RTD_ACQUIRE_MEASUREMENT_STATUS_FAULT
    );
}

static void test_composite_native_evidence(void)
{
    rtd_acquire_diagnostic_t diagnostics[1];
    rtd_acquire_native_evidence_t evidence[2];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        1U,
        evidence,
        2U
    );

    measurement.diagnostic_count = 1U;
    measurement.native_evidence_count = 2U;
    diagnostics[0] = (rtd_acquire_diagnostic_t){
        .code = RTD_ACQUIRE_DIAGNOSTIC_CODE_SENSOR_CIRCUIT_OPEN,
        .severity = RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT,
        .native_evidence_offset = 0U,
        .native_evidence_count = 2U,
    };
    evidence[0] = (rtd_acquire_native_evidence_t){
        .identifier = "0x60n0:02",
        .message = "Overrange",
    };
    evidence[1] = (rtd_acquire_native_evidence_t){
        .identifier = "0x60n0:07",
        .message = "Error",
    };

    assert(rtd_acquire_measurement_is_valid(&measurement));
}

static void test_reset_preserves_storage(void)
{
    rtd_acquire_diagnostic_t diagnostics[2];
    rtd_acquire_native_evidence_t evidence[3];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        2U,
        evidence,
        3U
    );

    measurement.has_resistance = true;
    measurement.resistance_ohms = 100.0F;
    measurement.has_standard_uncertainty = true;
    measurement.standard_uncertainty_ohms = 0.1F;
    measurement.diagnostic_count = 1U;
    measurement.native_evidence_count = 1U;

    rtd_acquire_measurement_reset(&measurement);

    assert(!measurement.has_resistance);
    assert(!measurement.has_standard_uncertainty);
    assert(measurement.diagnostic_count == 0U);
    assert(measurement.native_evidence_count == 0U);
    assert(measurement.diagnostics == diagnostics);
    assert(measurement.diagnostic_capacity == 2U);
    assert(measurement.native_evidence == evidence);
    assert(measurement.native_evidence_capacity == 3U);
}

static void test_invalid_measurements_are_rejected(void)
{
    rtd_acquire_diagnostic_t diagnostics[2];
    rtd_acquire_native_evidence_t evidence[2];
    rtd_acquire_measurement_t measurement;

    assert(!rtd_acquire_measurement_is_valid(NULL));

    measurement = make_measurement(diagnostics, 2U, evidence, 2U);
    assert(!rtd_acquire_measurement_is_valid(&measurement));

    measurement.has_resistance = true;
    measurement.resistance_ohms = -1.0F;
    assert(!rtd_acquire_measurement_is_valid(&measurement));

    measurement.resistance_ohms = INFINITY;
    assert(!rtd_acquire_measurement_is_valid(&measurement));

    measurement.resistance_ohms = NAN;
    assert(!rtd_acquire_measurement_is_valid(&measurement));

    measurement.resistance_ohms = 100.0F;
    measurement.has_standard_uncertainty = true;
    measurement.standard_uncertainty_ohms = -0.1F;
    assert(!rtd_acquire_measurement_is_valid(&measurement));
}

static void test_fault_cannot_keep_resistance_or_uncertainty(void)
{
    rtd_acquire_diagnostic_t diagnostics[1];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        1U,
        NULL,
        0U
    );

    measurement.diagnostic_count = 1U;
    diagnostics[0] = (rtd_acquire_diagnostic_t){
        .code = RTD_ACQUIRE_DIAGNOSTIC_CODE_SENSOR_INPUT_FAULT,
        .severity = RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT,
        .native_evidence_offset = 0U,
        .native_evidence_count = 0U,
    };
    measurement.has_resistance = true;
    measurement.resistance_ohms = 100.0F;
    assert(!rtd_acquire_measurement_is_valid(&measurement));

    measurement.has_resistance = false;
    measurement.has_standard_uncertainty = true;
    measurement.standard_uncertainty_ohms = 0.1F;
    assert(!rtd_acquire_measurement_is_valid(&measurement));
}

static void test_duplicate_diagnostic_codes_are_rejected(void)
{
    rtd_acquire_diagnostic_t diagnostics[2];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        2U,
        NULL,
        0U
    );

    measurement.has_resistance = true;
    measurement.resistance_ohms = 100.0F;
    measurement.diagnostic_count = 2U;
    diagnostics[0] = (rtd_acquire_diagnostic_t){
        .code = RTD_ACQUIRE_DIAGNOSTIC_CODE_REFERENCE_LOW,
        .severity = RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_WARNING,
        .native_evidence_offset = 0U,
        .native_evidence_count = 0U,
    };
    diagnostics[1] = diagnostics[0];

    assert(!rtd_acquire_measurement_is_valid(&measurement));
}

static void test_storage_bounds_are_checked(void)
{
    rtd_acquire_diagnostic_t diagnostics[1];
    rtd_acquire_native_evidence_t evidence[1];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        1U,
        evidence,
        1U
    );

    measurement.has_resistance = true;
    measurement.resistance_ohms = 100.0F;
    measurement.diagnostic_count = 2U;
    assert(!rtd_acquire_measurement_is_valid(&measurement));

    measurement.diagnostic_count = 1U;
    measurement.native_evidence_count = 1U;
    diagnostics[0] = (rtd_acquire_diagnostic_t){
        .code = RTD_ACQUIRE_DIAGNOSTIC_CODE_REFERENCE_LOW,
        .severity = RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_WARNING,
        .native_evidence_offset = 1U,
        .native_evidence_count = 1U,
    };
    evidence[0] = (rtd_acquire_native_evidence_t){
        .identifier = "REF_L0",
        .message = NULL,
    };
    assert(!rtd_acquire_measurement_is_valid(&measurement));
}

static void test_native_evidence_requires_content(void)
{
    rtd_acquire_diagnostic_t diagnostics[1];
    rtd_acquire_native_evidence_t evidence[1];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        1U,
        evidence,
        1U
    );

    measurement.has_resistance = true;
    measurement.resistance_ohms = 100.0F;
    measurement.diagnostic_count = 1U;
    measurement.native_evidence_count = 1U;
    diagnostics[0] = (rtd_acquire_diagnostic_t){
        .code = RTD_ACQUIRE_DIAGNOSTIC_CODE_REFERENCE_LOW,
        .severity = RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_WARNING,
        .native_evidence_offset = 0U,
        .native_evidence_count = 1U,
    };
    evidence[0] = (rtd_acquire_native_evidence_t){
        .identifier = "   ",
        .message = "\t",
    };

    assert(!rtd_acquire_measurement_is_valid(&measurement));
}

int main(void)
{
    test_ok_measurement();
    test_warning_measurement_with_native_evidence();
    test_fault_measurement();
    test_fault_status_wins_over_warning();
    test_composite_native_evidence();
    test_reset_preserves_storage();
    test_invalid_measurements_are_rejected();
    test_fault_cannot_keep_resistance_or_uncertainty();
    test_duplicate_diagnostic_codes_are_rejected();
    test_storage_bounds_are_checked();
    test_native_evidence_requires_content();
    return 0;
}
