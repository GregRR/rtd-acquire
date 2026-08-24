# Reading devices

**Introduced in:** `rtd-acquire 0.1.0a1`

The common application boundary is intentionally small:

```python
measurement = device.read()
```

Anything satisfying the `AcquisitionDevice` protocol can be used where your
application expects that contract.

```python
from rtd_acquire import AcquisitionDevice, Measurement


def read_resistance(device: AcquisitionDevice) -> float:
    measurement: Measurement = device.read()
    if measurement.resistance_ohms is None:
        raise RuntimeError("acquisition returned a fault measurement")
    return measurement.resistance_ohms
```

## A read is one acquisition operation

The base protocol does not promise thread safety or reentrancy. Unless a
specific device documents stronger behavior, serialize access to a device
instance.

## Device result versus operation failure

A device can successfully communicate with hardware and return a
`Measurement` whose status is `warning` or `fault`. That is different from the
acquisition operation itself failing, which raises `AcquisitionError`.

Keep those two cases separate in application logic; they carry different
information.
