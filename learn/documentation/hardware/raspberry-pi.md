# Raspberry Pi / Linux SPI

**Introduced in:** `rtd-acquire 0.1.0a1`

The first Python host adapter uses Linux `spidev` rather than direct Raspberry
Pi peripheral-register access.

This is intentionally a **Linux adapter**, not a BCM2711- or RP1-specific
implementation. Raspberry Pi 4 and Raspberry Pi 5 reach the driver through the
same userspace `/dev/spidev*` contract.

## Install the optional backend

```sh
python -m pip install "rtd-acquire[raspberry-pi]"
```

## Configure the SPI connection

```python
from rtd_acquire.transports import LinuxSpidevDevice, SpiSettings

settings = SpiSettings(
    clock_polarity=0,
    clock_phase=1,
    clock_frequency_hz=1_000_000,
)

with LinuxSpidevDevice("/dev/spidev0.0", settings) as spi:
    # Pass `spi` to an acquisition driver such as MAX31865.
    ...
```

SPI must be enabled in the operating system before the device node is
available. The adapter accepts the full path instead of assuming a fixed bus or
chip-select number.

## Support versus validation

The Linux architecture is intended to work on compatible `spidev` hosts. That
is not the same as claiming every host has been physically validated.

For `0.2.0`:

- Raspberry Pi 4: implementation present; formal project hardware validation
  pending;
- Raspberry Pi 5: same Linux architecture, but explicitly unvalidated on
  physical Pi 5 hardware; and
- other compatible Linux systems: architecturally usable, not automatically
  project-validated.
