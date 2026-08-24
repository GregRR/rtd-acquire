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
HERO/Arduino adapter ─┐
other C platform ─────┼─> SPI + delay HALs ─> portable C device driver
host test fakes ──────┘
```

The SPI HAL owns complete transactions, including chip-select handling. The
blocking-delay HAL supplies only the relative timing capability the device
needs. This keeps GPIO, clocks, schedulers, and Arduino-specific APIs out of the
portable core unless a future device actually requires them.

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
