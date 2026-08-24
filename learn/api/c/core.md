---
title: Portable C core result contracts
---

# Portable C core result contracts

The portable C core result API is declared in
`c/include/rtd_acquire/core.h` and implemented by `c/src/core.c`.

## Measurement and diagnostic storage

**Introduced in:** `rtd-acquire 0.2.0`

The C implementation uses caller-owned storage rather than mandatory heap
allocation or a library-wide fixed diagnostic maximum.

```c
rtd_acquire_diagnostic_t diagnostics[6];
rtd_acquire_native_evidence_t evidence[6];
rtd_acquire_measurement_t measurement;

rtd_acquire_measurement_init(
    &measurement,
    diagnostics,
    6U,
    evidence,
    6U
);
```

The capacities are chosen by the caller. A future device driver returns an
execution/storage error instead of writing beyond those capacities.

### `rtd_acquire_real_t`

```c
typedef float rtd_acquire_real_t;
```

Resistance and standard uncertainty use this portable C scalar. C `float` is
chosen for the embedded-oriented core. The semantic result contract does not
require one floating representation; the version-1 cross-language conformance
suite separately freezes a Python-binary64/C-binary32 numeric profile for
binary32 targets.

### `rtd_acquire_measurement_t`

The measurement stores:

- `has_resistance` plus `resistance_ohms`;
- `has_standard_uncertainty` plus `standard_uncertainty_ohms`;
- caller-owned `diagnostics` with used count and capacity; and
- caller-owned `native_evidence` with used count and capacity.

Explicit presence flags are used instead of `NaN`, infinity, or another magic
value to represent absence.

`rtd_acquire_measurement_reset()` clears the result fields and used counts while
preserving the bound storage pointers and capacities. The reset state is mutable
assembly storage, not a completed valid measurement.

### `rtd_acquire_measurement_status_t`

- `RTD_ACQUIRE_MEASUREMENT_STATUS_OK`
- `RTD_ACQUIRE_MEASUREMENT_STATUS_WARNING`
- `RTD_ACQUIRE_MEASUREMENT_STATUS_FAULT`

`rtd_acquire_measurement_status()` derives status from the diagnostics. It is
not independently stored in the result. A null pointer is not a valid completed
measurement; the function nevertheless returns `OK` for null as a defensive
fallback so accidental null input does not cause undefined behavior. Callers
must use valid initialized measurement objects for semantic status decisions.

### `rtd_acquire_measurement_is_valid()`

The validator checks the shared Python/C result invariants, including:

- finite, non-negative resistance and uncertainty when present;
- diagnostic/evidence counts within caller capacities;
- valid diagnostic codes and severities;
- at most one diagnostic for each normalized code;
- native-evidence ranges contained within the measurement evidence storage;
- at least one non-whitespace identifier or message for each used evidence item;
- no resistance or uncertainty when any fault diagnostic is present; and
- a fault diagnostic whenever no trustworthy resistance is present.

A driver should expose only a valid completed result. Platform, I/O, delay, and
insufficient-storage failures remain separate execution errors rather than being
encoded as measurement diagnostics.

## Diagnostics

### `rtd_acquire_diagnostic_severity_t`

- `RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_WARNING`
- `RTD_ACQUIRE_DIAGNOSTIC_SEVERITY_FAULT`

### `rtd_acquire_diagnostic_code_t`

The enum mirrors the normalized `DiagnosticCode` vocabulary frozen by the
Python contract. New normalized conditions may be added later, but existing
codes are not redefined to absorb unrelated semantics. The numeric C enum value
is an implementation/API value; language-neutral conformance data continues to
identify codes by their stable string names.

### `rtd_acquire_native_evidence_t`

```c
typedef struct {
    const char *identifier;
    const char *message;
} rtd_acquire_native_evidence_t;
```

At least one field must contain non-whitespace text. The pointers are non-owning:
the referenced text must remain alive while the measurement is consumed.

### `rtd_acquire_diagnostic_t`

```c
typedef struct {
    rtd_acquire_diagnostic_code_t code;
    rtd_acquire_diagnostic_severity_t severity;
    size_t native_evidence_offset;
    size_t native_evidence_count;
} rtd_acquire_diagnostic_t;
```

Each diagnostic references a contiguous range in the measurement's shared
native-evidence array. This preserves zero, one, or multiple native observations
without nested allocation or invented combined vendor identifiers.
