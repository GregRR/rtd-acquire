# Hardware integration

A hardware driver should depend on the smallest capability it actually needs.
The MAX31865, for example, consumes a generic SPI device rather than a Raspberry
Pi object.

```text
Linux spidev adapter ─┐
                      ├─> SpiDevice ─> MAX31865 ─> Measurement
future MCU adapter ───┘
```

This structure separates three concerns:

1. **platform transport** — how bytes move on a particular host;
2. **device driver** — what transactions and electrical configuration the
   acquisition device requires; and
3. **measurement contract** — the normalized resistance and diagnostic result
   returned to applications.

Keeping these boundaries separate makes the same driver easier to test with an
emulator and easier to reuse on future platforms.

For the current concrete path, see [MAX31865](../hardware/max31865.md) and
[Raspberry Pi / Linux SPI](../hardware/raspberry-pi.md).
