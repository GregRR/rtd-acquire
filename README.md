# rtd-acquire

`rtd-acquire` is a hardware-agnostic acquisition layer for resistance
temperature detectors (RTDs). Its job is to obtain the best trustworthy
estimate of an RTD element's resistance from real or simulated acquisition
hardware and report acquisition-level diagnostics.

It intentionally stops at resistance. RTD characteristic interpretation,
resistance-to-temperature conversion, tolerance, model calibration, and
model-level uncertainty belong in `rtd-sensor`.

```text
physical RTD
    ↓
converter / ADC / transmitter / controller
    ↓
rtd-acquire
    ↓
resistance + acquisition diagnostics
    ↓
rtd-sensor
    ↓
temperature / RTD-model interpretation
```

## Initial targets

The first implementation target is the Analog Devices MAX31865, with Python
hardware testing on a Raspberry Pi 4 Model B and portable C hardware testing on
Arduino-compatible HERO boards.

The second planned hardware family is the TI ADS124S08 precision ADC/front end.
Later targets cover industrial resistance inputs, 4–20 mA transmitters,
industrial digital interfaces, and configurable custom acquisition circuits.

## Raspberry Pi Linux SPI

The Python Raspberry Pi path uses the normal Linux `spidev` userspace API, not
direct SoC register access. Install the optional backend dependency with the
`raspberry-pi` extra. In a source checkout using `uv`:

```sh
uv sync --extra raspberry-pi
```

A MAX31865 on SPI0/CE0 can then be wired through the generic Linux adapter:

```python
from rtd_acquire.max31865 import MAX31865, MAX31865Config
from rtd_acquire.transports import LinuxSpidevDevice, SpiSettings

settings = SpiSettings(
    clock_polarity=0,
    clock_phase=1,
    clock_frequency_hz=1_000_000,
)
config = MAX31865Config(
    reference_resistance_ohms=430.0,
    wire_count=3,
    filter_frequency_hz=60,
)

with LinuxSpidevDevice("/dev/spidev0.0", settings) as spi:
    measurement = MAX31865(spi, config).read()
```

SPI must first be enabled in Raspberry Pi OS. The implementation targets the
same Linux interface on Raspberry Pi 4 and 5; physical validation is currently
pending on Pi 4 and has not yet been performed on Pi 5.

See:

- [DESIGN.md](docs/DESIGN.md) — architecture and project contracts
- [ROADMAP.md](docs/ROADMAP.md) — implementation sequence
- [HARDWARE.md](docs/HARDWARE.md) — acquisition hardware catalog
- [DIAGNOSTICS.md](docs/DIAGNOSTICS.md) — native diagnostic survey and normalization research

## Status

Pre-release development. The core measurement/diagnostic contracts, the
platform-independent MAX31865 driver, and the Linux `spidev` adapter are
implemented. Raspberry Pi physical hardware validation is still pending. Public
APIs are not yet released or stable.

## License

Mozilla Public License 2.0 (MPL-2.0), matching `rtd-sensor`.
