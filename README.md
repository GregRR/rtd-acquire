# rtd-acquire

[![PyPI version](https://img.shields.io/pypi/v/rtd-acquire.svg)](https://pypi.org/project/rtd-acquire/)
[![Python versions](https://img.shields.io/pypi/pyversions/rtd-acquire.svg)](https://pypi.org/project/rtd-acquire/)
[![CI](https://github.com/GregRR/rtd-acquire/actions/workflows/ci.yml/badge.svg)](https://github.com/GregRR/rtd-acquire/actions/workflows/ci.yml)
[![License](https://img.shields.io/pypi/l/rtd-acquire.svg)](LICENSE)
[![Documentation](https://img.shields.io/badge/docs-GitHub%20Pages-blue)](https://gregrr.github.io/rtd-acquire/)

`rtd-acquire` is a hardware-agnostic acquisition layer for resistance
temperature detectors (RTDs). Its job is to obtain the best trustworthy
estimate of an RTD element's resistance from real or simulated acquisition
hardware and report acquisition-level diagnostics.

It intentionally stops at resistance. RTD characteristic interpretation,
resistance-to-temperature conversion, tolerance, model calibration, and
model-level uncertainty belong in
[`rtd-sensor`](https://github.com/GregRR/rtd-sensor).

```text
physical RTD
    ↓
resistance-measurement path
    ├── raw converter / ADC / electrical observations
    │       ↓
    │   rtd-acquire
    │       ↓
    │   resistance + acquisition diagnostics
    │
    └── instrument / RTD interface already reports resistance
            ↓
        resistance
            ↓
        rtd-sensor
            ↓
        temperature / RTD-model interpretation
```

## When do I need rtd-acquire?

Use `rtd-acquire` when hardware still needs acquisition work before it can
produce a trustworthy estimate of RTD-element resistance. Examples include raw
converter or ADC data, reference/excitation/scaling calculations, wiring or
lead compensation, acquisition calibration, and device-native diagnostics.

You may not need `rtd-acquire` when an instrument, RTD interface, DAQ, or other
system already provides the desired RTD-element resistance in ohms. That
resistance can be passed directly to `rtd-sensor` or another model layer.

A device that exposes only internally calculated temperature is different. It
has already crossed the RTD-model interpretation boundary and is not a normal
`rtd-acquire` resistance backend unless a sufficiently direct resistance or
electrical-observation interface is also available.

## Initial targets

The first implementation target is the Analog Devices MAX31865, with Python
hardware testing on a Raspberry Pi 4 Model B and portable C hardware testing on
Arduino-compatible HERO boards.

The second planned hardware family is the TI ADS124S08 precision ADC/front end.
The TI ADS1220 is a later lower-cost precision-ADC candidate with overlapping
RTD-acquisition concerns; it does not displace the ADS124S08 milestone. Later
targets cover industrial resistance inputs, 4–20 mA transmitters, industrial
digital interfaces, and configurable custom acquisition circuits.

## Installation

`rtd-acquire` requires Python 3.11 or later and is published on
[PyPI](https://pypi.org/project/rtd-acquire/):

```sh
python -m pip install rtd-acquire
```

A minimal hardware-free acquisition uses the deterministic simulator:

```python
from rtd_acquire import Measurement
from rtd_acquire.simulation import SimulatedAcquisitionDevice

device = SimulatedAcquisitionDevice([Measurement(resistance_ohms=100.0)])
measurement = device.read()
print(measurement.resistance_ohms)
```

For Raspberry Pi/Linux SPI support, install the optional backend dependency:

```sh
python -m pip install "rtd-acquire[raspberry-pi]"
```

Developers working from a source checkout can instead use `uv sync`; see
[`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md) for the project quality gates.

## Raspberry Pi Linux SPI

The Python Raspberry Pi path uses the normal Linux `spidev` userspace API, not
direct SoC register access. Install the `raspberry-pi` extra shown above before
using this backend. In a development checkout, the equivalent command is
`uv sync --extra raspberry-pi`.

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

## Deterministic simulation

Applications can exercise the same `AcquisitionDevice` contract without
hardware by replaying validated measurements and explicit acquisition failures:

```python
from rtd_acquire import Measurement
from rtd_acquire.simulation import SimulatedAcquisitionDevice

simulated = SimulatedAcquisitionDevice(
    [
        Measurement(resistance_ohms=100.0),
        Measurement(resistance_ohms=101.0, standard_uncertainty_ohms=0.02),
    ],
    repeat=True,
)

measurement = simulated.read()
```

This generic simulator works at the measurement boundary. The separate
`MAX31865SpiEmulator` exercises MAX31865 register/SPI behavior through the real
driver. Neither simulator performs RTD temperature-model interpretation.

## Integration with rtd-sensor

`rtd-acquire` and `rtd-sensor` remain independent packages. Applications pass
the acquired resistance explicitly into the desired RTD model:

```python
from rtd_sensor import pt100

measurement = device.read()
if measurement.resistance_ohms is not None:
    temperature_c = pt100.resistance_to_celsius(measurement.resistance_ohms)
```

See `examples/rtd_sensor_pt100.py` for a runnable hardware-free example.
`rtd-sensor` is not an `rtd-acquire` runtime dependency.

Physical MAX31865 validation is tracked separately in
`docs/HARDWARE_VALIDATION.md`.

See:

- [DESIGN.md](docs/DESIGN.md) — architecture and project contracts
- [ROADMAP.md](docs/ROADMAP.md) — implementation sequence
- [HARDWARE.md](docs/HARDWARE.md) — acquisition hardware catalog
- [DIAGNOSTICS.md](docs/DIAGNOSTICS.md) — native diagnostic survey and normalization research
- [HARDWARE_VALIDATION.md](docs/HARDWARE_VALIDATION.md) — physical validation gate
- [REFERENCES.md](docs/REFERENCES.md) — external technical bibliography
- [DEVELOPMENT.md](docs/DEVELOPMENT.md) — development and release automation gates
- [CHANGELOG.md](docs/CHANGELOG.md) — release history

## Status

`0.2.0` adds the independent portable C11 implementation, fault-checked
MAX31865 acquisition, shared Python/C conformance with an explicit binary32
numeric profile, and an Arduino AVR / HERO platform adapter to the Python
acquisition stack introduced in `0.1.0a1`. The portable C sources and adapter
ship in the source distribution; the Python wheel remains Python-only.

Physical Raspberry Pi/MAX31865 and HERO/MAX31865 validation is still pending.
`rtd-acquire` remains pre-1.0, and public APIs may change before `1.0`.

## License

Mozilla Public License 2.0 (MPL-2.0), matching `rtd-sensor`.
