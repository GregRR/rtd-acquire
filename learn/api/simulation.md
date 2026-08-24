# `rtd_acquire.simulation`

## `SimulatedAcquisitionFailure`

**Introduced in:** `rtd-acquire 0.1.0a1`

```text
SimulatedAcquisitionFailure(
    message: str = "simulated acquisition failure",
)
```

Represents one scripted failure of the acquisition operation. When consumed by
the simulated device, it raises `AcquisitionError`.

## `SimulatedAcquisitionDevice`

**Introduced in:** `rtd-acquire 0.1.0a1`

```text
SimulatedAcquisitionDevice(
    steps: Iterable[Measurement | SimulatedAcquisitionFailure],
    *,
    repeat: bool = False,
)
```

The script must contain at least one step.

### `read()`

Consumes and returns the next `Measurement`, or raises `AcquisitionError` for a
`SimulatedAcquisitionFailure` entry. If a non-repeating script is exhausted,
`read()` raises `AcquisitionError`.

### `read_count`

Number of scripted entries consumed since construction or the last reset.

### `reset()`

Restarts the script and resets `read_count` to zero.
