#ifndef RTD_ACQUIRE_MAX31865_H
#define RTD_ACQUIRE_MAX31865_H

#include <stdbool.h>
#include <stdint.h>

#include "rtd_acquire/core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    rtd_acquire_real_t reference_resistance_ohms;
    uint8_t wire_count;
    uint8_t filter_frequency_hz;
    bool has_low_fault_threshold;
    rtd_acquire_real_t low_fault_threshold_ohms;
    bool has_high_fault_threshold;
    rtd_acquire_real_t high_fault_threshold_ohms;
} rtd_acquire_max31865_config_t;

bool rtd_acquire_max31865_config_is_valid(
    const rtd_acquire_max31865_config_t *config
);

bool rtd_acquire_max31865_base_config_byte(
    const rtd_acquire_max31865_config_t *config,
    uint8_t *value
);

bool rtd_acquire_max31865_encode_threshold_registers(
    const rtd_acquire_max31865_config_t *config,
    uint16_t *high_threshold_register,
    uint16_t *low_threshold_register
);

#ifdef __cplusplus
}
#endif

#endif
