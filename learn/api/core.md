# Core contracts

The root `rtd_acquire` package re-exports the core application-facing API.

## `AcquisitionDevice`

**Introduced in:** `rtd-acquire 0.1.0a1`

```python
class AcquisitionDevice(Protocol):
    def read(self) -> Measurement: ...
```

A structural protocol for anything capable of producing RTD resistance
measurements.

## `Measurement`

**Introduced in:** `rtd-acquire 0.1.0a1`

```text
Measurement(
    resistance_ohms: float | None,
    diagnostics: tuple[Diagnostic, ...] = (),
    standard_uncertainty_ohms: float | None = None,
)
```

`status` is a read-only derived property returning `MeasurementStatus`.

### `MeasurementStatus`

**Introduced in:** `rtd-acquire 0.1.0a1`

- `MeasurementStatus.OK` (`"ok"`)
- `MeasurementStatus.WARNING` (`"warning"`)
- `MeasurementStatus.FAULT` (`"fault"`)

## Diagnostics

**Introduced in:** `rtd-acquire 0.1.0a1`

```text
Diagnostic(
    code: DiagnosticCode,
    severity: DiagnosticSeverity,
    native_evidence: tuple[NativeEvidence, ...] = (),
)
```

`Diagnostic.message` returns the canonical message for its code.

### `DiagnosticSeverity`

**Introduced in:** `rtd-acquire 0.1.0a1`

- `WARNING` (`"warning"`)
- `FAULT` (`"fault"`)

### `NativeEvidence`

**Introduced in:** `rtd-acquire 0.1.0a1`

```text
NativeEvidence(
    identifier: str | None = None,
    message: str | None = None,
)
```

At least one non-empty field is required.

### `DiagnosticCode`

**Introduced in:** `rtd-acquire 0.1.0a1`

`DiagnosticCode` is a string enum containing the project's stable normalized
acquisition diagnostic identities. Use `diagnostic_message(code)` to obtain the
canonical human-readable message.

## Exceptions

**Introduced in:** `rtd-acquire 0.1.0a1`

```text
RtdAcquireError
├── ConfigurationError
└── AcquisitionError
```

`ConfigurationError` represents invalid caller configuration. `AcquisitionError`
represents failure of the acquisition operation itself.
