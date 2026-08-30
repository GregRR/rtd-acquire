# Implementing acquisition devices

**Introduced in:** `rtd-acquire 0.1.0a1`

Application-facing acquisition objects can satisfy the small
`AcquisitionDevice` protocol:

```python
from rtd_acquire import Measurement


class MyDevice:
    def read(self) -> Measurement:
        ...
```

The type does not need to inherit from a framework base class. Structural
typing keeps the boundary lightweight.

## Responsibilities

An acquisition implementation should:

- return resistance only when the result is trustworthy under its documented
  acquisition semantics;
- normalize supported diagnostics without over-interpreting native evidence;
- preserve useful native evidence when available;
- use `AcquisitionError` when the acquisition operation itself cannot complete;
  and
- keep RTD model selection and temperature conversion outside the driver.

## Threading

The base protocol does not require `read()` to be thread-safe or reentrant.
Document stronger guarantees explicitly if your implementation provides them.

## Multi-channel physical hardware

**Multi-channel semantics introduced in:** `rtd-acquire 0.3.0`

One `AcquisitionDevice` represents one logical resistance source. A physical
converter or module with several channels may keep shared transport, locking,
and device-wide configuration below several channel-scoped views:

```text
physical backend / shared transport
    ├── channel 0 view → AcquisitionDevice → Measurement
    ├── channel 1 view → AcquisitionDevice → Measurement
    └── channel N view → AcquisitionDevice → Measurement
```

Keep application identity outside the measurement contract. Physical location,
equipment names, control-loop roles, and RTD-model selection do not belong in
`Measurement` merely because multiple inputs share one device.

A multi-channel backend also does not automatically require a generic batch or
simultaneous-read API. Add coordinated sampling only when a concrete device has
semantics that justify such a contract.
