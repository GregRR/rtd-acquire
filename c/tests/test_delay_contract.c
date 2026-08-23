#include <assert.h>
#include <stdint.h>

#include "rtd_acquire/delay.h"

typedef struct {
    unsigned int calls;
    uint32_t last_duration_us;
    rtd_acquire_delay_result_t result;
} fake_delay_context_t;

static rtd_acquire_delay_result_t fake_delay_us(
    void *context,
    uint32_t duration_us
)
{
    fake_delay_context_t *state = (fake_delay_context_t *)context;

    state->calls += 1U;
    state->last_duration_us = duration_us;
    return state->result;
}

int main(void)
{
    fake_delay_context_t context = {0U, 0U, RTD_ACQUIRE_DELAY_OK};
    rtd_acquire_delay_t delay = {
        .context = &context,
        .delay_us = fake_delay_us,
    };

    assert(delay.delay_us(delay.context, 11500U) == RTD_ACQUIRE_DELAY_OK);
    assert(context.calls == 1U);
    assert(context.last_duration_us == 11500U);

    context.result = RTD_ACQUIRE_DELAY_ERROR;
    assert(delay.delay_us(delay.context, 600U) == RTD_ACQUIRE_DELAY_ERROR);
    assert(context.calls == 2U);
    assert(context.last_duration_us == 600U);

    return 0;
}
