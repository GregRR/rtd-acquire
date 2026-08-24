# Measurement results

**Introduced in:** `rtd-acquire 0.1.0a1`

Every successful device read returns a `Measurement`.

```python
from rtd_acquire import Measurement

measurement = Measurement(
    resistance_ohms=109.73,
    standard_uncertainty_ohms=0.02,
)
```

## `resistance_ohms`

This is `rtd-acquire`'s primary result: the acquisition layer's best trustworthy
estimate of the RTD element resistance.

A fault result uses `None` instead of supplying a resistance that the
acquisition system no longer considers trustworthy.

## `status`

`status` is derived from the diagnostics rather than stored independently:

- `ok` — no diagnostics;
- `warning` — one or more warning diagnostics and a usable resistance;
- `fault` — at least one fault diagnostic; resistance is `None`.

This prevents the status and diagnostic evidence from drifting out of sync.

## `diagnostics`

Diagnostics use stable normalized codes while preserving device- or
protocol-native evidence. A measurement contains at most one diagnostic for a
given normalized code.

See [Diagnostics](../measurement-diagnostics/diagnostics.md).

## `standard_uncertainty_ohms`

When an acquisition path can establish a standard uncertainty for its
resistance result, it may report that value in ohms. `None` means the acquisition
layer did not quantify one; it does **not** mean zero uncertainty.

Fault measurements cannot carry resistance uncertainty because they do not
carry a trustworthy resistance value.
