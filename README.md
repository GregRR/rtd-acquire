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

## Installation

`rtd-acquire` requires Python 3.11 or later. While the project is in alpha,
install the latest prerelease explicitly:

```sh
python -m pip install --pre rtd-acquire
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
python -m pip install --pre "rtd-acquire[raspberry-pi]"
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

`0.1.0a1` is the first public alpha release. The core measurement/diagnostic
contracts, the platform-independent MAX31865 driver, Linux `spidev` adapter,
simulation/emulation support, and shared conformance vectors are implemented.
Raspberry Pi/MAX31865 physical hardware validation is still pending, and public
APIs may change before `0.1.0`.

## License

Mozilla Public License 2.0 (MPL-2.0), matching `rtd-sensor`.
