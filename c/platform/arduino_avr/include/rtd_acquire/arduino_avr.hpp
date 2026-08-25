#ifndef RTD_ACQUIRE_ARDUINO_AVR_HPP
#define RTD_ACQUIRE_ARDUINO_AVR_HPP

#include <Arduino.h>
#include <SPI.h>

#include <stdbool.h>
#include <stdint.h>

#include "rtd_acquire/delay.h"
#include "rtd_acquire/spi.h"

/*
 * Caller-owned Arduino AVR SPI adapter state. The SPIClass and chip-select pin
 * are non-owning references to platform resources managed by the application.
 */
typedef struct {
    SPIClass *bus;
    uint8_t chip_select_pin;
    uint32_t requested_clock_frequency_hz;
    uint8_t arduino_bit_order;
    uint8_t arduino_data_mode;
    bool chip_select_active_low;
} rtd_acquire_arduino_avr_spi_context_t;

/*
 * Initialize an Arduino AVR SPI capability and configure its chip-select pin.
 * Only 8-bit SPI words are supported by this adapter. The HAL settings record
 * the effective AVR SPI clock selected by Arduino's discrete clock dividers.
 */
bool rtd_acquire_arduino_avr_spi_init(
    rtd_acquire_arduino_avr_spi_context_t *context,
    rtd_acquire_spi_t *spi,
    SPIClass *bus,
    uint8_t chip_select_pin,
    const rtd_acquire_spi_settings_t *requested_settings
);

/* Release one SPI.begin() reference acquired by the initializer. */
void rtd_acquire_arduino_avr_spi_end(
    rtd_acquire_arduino_avr_spi_context_t *context
);

/* Bind the Arduino blocking delay functions to the portable delay HAL. */
bool rtd_acquire_arduino_avr_delay_init(rtd_acquire_delay_t *delay);

#endif
