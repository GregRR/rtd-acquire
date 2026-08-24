#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtd_acquire/max31865.h"

_Static_assert(FLT_RADIX == 2, "binary32 profile requires radix-2 float");
_Static_assert(FLT_MANT_DIG == 24, "binary32 profile requires 24-bit float precision");
_Static_assert(FLT_MAX_EXP == 128, "binary32 profile requires binary32 exponent range");

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

static bool parse_optional_real(
    const char *text,
    bool *present,
    rtd_acquire_real_t *value
)
{
    if (strcmp(text, "none") == 0) {
        *present = false;
        *value = 0.0F;
        return true;
    }

    *present = true;
    return parse_real(text, value);
}

static bool parse_byte(const char *text, uint8_t *value)
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
    rtd_acquire_max31865_config_t config;
    uint16_t high;
    uint16_t low;

    if (argc != 6) {
        return 2;
    }
    if (!parse_real(argv[1], &config.reference_resistance_ohms)
        || !parse_byte(argv[2], &config.wire_count)
        || !parse_byte(argv[3], &config.filter_frequency_hz)
        || !parse_optional_real(
            argv[4],
            &config.has_low_fault_threshold,
            &config.low_fault_threshold_ohms
        )
        || !parse_optional_real(
            argv[5],
            &config.has_high_fault_threshold,
            &config.high_fault_threshold_ohms
        )) {
        return 2;
    }

    {
        const rtd_acquire_max31865_result_t result =
            rtd_acquire_max31865_encode_threshold_registers(
                &config,
                &high,
                &low
            );
        if (result == RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR) {
            puts("configuration_error");
            return 0;
        }
        if (result != RTD_ACQUIRE_MAX31865_RESULT_OK) {
            puts("operation_error");
            return 0;
        }
    }

    printf("registers %u %u\n", (unsigned int)high, (unsigned int)low);
    return 0;
}
