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
