#ifndef RTD_ACQUIRE_DELAY_H
#define RTD_ACQUIRE_DELAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RTD_ACQUIRE_DELAY_OK = 0,
    RTD_ACQUIRE_DELAY_ERROR = 1
} rtd_acquire_delay_result_t;

/*
 * Block for at least duration_us microseconds. A successful return must not
 * occur before the requested interval has elapsed.
 */
typedef rtd_acquire_delay_result_t (*rtd_acquire_delay_us_fn)(
    void *context,
    uint32_t duration_us
);

typedef struct {
    void *context;
    rtd_acquire_delay_us_fn delay_us;
} rtd_acquire_delay_t;

#ifdef __cplusplus
}
#endif

#endif
