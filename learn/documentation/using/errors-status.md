# Errors and status

`rtd-acquire` separates invalid configuration, acquisition-operation failures,
and hardware-reported measurement diagnostics.

## Exceptions

`RtdAcquireError` is the package exception base class.

- `ConfigurationError` — caller configuration is invalid or an optional backend
  required for that configuration is unavailable.
- `AcquisitionError` — the acquisition operation itself could not complete, for
  example because an SPI transfer failed.

```python
from rtd_acquire import AcquisitionError, ConfigurationError

try:
    measurement = device.read()
except ConfigurationError:
    # Fix configuration rather than retrying unchanged.
    raise
except AcquisitionError:
    # Communication/acquisition did not complete.
    raise
```

## Measurement status is not an exception

A successfully completed acquisition can still return warning or fault
evidence. Inspect the returned `Measurement`:

```python
if measurement.status == "fault":
    for diagnostic in measurement.diagnostics:
        print(diagnostic.code, diagnostic.message)
```

A fault measurement has `resistance_ohms is None`. A warning may still carry a
usable resistance because the evidence does not necessarily invalidate the
measurement.
