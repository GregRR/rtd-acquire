---
title: Start Here
---

# Start Here

This is the quickest route from installation to your first `rtd-acquire`
measurement. No hardware is required.

!!! tip "Want to explore RTD temperature behavior instead?"
    The interactive experiments live in the
    [rtd-sensor RTD Playground](https://gregrr.github.io/rtd-sensor/playground/).
    `rtd-acquire` focuses on obtaining resistance; `rtd-sensor` interprets it.

## 1. Install the alpha

`rtd-acquire` requires Python 3.11 or newer. While the project is in prerelease,
install it with `--pre`:

```sh
python -m pip install --pre rtd-acquire
```

For Raspberry Pi / Linux `spidev` support:

```sh
python -m pip install --pre "rtd-acquire[raspberry-pi]"
```

See [Installation](documentation/using/installation.md) for development-checkout
instructions and the current support boundary.

## 2. Make a hardware-free measurement

```python
from rtd_acquire import Measurement
from rtd_acquire.simulation import SimulatedAcquisitionDevice

sensor = SimulatedAcquisitionDevice(
    [Measurement(resistance_ohms=109.73)]
)

measurement = sensor.read()
print(measurement.resistance_ohms)
print(measurement.status)
```

The device returns a `Measurement`, not a temperature. The important fields are:

- `resistance_ohms` — the trustworthy acquired resistance, or `None` for a
  fault result;
- `status` — derived as `ok`, `warning`, or `fault`;
- `diagnostics` — normalized acquisition diagnostics; and
- `standard_uncertainty_ohms` — optional quantified standard uncertainty in the
  acquired resistance.

## 3. Hand the resistance to rtd-sensor

The packages stay independent. If your application wants a Pt100 temperature,
pass the resistance explicitly to `rtd-sensor`:

```python
from rtd_sensor import pt100

measurement = sensor.read()
if measurement.resistance_ohms is not None:
    temperature_c = pt100.resistance_to_celsius(
        measurement.resistance_ohms
    )
```

Install `rtd-sensor` separately when you need model interpretation.

## 4. Move to real hardware

The first hardware driver is the MAX31865. A typical Raspberry Pi path is:

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

Do not copy the `430.0` reference-resistor value blindly. Configure the actual
reference resistance used by your board.

!!! warning "Physical validation is still pending"
    The Linux/MAX31865 implementation is present in `0.1.0a1`, but the project's
    formal Raspberry Pi 4 + real MAX31865 validation gate has not yet been
    completed. Treat this as alpha hardware support.

## Where to go next

- [Documentation](documentation/index.md) for concepts and task-oriented guides.
- [API Reference](api/index.md) for public Python interfaces.
- [Advanced](advanced/index.md) for conformance, C, and embedded architecture.
- [RTD Playground](https://gregrr.github.io/rtd-sensor/playground/) for
  experiments with RTD models and temperature interpretation.
