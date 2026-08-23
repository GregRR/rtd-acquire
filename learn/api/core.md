# Core contracts

The root `rtd_acquire` package re-exports the core application-facing API.

## `AcquisitionDevice`

```python
class AcquisitionDevice(Protocol):
    def read(self) -> Measurement: ...
```

A structural protocol for anything capable of producing RTD resistance
measurements.

## `Measurement`

```text
Measurement(
    resistance_ohms: float | None,
    diagnostics: tuple[Diagnostic, ...] = (),
    standard_uncertainty_ohms: float | None = None,
)
```

`status` is a read-only derived property returning `MeasurementStatus`.

### `MeasurementStatus`

- `MeasurementStatus.OK` (`"ok"`)
- `MeasurementStatus.WARNING` (`"warning"`)
- `MeasurementStatus.FAULT` (`"fault"`)

## Diagnostics

```text
Diagnostic(
    code: DiagnosticCode,
    severity: DiagnosticSeverity,
    native_evidence: tuple[NativeEvidence, ...] = (),
)
```

`Diagnostic.message` returns the canonical message for its code.

### `DiagnosticSeverity`

- `WARNING` (`"warning"`)
- `FAULT` (`"fault"`)

### `NativeEvidence`

```text
NativeEvidence(
    identifier: str | None = None,
    message: str | None = None,
)
```

At least one non-empty field is required.

### `DiagnosticCode`

`DiagnosticCode` is a string enum containing the project's stable normalized
acquisition diagnostic identities. Use `diagnostic_message(code)` to obtain the
canonical human-readable message.

## Exceptions

```text
RtdAcquireError
├── ConfigurationError
└── AcquisitionError
```

`ConfigurationError` represents invalid caller configuration. `AcquisitionError`
represents failure of the acquisition operation itself.
