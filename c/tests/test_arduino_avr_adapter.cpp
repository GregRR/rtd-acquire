#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "Arduino.h"
#include "SPI.h"
#include "rtd_acquire/arduino_avr.hpp"

static rtd_acquire_spi_settings_t valid_settings(void)
{
    rtd_acquire_spi_settings_t settings = {
        0U,
        1U,
        5000000U,
        RTD_ACQUIRE_SPI_MSB_FIRST,
        8U,
        true,
    };
    return settings;
}

static void test_initialization_reports_effective_avr_clock(void)
{
    SPIClass bus;
    rtd_acquire_arduino_avr_spi_context_t context = {};
    rtd_acquire_spi_t spi = {};
    rtd_acquire_spi_settings_t settings = valid_settings();

    rtd_arduino_stub_reset();
    assert(rtd_acquire_arduino_avr_spi_init(
        &context,
        &spi,
        &bus,
        10U,
        &settings
    ));

    assert(bus.begin_calls == 1U);
    assert(spi.context == &context);
    assert(spi.settings.clock_frequency_hz == 4000000U);
    assert(spi.settings.clock_polarity == 0U);
    assert(spi.settings.clock_phase == 1U);
    assert(spi.settings.bit_order == RTD_ACQUIRE_SPI_MSB_FIRST);
    assert(spi.settings.bits_per_word == 8U);
    assert(spi.settings.chip_select_active_low);
    assert(rtd_arduino_stub_state.last_pin == 10U);
    assert(rtd_arduino_stub_state.last_pin_mode == OUTPUT);
    assert(rtd_arduino_stub_state.event_count == 3U);
    assert(rtd_arduino_stub_state.events[0] == RTD_ARDUINO_STUB_EVENT_SPI_BEGIN);
    assert(rtd_arduino_stub_state.events[1] == RTD_ARDUINO_STUB_EVENT_PIN_WRITE);
    assert(rtd_arduino_stub_state.events[2] == RTD_ARDUINO_STUB_EVENT_PIN_MODE);

    rtd_acquire_arduino_avr_spi_end(&context);
    assert(bus.end_calls == 1U);
    assert(context.bus == NULL);
}

static void test_transaction_owns_chip_select_and_settings(void)
{
    SPIClass bus;
    rtd_acquire_arduino_avr_spi_context_t context = {};
    rtd_acquire_spi_t spi = {};
    rtd_acquire_spi_settings_t settings = valid_settings();
    const uint8_t tx[] = {0x12U, 0x34U};
    uint8_t rx[] = {0U, 0U};

    assert(rtd_acquire_arduino_avr_spi_init(
        &context,
        &spi,
        &bus,
        7U,
        &settings
    ));
    rtd_arduino_stub_reset();

    assert(spi.transfer(spi.context, tx, rx, sizeof(tx)) == RTD_ACQUIRE_SPI_OK);
    assert(rx[0] == 0xEDU);
    assert(rx[1] == 0xCBU);
    assert(bus.begin_transaction_calls == 1U);
    assert(bus.end_transaction_calls == 1U);
    assert(bus.transfer_calls == 2U);
    assert(bus.last_clock_hz == 5000000U);
    assert(bus.last_bit_order == MSBFIRST);
    assert(bus.last_data_mode == SPI_MODE1);
    assert(rtd_arduino_stub_state.last_pin == 7U);
    assert(rtd_arduino_stub_state.last_pin_value == HIGH);

    assert(rtd_arduino_stub_state.event_count == 6U);
    assert(rtd_arduino_stub_state.events[0]
        == RTD_ARDUINO_STUB_EVENT_SPI_BEGIN_TRANSACTION);
    assert(rtd_arduino_stub_state.events[1] == RTD_ARDUINO_STUB_EVENT_PIN_WRITE);
    assert(rtd_arduino_stub_state.events[2] == RTD_ARDUINO_STUB_EVENT_SPI_TRANSFER);
    assert(rtd_arduino_stub_state.events[3] == RTD_ARDUINO_STUB_EVENT_SPI_TRANSFER);
    assert(rtd_arduino_stub_state.events[4] == RTD_ARDUINO_STUB_EVENT_PIN_WRITE);
    assert(rtd_arduino_stub_state.events[5]
        == RTD_ARDUINO_STUB_EVENT_SPI_END_TRANSACTION);
}

static void test_spi_adapter_rejects_unsupported_settings(void)
{
    SPIClass bus;
    rtd_acquire_arduino_avr_spi_context_t context = {};
    rtd_acquire_spi_t spi = {};
    rtd_acquire_spi_settings_t settings = valid_settings();

    settings.bits_per_word = 16U;
    assert(!rtd_acquire_arduino_avr_spi_init(
        &context,
        &spi,
        &bus,
        10U,
        &settings
    ));
    assert(bus.begin_calls == 0U);

    settings = valid_settings();
    settings.clock_polarity = 2U;
    assert(!rtd_acquire_arduino_avr_spi_init(
        &context,
        &spi,
        &bus,
        10U,
        &settings
    ));
    assert(bus.begin_calls == 0U);
}

static void test_delay_adapter_splits_long_microsecond_waits(void)
{
    rtd_acquire_delay_t delay_hal = {};

    assert(rtd_acquire_arduino_avr_delay_init(&delay_hal));
    assert(delay_hal.context == NULL);
    assert(delay_hal.delay_us != NULL);

    rtd_arduino_stub_reset();
    assert(delay_hal.delay_us(delay_hal.context, 66501U) == RTD_ACQUIRE_DELAY_OK);
    assert(rtd_arduino_stub_state.event_count == 2U);
    assert(rtd_arduino_stub_state.events[0] == RTD_ARDUINO_STUB_EVENT_DELAY_MS);
    assert(rtd_arduino_stub_state.events[1] == RTD_ARDUINO_STUB_EVENT_DELAY_US);
    assert(rtd_arduino_stub_state.last_delay_ms == 66UL);
    assert(rtd_arduino_stub_state.last_delay_us == 501U);

    rtd_arduino_stub_reset();
    assert(delay_hal.delay_us(delay_hal.context, 600U) == RTD_ACQUIRE_DELAY_OK);
    assert(rtd_arduino_stub_state.event_count == 1U);
    assert(rtd_arduino_stub_state.events[0] == RTD_ARDUINO_STUB_EVENT_DELAY_US);
    assert(rtd_arduino_stub_state.last_delay_us == 600U);
}

int main(void)
{
    test_initialization_reports_effective_avr_clock();
    test_transaction_owns_chip_select_and_settings();
    test_spi_adapter_rejects_unsupported_settings();
    test_delay_adapter_splits_long_microsecond_waits();
    return 0;
}
