#include "rtd_acquire/core.h"

#include <math.h>

static bool rtd_acquire_text_has_content(const char *text)
{
    if (text == NULL) {
        return false;
    }

    while (*text != '\0') {
        switch (*text) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
            case '\f':
            case '\v':
                ++text;
                break;
            default:
                return true;
        }
    }

    return false;
}

static bool rtd_acquire_diagnostic_code_is_valid(
    rtd_acquire_diagnostic_code_t code
)
{
    const int value = (int)code;

    return value >= 0 && value < (int)RTD_ACQUIRE_DIAGNOSTIC_CODE_COUNT;
}

static bool rtd_acquire_diagnostic_severity_is_valid(
    rtd_acquire_diagnostic_severity_t severity
)
{
    return severity == RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_WARNING
        || severity == RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT;
}

void rtd_acquire_measurement_reset(rtd_acquire_measurement_t *measurement)
{
    if (measurement == NULL) {
        return;
    }

    measurement->has_resistance = false;
    measurement->resistance_ohms = 0.0F;
    measurement->has_standard_uncertainty = false;
    measurement->standard_uncertainty_ohms = 0.0F;
    measurement->diagnostic_count = 0U;
    measurement->native_evidence_count = 0U;
}

void rtd_acquire_measurement_init(
    rtd_acquire_measurement_t *measurement,
    rtd_acquire_diagnostic_t *diagnostics,
    size_t diagnostic_capacity,
    rtd_acquire_native_evidence_t *native_evidence,
    size_t native_evidence_capacity
)
{
    if (measurement == NULL) {
        return;
    }

    measurement->diagnostics = diagnostics;
    measurement->diagnostic_capacity = diagnostic_capacity;
    measurement->native_evidence = native_evidence;
    measurement->native_evidence_capacity = native_evidence_capacity;
    rtd_acquire_measurement_reset(measurement);
}

rtd_acquire_measurement_status_t rtd_acquire_measurement_status(
    const rtd_acquire_measurement_t *measurement
)
{
    size_t index;

    for (index = 0U; index < measurement->diagnostic_count; ++index) {
        if (measurement->diagnostics[index].severity
            == RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT) {
            return RTD_ACQUIRE_MEASUREMENT_STATUS_FAULT;
        }
    }

    if (measurement->diagnostic_count > 0U) {
        return RTD_ACQUIRE_MEASUREMENT_STATUS_WARNING;
    }

    return RTD_ACQUIRE_MEASUREMENT_STATUS_OK;
}

bool rtd_acquire_measurement_is_valid(
    const rtd_acquire_measurement_t *measurement
)
{
    size_t diagnostic_index;
    bool has_fault = false;

    if (measurement == NULL) {
        return false;
    }
    if (measurement->diagnostic_count > measurement->diagnostic_capacity
        || measurement->native_evidence_count
            > measurement->native_evidence_capacity) {
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
    if (measurement->has_resistance
        && (!isfinite(measurement->resistance_ohms)
            || measurement->resistance_ohms < 0.0F)) {
        return false;
    }
    if (measurement->has_standard_uncertainty
        && (!isfinite(measurement->standard_uncertainty_ohms)
            || measurement->standard_uncertainty_ohms < 0.0F)) {
        return false;
    }

    for (diagnostic_index = 0U;
         diagnostic_index < measurement->diagnostic_count;
         ++diagnostic_index) {
        const rtd_acquire_diagnostic_t *diagnostic =
            &measurement->diagnostics[diagnostic_index];
        size_t earlier_index;
        size_t evidence_index;

        if (!rtd_acquire_diagnostic_code_is_valid(diagnostic->code)
            || !rtd_acquire_diagnostic_severity_is_valid(
                diagnostic->severity
            )) {
            return false;
        }

        for (earlier_index = 0U;
             earlier_index < diagnostic_index;
             ++earlier_index) {
            if (measurement->diagnostics[earlier_index].code
                == diagnostic->code) {
                return false;
            }
        }

        if (diagnostic->native_evidence_offset
            > measurement->native_evidence_count) {
            return false;
        }
        if (diagnostic->native_evidence_count
            > measurement->native_evidence_count
                - diagnostic->native_evidence_offset) {
            return false;
        }

        for (evidence_index = 0U;
             evidence_index < diagnostic->native_evidence_count;
             ++evidence_index) {
            const rtd_acquire_native_evidence_t *evidence =
                &measurement->native_evidence[
                    diagnostic->native_evidence_offset + evidence_index
                ];

            if (!rtd_acquire_text_has_content(evidence->identifier)
                && !rtd_acquire_text_has_content(evidence->message)) {
                return false;
            }
        }

        if (diagnostic->severity == RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT) {
            has_fault = true;
        }
    }

    if (has_fault) {
        return !measurement->has_resistance
            && !measurement->has_standard_uncertainty;
    }

    return measurement->has_resistance;
}
