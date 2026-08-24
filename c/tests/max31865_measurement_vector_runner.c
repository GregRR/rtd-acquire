#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "rtd_acquire/max31865.h"

static const char *status_name(rtd_acquire_measurement_status_t status)
{
    switch (status) {
        case RTD_ACQUIRE_MEASUREMENT_STATUS_OK:
            return "ok";
        case RTD_ACQUIRE_MEASUREMENT_STATUS_WARNING:
            return "warning";
        case RTD_ACQUIRE_MEASUREMENT_STATUS_FAULT:
            return "fault";
    }
    return "unknown";
}

static const char *severity_name(rtd_acquire_diagnostic_severity_t severity)
{
    switch (severity) {
        case RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_WARNING:
            return "warning";
        case RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT:
            return "fault";
    }
    return "unknown";
}

static const char *code_name(rtd_acquire_diagnostic_code_t code)
{
    switch (code) {
        case RTD_ACQUIRE_DIAGNOSTIC_CODE_RESISTANCE_HIGH_THRESHOLD:
            return "resistance_high_threshold";
        case RTD_ACQUIRE_DIAGNOSTIC_CODE_RESISTANCE_LOW_THRESHOLD:
            return "resistance_low_threshold";
        case RTD_ACQUIRE_DIAGNOSTIC_CODE_REFERENCE_INPUT_ABOVE_THRESHOLD:
            return "reference_input_above_threshold";
        case RTD_ACQUIRE_DIAGNOSTIC_CODE_REFERENCE_INPUT_BELOW_THRESHOLD:
            return "reference_input_below_threshold";
        case RTD_ACQUIRE_DIAGNOSTIC_CODE_RTD_INPUT_BELOW_THRESHOLD:
            return "rtd_input_below_threshold";
        case RTD_ACQUIRE_DIAGNOSTIC_CODE_INPUT_VOLTAGE_FAULT:
            return "input_voltage_fault";
        default:
            return "unknown";
    }
}

static bool parse_real(const char *text, rtd_acquire_real_t *value)
{
    char *end = NULL;
    const float parsed = strtof(text, &end);

    if (end == text || *end != '\0') {
        return false;
    }
    *value = parsed;
    return true;
}

static bool parse_u16(const char *text, uint16_t *value)
{
    char *end = NULL;
    const unsigned long parsed = strtoul(text, &end, 10);

    if (end == text || *end != '\0' || parsed > UINT16_MAX) {
        return false;
    }
    *value = (uint16_t)parsed;
    return true;
}

static bool parse_u8(const char *text, uint8_t *value)
{
    char *end = NULL;
    const unsigned long parsed = strtoul(text, &end, 10);

    if (end == text || *end != '\0' || parsed > UINT8_MAX) {
        return false;
    }
    *value = (uint8_t)parsed;
    return true;
}

int main(int argc, char **argv)
{
    rtd_acquire_max31865_config_t config = {
        .reference_resistance_ohms = 0.0F,
        .wire_count = 3U,
        .filter_frequency_hz = 60U,
        .has_low_fault_threshold = false,
        .low_fault_threshold_ohms = 0.0F,
        .has_high_fault_threshold = false,
        .high_fault_threshold_ohms = 0.0F,
    };
    rtd_acquire_diagnostic_t diagnostics[
        RTD_ACQUIRE_MAX31865_MAX_DIAGNOSTICS
    ];
    rtd_acquire_native_evidence_t evidence[
        RTD_ACQUIRE_MAX31865_MAX_NATIVE_EVIDENCE
    ];
    rtd_acquire_measurement_t measurement;
    uint16_t rtd_register;
    uint8_t fault_status_register;
    size_t diagnostic_index;

    if (argc != 4) {
        return 2;
    }
    if (!parse_real(argv[1], &config.reference_resistance_ohms)
        || !parse_u16(argv[2], &rtd_register)
        || !parse_u8(argv[3], &fault_status_register)) {
        return 2;
    }

    rtd_acquire_measurement_init(
        &measurement,
        diagnostics,
        RTD_ACQUIRE_MAX31865_MAX_DIAGNOSTICS,
        evidence,
        RTD_ACQUIRE_MAX31865_MAX_NATIVE_EVIDENCE
    );
    if (!rtd_acquire_max31865_measurement_from_registers(
        &config,
        rtd_register,
        fault_status_register,
        &measurement
    )) {
        puts("decode_error");
        return 0;
    }

    printf("status %s\n", status_name(rtd_acquire_measurement_status(&measurement)));
    if (measurement.has_resistance) {
        printf("resistance %.9g\n", (double)measurement.resistance_ohms);
    } else {
        puts("resistance none");
    }
    if (measurement.has_standard_uncertainty) {
        printf(
            "uncertainty %.9g\n",
            (double)measurement.standard_uncertainty_ohms
        );
    } else {
        puts("uncertainty none");
    }
    printf("diagnostics %zu\n", measurement.diagnostic_count);

    for (diagnostic_index = 0U;
         diagnostic_index < measurement.diagnostic_count;
         ++diagnostic_index) {
        const rtd_acquire_diagnostic_t *diagnostic =
            &measurement.diagnostics[diagnostic_index];
        size_t evidence_index;

        printf(
            "diagnostic %s %s %zu\n",
            code_name(diagnostic->code),
            severity_name(diagnostic->severity),
            diagnostic->native_evidence_count
        );
        for (evidence_index = 0U;
             evidence_index < diagnostic->native_evidence_count;
             ++evidence_index) {
            const rtd_acquire_native_evidence_t *native =
                &measurement.native_evidence[
                    diagnostic->native_evidence_offset + evidence_index
                ];
            printf(
                "evidence %s\t%s\n",
                native->identifier == NULL ? "none" : native->identifier,
                native->message == NULL ? "none" : native->message
            );
        }
    }

    return 0;
}
