// Sources: ArduinoCore-avr SPI.h and wiring.c; see docs/REFERENCES.md.
#include "rtd_acquire/arduino_avr.hpp"

#include <stddef.h>
#include <stdint.h>

static uint8_t rtd_acquire_arduino_avr_data_mode(
    uint8_t clock_polarity,
    uint8_t clock_phase
)
{
    if (clock_polarity == 0U) {
        return clock_phase == 0U ? SPI_MODE0 : SPI_MODE1;
    }
    return clock_phase == 0U ? SPI_MODE2 : SPI_MODE3;
}

static uint32_t rtd_acquire_arduino_avr_effective_clock(uint32_t requested_hz)
{
    uint32_t clock_hz = (uint32_t)(F_CPU / 2UL);
    uint16_t divisor = 2U;

    while ((clock_hz > requested_hz) && (divisor < 128U)) {
        divisor = (uint16_t)(divisor * 2U);
        clock_hz = (uint32_t)(F_CPU / (uint32_t)divisor);
    }

    return clock_hz;
}

static rtd_acquire_spi_result_t rtd_acquire_arduino_avr_transfer(
    void *opaque_context,
    const uint8_t *tx,
    uint8_t *rx,
    size_t length
)
{
    rtd_acquire_arduino_avr_spi_context_t *context =
        static_cast<rtd_acquire_arduino_avr_spi_context_t *>(opaque_context);
    size_t index;
    uint8_t asserted_level;
    uint8_t deasserted_level;

    if (context == NULL || context->bus == NULL) {
        return RTD_ACQUIRE_SPI_IO_ERROR;
    }
    if (length == 0U) {
        return RTD_ACQUIRE_SPI_OK;
    }
    if (tx == NULL || rx == NULL) {
        return RTD_ACQUIRE_SPI_IO_ERROR;
    }

    asserted_level = context->chip_select_active_low ? LOW : HIGH;
    deasserted_level = context->chip_select_active_low ? HIGH : LOW;

    context->bus->beginTransaction(SPISettings(
        context->requested_clock_frequency_hz,
        context->arduino_bit_order,
        context->arduino_data_mode
    ));
    digitalWrite(context->chip_select_pin, asserted_level);
    for (index = 0U; index < length; ++index) {
        rx[index] = context->bus->transfer(tx[index]);
    }
    digitalWrite(context->chip_select_pin, deasserted_level);
    context->bus->endTransaction();

    return RTD_ACQUIRE_SPI_OK;
}

static rtd_acquire_delay_result_t rtd_acquire_arduino_avr_delay_us(
    void *context,
    uint32_t duration_us
)
{
    uint32_t whole_ms;
    uint16_t remainder_us;

    (void)context;

    whole_ms = duration_us / 1000U;
    remainder_us = (uint16_t)(duration_us % 1000U);

    if (whole_ms > 0U) {
        delay((unsigned long)whole_ms);
    }
    if (remainder_us > 0U) {
        delayMicroseconds((unsigned int)remainder_us);
    }

    return RTD_ACQUIRE_DELAY_OK;
}

bool rtd_acquire_arduino_avr_spi_init(
    rtd_acquire_arduino_avr_spi_context_t *context,
    rtd_acquire_spi_t *spi,
    SPIClass *bus,
    uint8_t chip_select_pin,
    const rtd_acquire_spi_settings_t *requested_settings
)
{
    uint8_t arduino_bit_order;
    uint8_t deasserted_level;

    if (context == NULL || spi == NULL || bus == NULL
        || requested_settings == NULL) {
        return false;
    }
    if (requested_settings->clock_polarity > 1U
        || requested_settings->clock_phase > 1U
        || requested_settings->clock_frequency_hz == 0U
        || requested_settings->bits_per_word != 8U) {
        return false;
    }
    if (requested_settings->bit_order == RTD_ACQUIRE_SPI_MSB_FIRST) {
        arduino_bit_order = MSBFIRST;
    } else if (requested_settings->bit_order == RTD_ACQUIRE_SPI_LSB_FIRST) {
        arduino_bit_order = LSBFIRST;
    } else {
        return false;
    }

    context->bus = bus;
    context->chip_select_pin = chip_select_pin;
    context->requested_clock_frequency_hz =
        requested_settings->clock_frequency_hz;
    context->arduino_bit_order = arduino_bit_order;
    context->arduino_data_mode = rtd_acquire_arduino_avr_data_mode(
        requested_settings->clock_polarity,
        requested_settings->clock_phase
    );
    context->chip_select_active_low = requested_settings->chip_select_active_low;

    bus->begin();
    deasserted_level = context->chip_select_active_low ? HIGH : LOW;
    digitalWrite(chip_select_pin, deasserted_level);
    pinMode(chip_select_pin, OUTPUT);

    spi->context = context;
    spi->settings = *requested_settings;
    spi->settings.clock_frequency_hz =
        rtd_acquire_arduino_avr_effective_clock(
            requested_settings->clock_frequency_hz
        );
    spi->transfer = rtd_acquire_arduino_avr_transfer;

    return true;
}

void rtd_acquire_arduino_avr_spi_end(
    rtd_acquire_arduino_avr_spi_context_t *context
)
{
    uint8_t deasserted_level;

    if (context == NULL || context->bus == NULL) {
        return;
    }

    deasserted_level = context->chip_select_active_low ? HIGH : LOW;
    digitalWrite(context->chip_select_pin, deasserted_level);
    context->bus->end();
    context->bus = NULL;
}

bool rtd_acquire_arduino_avr_delay_init(rtd_acquire_delay_t *delay_hal)
{
    if (delay_hal == NULL) {
        return false;
    }

    delay_hal->context = NULL;
    delay_hal->delay_us = rtd_acquire_arduino_avr_delay_us;
    return true;
}
