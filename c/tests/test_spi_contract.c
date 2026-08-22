#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "rtd_acquire/spi.h"

typedef struct {
    unsigned int calls;
} fake_spi_context_t;

static rtd_acquire_spi_result_t fake_transfer(
    void *context,
    const uint8_t *tx,
    uint8_t *rx,
    size_t length
)
{
    fake_spi_context_t *state = (fake_spi_context_t *)context;
    size_t index;

    state->calls += 1U;
    for (index = 0U; index < length; ++index) {
        rx[index] = tx[index];
    }

    return RTD_ACQUIRE_SPI_OK;
}

int main(void)
{
    fake_spi_context_t context = {0U};
    const uint8_t tx[] = {0x01U, 0x02U, 0x03U};
    uint8_t rx[] = {0U, 0U, 0U};
    rtd_acquire_spi_t spi = {
        .context = &context,
        .settings = {
            .clock_polarity = 0U,
            .clock_phase = 1U,
            .clock_frequency_hz = 1000000U,
            .bit_order = RTD_ACQUIRE_SPI_MSB_FIRST,
            .bits_per_word = 8U,
            .chip_select_active_low = true,
        },
        .transfer = fake_transfer,
    };

    assert(spi.settings.clock_phase == 1U);
    assert(spi.settings.bit_order == RTD_ACQUIRE_SPI_MSB_FIRST);
    assert(spi.settings.chip_select_active_low);
    assert(spi.transfer(spi.context, tx, rx, sizeof(tx)) == RTD_ACQUIRE_SPI_OK);
    assert(context.calls == 1U);
    assert(rx[0] == tx[0]);
    assert(rx[1] == tx[1]);
    assert(rx[2] == tx[2]);

    return 0;
}
