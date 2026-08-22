#ifndef RTD_ACQUIRE_SPI_H
#define RTD_ACQUIRE_SPI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RTD_ACQUIRE_SPI_MSB_FIRST = 0,
    RTD_ACQUIRE_SPI_LSB_FIRST = 1
} rtd_acquire_spi_bit_order_t;

typedef struct {
    uint8_t clock_polarity;
    uint8_t clock_phase;
    uint32_t clock_frequency_hz;
    rtd_acquire_spi_bit_order_t bit_order;
    uint8_t bits_per_word;
    bool chip_select_active_low;
} rtd_acquire_spi_settings_t;

typedef enum {
    RTD_ACQUIRE_SPI_OK = 0,
    RTD_ACQUIRE_SPI_IO_ERROR = 1
} rtd_acquire_spi_result_t;

typedef rtd_acquire_spi_result_t (*rtd_acquire_spi_transfer_fn)(
    void *context,
    const uint8_t *tx,
    uint8_t *rx,
    size_t length
);

typedef struct {
    void *context;
    rtd_acquire_spi_settings_t settings;
    rtd_acquire_spi_transfer_fn transfer;
} rtd_acquire_spi_t;

#ifdef __cplusplus
}
#endif

#endif
