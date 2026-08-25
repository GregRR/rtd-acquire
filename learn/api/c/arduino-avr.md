---
title: Arduino AVR / HERO adapter
---

# Arduino AVR / HERO adapter

**Introduced in:** `rtd-acquire 0.2.0`

The first concrete embedded adapter binds the portable C HALs to the Arduino AVR
core used by UNO R3-class boards, including the inventr.io HERO board.

The adapter lives under `c/platform/arduino_avr/`. It is C++ because Arduino's
`SPIClass` and `SPISettings` interfaces are C++, while the acquisition core it
feeds remains portable C11.

## SPI adapter

Include the Arduino-friendly umbrella header:

```cpp
#include <RtdAcquire.h>
```

Caller-owned adapter state is stored in:

```cpp
rtd_acquire_arduino_avr_spi_context_t
```

Initialize it with:

```cpp
bool rtd_acquire_arduino_avr_spi_init(
    rtd_acquire_arduino_avr_spi_context_t *context,
    rtd_acquire_spi_t *spi,
    SPIClass *bus,
    uint8_t chip_select_pin,
    const rtd_acquire_spi_settings_t *requested_settings
);
```

Initialization:

- retains a non-owning pointer to the supplied `SPIClass`;
- calls `SPI.begin()`;
- configures the caller-selected chip-select GPIO as an output in its
  deasserted state;
- maps CPOL/CPHA to Arduino SPI modes;
- maps MSB/LSB bit order;
- requires 8-bit SPI words; and
- populates `rtd_acquire_spi_t` with the **effective** AVR SPI clock rather than
  blindly repeating the requested clock.

Arduino AVR chooses among discrete SPI divisors. For example, a 5 MHz request
on a 16 MHz UNO/HERO becomes an effective 4 MHz clock. The portable MAX31865
driver therefore validates the clock the hardware will actually use.

Each HAL transfer performs one `beginTransaction()`/`endTransaction()` pair and
asserts the selected chip-select GPIO for the complete byte sequence. The
adapter supports active-low and active-high chip select, although the MAX31865
contract requires active-low.

Arduino's byte `SPI.transfer()` API does not provide a transport-error return.
Once adapter arguments are valid, a completed Arduino transaction therefore
returns `RTD_ACQUIRE_SPI_OK`; the adapter cannot infer an I/O failure that the
platform API itself does not expose.

Call:

```cpp
void rtd_acquire_arduino_avr_spi_end(
    rtd_acquire_arduino_avr_spi_context_t *context
);
```

when the application wants to release the corresponding `SPI.begin()`
initialization reference.

## Delay adapter

Bind the portable blocking-delay HAL with:

```cpp
bool rtd_acquire_arduino_avr_delay_init(rtd_acquire_delay_t *delay);
```

The portable HAL accepts a `uint32_t` microsecond duration. On AVR,
`delayMicroseconds()` takes the narrower `unsigned int`, so long waits are split
into whole milliseconds through `delay()` plus a sub-millisecond remainder
through `delayMicroseconds()`. This prevents truncation of the MAX31865's
conversion and settling waits.

Arduino's delay APIs do not return a runtime error, so a valid adapter call maps
to `RTD_ACQUIRE_DELAY_OK`.

## Example and validation

`c/platform/arduino_avr/examples/max31865_read/max31865_read.ino` shows the
minimal setup for a caller-owned MAX31865 measurement and the two adapter HALs.
It uses a 1 MHz, mode-1, MSB-first, active-low SPI configuration and a
caller-selected chip-select pin.

The adapter has two software validation layers:

1. a strict host C++11 contract test using minimal Arduino/SPI stubs; and
2. CI compilation of the real example for `arduino:avr:uno` using Arduino AVR
   Boards 1.8.8.

Those checks validate API/toolchain integration. They do **not** mark the
separate physical HERO + MAX31865 + RTD/reference-resistor validation item as
complete.

## C++ convenience layer

The 0.2 API intentionally stops at the Arduino AVR HAL adapter plus the portable
C MAX31865 interface. A separate C++ object wrapper was evaluated and deferred
until physical-hardware or user feedback demonstrates a concrete usability
benefit. If added later, it should remain a thin delegate to the portable C
driver rather than becoming a second acquisition implementation.
