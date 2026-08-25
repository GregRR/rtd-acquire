# Hardware integration

A hardware driver should depend on the smallest capability it actually needs.
The Python MAX31865, for example, consumes a generic SPI device rather than a
Raspberry Pi object.

**Python transport boundary introduced in:** `rtd-acquire 0.1.0a1`

```text
Linux spidev adapter ─┐
                      ├─> SpiDevice ─> MAX31865 ─> Measurement
other Python adapter ─┘
```

The portable C implementation uses the same architectural rule with small HAL
interfaces rather than Python protocols.

**Portable C HAL boundary introduced in:** `rtd-acquire 0.2.0`

```text
Arduino AVR / HERO adapter ─┐
other C platform ────────────┼─> SPI + delay HALs ─> portable C device driver
host test fakes ─────────────┘
```

The SPI HAL owns complete transactions, including chip-select handling. The
blocking-delay HAL supplies only the relative timing capability the device
needs. This keeps GPIO, clocks, schedulers, and Arduino-specific APIs out of the
portable core unless a future device actually requires them.

### Arduino AVR / HERO adapter

**Introduced in:** `rtd-acquire 0.2.0`

The first concrete embedded adapter now targets the Arduino AVR / UNO-class
core used by the inventr.io HERO. It maps Arduino `SPIClass` transactions and a
caller-selected chip-select GPIO into `rtd_acquire_spi_t`, and maps Arduino
blocking delay functions into `rtd_acquire_delay_t`. Arduino headers remain
confined to the platform adapter.

The adapter reports the effective discrete AVR SPI clock and splits long
microsecond waits across `delay()` and `delayMicroseconds()`. CI compiles its
MAX31865 example for `arduino:avr:uno`; physical HERO/MAX31865 validation is
still tracked separately. See the [Arduino AVR / HERO C API](../../api/c/arduino-avr.md).

This structure separates three concerns:

1. **platform capabilities** — how a particular host performs SPI, delay, or
   another required operation;
2. **device driver** — what transactions, timing, and electrical configuration
   the acquisition device requires; and
3. **measurement contract** — the normalized resistance and diagnostic result
   returned to applications.

Keeping these boundaries separate makes device logic easier to test with fakes
or emulators and easier to reuse on future platforms.

For the current Python hardware path, see [MAX31865](../hardware/max31865.md)
and [Raspberry Pi / Linux SPI](../hardware/raspberry-pi.md). For the C contracts,
see the [Portable C HAL API](../../api/c/hal.md).
