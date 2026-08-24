#ifndef RTD_ACQUIRE_CORE_H
#define RTD_ACQUIRE_CORE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef float rtd_acquire_real_t;

typedef enum {
    RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_WARNING = 0,
    RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT = 1
} rtd_acquire_diagnostic_severity_t;

typedef enum {
    RTD_ACQUIRE_DIAGNOSTIC_CODE_SENSOR_CIRCUIT_OPEN = 0,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_SENSOR_CIRCUIT_SHORT = 1,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_SENSOR_INPUT_FAULT = 2,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_SENSOR_NOT_DETECTED = 3,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_SENSOR_BURNOUT = 4,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_SENSOR_DRIFT = 5,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_LEAD_RESISTANCE_HIGH = 6,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_RESISTANCE_HIGH_THRESHOLD = 7,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_RESISTANCE_LOW_THRESHOLD = 8,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_INPUT_OVERRANGE = 9,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_INPUT_UNDERRANGE = 10,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_REFERENCE_LOW = 11,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_REFERENCE_FAULT = 12,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_REFERENCE_INPUT_ABOVE_THRESHOLD = 13,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_REFERENCE_INPUT_BELOW_THRESHOLD = 14,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_RTD_INPUT_BELOW_THRESHOLD = 15,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_INPUT_VOLTAGE_FAULT = 16,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_PGA_POSITIVE_OUTPUT_NEAR_POSITIVE_RAIL = 17,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_PGA_POSITIVE_OUTPUT_NEAR_NEGATIVE_RAIL = 18,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_PGA_NEGATIVE_OUTPUT_NEAR_POSITIVE_RAIL = 19,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_PGA_NEGATIVE_OUTPUT_NEAR_NEGATIVE_RAIL = 20,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_POSITIVE_INPUT_OVERVOLTAGE = 21,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_POSITIVE_INPUT_UNDERVOLTAGE = 22,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_NEGATIVE_INPUT_OVERVOLTAGE = 23,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_NEGATIVE_INPUT_UNDERVOLTAGE = 24,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_ADC_SATURATION = 25,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_CONVERSION_ERROR = 26,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_CALIBRATION_ERROR = 27,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_ANALOG_SUPPLY_FAULT = 28,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_DIGITAL_SUPPLY_FAULT = 29,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_LDO_DECOUPLING_FAULT = 30,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_DATA_CRC_ERROR = 31,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_SPI_CRC_ERROR = 32,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_SPI_CLOCK_COUNT_ERROR = 33,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_SPI_READ_ERROR = 34,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_SPI_WRITE_ERROR = 35,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_SPI_WRITE_IGNORED = 36,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_REGISTER_INTEGRITY_ERROR = 37,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_ROM_INTEGRITY_ERROR = 38,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_NONVOLATILE_MEMORY_ERROR = 39,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_CONFIGURATION_ERROR = 40,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_HARDWARE_FAULT = 41,
    RTD_ACQUIRE_DIAGNOSTIC_CODE_COUNT = 42
} rtd_acquire_diagnostic_code_t;

typedef struct {
    const char *identifier;
    const char *message;
} rtd_acquire_native_evidence_t;

typedef struct {
    rtd_acquire_diagnostic_code_t code;
    rtd_acquire_diagnostic_severity_t severity;
    size_t native_evidence_offset;
    size_t native_evidence_count;
} rtd_acquire_diagnostic_t;

typedef enum {
    RTD_ACQUIRE_MEASUREMENT_STATUS_OK = 0,
    RTD_ACQUIRE_MEASUREMENT_STATUS_WARNING = 1,
    RTD_ACQUIRE_MEASUREMENT_STATUS_FAULT = 2
} rtd_acquire_measurement_status_t;

typedef struct {
    bool has_resistance;
    rtd_acquire_real_t resistance_ohms;
    bool has_standard_uncertainty;
    rtd_acquire_real_t standard_uncertainty_ohms;

    rtd_acquire_diagnostic_t *diagnostics;
    size_t diagnostic_count;
    size_t diagnostic_capacity;

    rtd_acquire_native_evidence_t *native_evidence;
    size_t native_evidence_count;
    size_t native_evidence_capacity;
} rtd_acquire_measurement_t;

void rtd_acquire_measurement_init(
    rtd_acquire_measurement_t *measurement,
    rtd_acquire_diagnostic_t *diagnostics,
    size_t diagnostic_capacity,
    rtd_acquire_native_evidence_t *native_evidence,
    size_t native_evidence_capacity
);

void rtd_acquire_measurement_reset(rtd_acquire_measurement_t *measurement);

bool rtd_acquire_measurement_is_valid(
    const rtd_acquire_measurement_t *measurement
);

rtd_acquire_measurement_status_t rtd_acquire_measurement_status(
    const rtd_acquire_measurement_t *measurement
);

#ifdef __cplusplus
}
#endif

#endif
