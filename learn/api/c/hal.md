---
title: Portable C HAL interfaces
---

# Portable C HAL interfaces

The portable C core depends on small capability-specific interfaces instead of
one platform object. Platform adapters supply these capabilities while device
code remains independent of Arduino, HERO, Linux, or any MCU vendor SDK.

## `rtd_acquire_spi_t`

**Introduced in:** `rtd-acquire 0.2.0`

Declared in `c/include/rtd_acquire/spi.h`:

```c
typedef struct {
    void *context;
    rtd_acquire_spi_settings_t settings;
    rtd_acquire_spi_transfer_fn transfer;
} rtd_acquire_spi_t;
```

The caller owns the opaque `context` and any storage it refers to. `settings`
records the effective connection settings, and `transfer` performs one complete
full-duplex transaction.

The transfer callback owns chip-select assertion and deassertion for the whole
transaction. A platform adapter can implement that with hardware chip select or
GPIO internally; portable device code does not need a separate chip-select API.

SPI transport success/failure is a platform-operation result. It is separate
from normalized acquisition diagnostics reported by the measurement device.

### `rtd_acquire_spi_settings_t`

```c
typedef struct {
    uint8_t clock_polarity;
    uint8_t clock_phase;
    uint32_t clock_frequency_hz;
    rtd_acquire_spi_bit_order_t bit_order;
    uint8_t bits_per_word;
    bool chip_select_active_low;
} rtd_acquire_spi_settings_t;
```

These fields correspond to the observable Python `SpiSettings` semantics.

### `rtd_acquire_spi_result_t`

```c
typedef enum {
    RTD_ACQUIRE_SPI_OK = 0,
    RTD_ACQUIRE_SPI_IO_ERROR = 1
} rtd_acquire_spi_result_t;
```

## `rtd_acquire_delay_t`

**Introduced in:** `rtd-acquire 0.2.0`

Declared in `c/include/rtd_acquire/delay.h`:

```c
typedef struct {
    void *context;
    rtd_acquire_delay_us_fn delay_us;
} rtd_acquire_delay_t;
```

The caller owns the opaque `context`. The callback provides a blocking relative
delay in whole microseconds:

```c
typedef rtd_acquire_delay_result_t (*rtd_acquire_delay_us_fn)(
    void *context,
    uint32_t duration_us
);
```

A successful callback must not return before the requested interval has elapsed.
Oversleep is allowed. This is intentionally a small blocking-delay capability,
not a clock, scheduler, timer-allocation, or Arduino `delay()` abstraction.

Delay-operation success/failure remains separate from device-reported
`Measurement` diagnostics.

### `rtd_acquire_delay_result_t`

```c
typedef enum {
    RTD_ACQUIRE_DELAY_OK = 0,
    RTD_ACQUIRE_DELAY_ERROR = 1
} rtd_acquire_delay_result_t;
```

## Ownership and allocation

Neither HAL requires heap allocation. The interface structures can live in
caller-owned static, automatic, or otherwise fixed storage, and their context
pointers refer to storage whose lifetime is managed by the platform adapter or
application.
