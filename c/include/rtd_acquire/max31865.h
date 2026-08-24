#ifndef RTD_ACQUIRE_MAX31865_H
#define RTD_ACQUIRE_MAX31865_H

#include <stdbool.h>
#include <stdint.h>

#include "rtd_acquire/core.h"

#define RTD_ACQUIRE_MAX31865_MAX_DIAGNOSTICS 6U
#define RTD_ACQUIRE_MAX31865_MAX_NATIVE_EVIDENCE 6U

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Public MAX31865 operation results. Configuration errors include invalid
 * electrical configuration and, for the later acquisition sequence,
 * incompatible SPI settings. SPI and delay failures are acquisition-operation
 * failures; insufficient storage is specific to caller-owned C result buffers.
 */
typedef enum {
    RTD_ACQUIRE_MAX31865_RESULT_OK = 0,
    RTD_ACQUIRE_MAX31865_RESULT_INVALID_ARGUMENT = 1,
    RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR = 2,
    RTD_ACQUIRE_MAX31865_RESULT_INSUFFICIENT_STORAGE = 3,
    RTD_ACQUIRE_MAX31865_RESULT_SPI_IO_ERROR = 4,
    RTD_ACQUIRE_MAX31865_RESULT_DELAY_ERROR = 5,
    RTD_ACQUIRE_MAX31865_RESULT_INTERNAL_ERROR = 6
} rtd_acquire_max31865_result_t;

typedef struct {
    rtd_acquire_real_t reference_resistance_ohms;
    uint8_t wire_count;
    uint8_t filter_frequency_hz;
    bool has_low_fault_threshold;
    rtd_acquire_real_t low_fault_threshold_ohms;
    bool has_high_fault_threshold;
    rtd_acquire_real_t high_fault_threshold_ohms;
} rtd_acquire_max31865_config_t;

/* Return true only when config is a valid static MAX31865 configuration. */
bool rtd_acquire_max31865_config_is_valid(
    const rtd_acquire_max31865_config_t *config
);

/* Encode only the static wire/filter configuration bits into *value. */
rtd_acquire_max31865_result_t rtd_acquire_max31865_base_config_byte(
    const rtd_acquire_max31865_config_t *config,
    uint8_t *value
);

/* Encode the high/low threshold register words without performing I/O. */
rtd_acquire_max31865_result_t rtd_acquire_max31865_encode_threshold_registers(
    const rtd_acquire_max31865_config_t *config,
    uint16_t *high_threshold_register,
    uint16_t *low_threshold_register
);

/*
 * Decode native registers into caller-owned measurement storage. The function
 * returns INSUFFICIENT_STORAGE before modifying *measurement when the supplied
 * arrays cannot preserve every required diagnostic/evidence item.
 */
rtd_acquire_max31865_result_t rtd_acquire_max31865_measurement_from_registers(
    const rtd_acquire_max31865_config_t *config,
    uint16_t rtd_register,
    uint8_t fault_status_register,
    rtd_acquire_measurement_t *measurement
);

#ifdef __cplusplus
}
#endif

#endif
