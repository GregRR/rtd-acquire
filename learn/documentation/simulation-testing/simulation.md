# Simulated acquisition

Use `SimulatedAcquisitionDevice` when you want deterministic behavior through
the same `AcquisitionDevice` contract your application uses for real hardware.

```python
from rtd_acquire import Measurement
from rtd_acquire.simulation import SimulatedAcquisitionDevice

simulated = SimulatedAcquisitionDevice(
    [
        Measurement(resistance_ohms=100.0),
        Measurement(
            resistance_ohms=101.0,
            standard_uncertainty_ohms=0.02,
        ),
    ],
    repeat=True,
)
```

Each call to `read()` consumes one scripted entry. With `repeat=True`, the whole
script loops; otherwise reading past the end raises `AcquisitionError`.

## Script an operation failure

```python
from rtd_acquire.simulation import SimulatedAcquisitionFailure

simulated = SimulatedAcquisitionDevice(
    [SimulatedAcquisitionFailure("simulated bus failure")]
)
```

That entry raises `AcquisitionError`, which is different from returning a
fault-status `Measurement`.

## What it does not model

The generic simulator does not know about MAX31865 registers, RTD temperature,
lead resistance, analog settling, or noise. Supply the measurement behavior your
application test needs explicitly.
