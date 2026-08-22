# Conformance vector format v1

Version 1 uses UTF-8 JSON and is intended to be readable by Python, C-side host
test harnesses, and other future implementations.

`manifest.json` lists the available vector sets. Each vector-set file contains:

- `schema_version` — integer format version; currently `1`;
- `device` — stable device-family identifier;
- `vectors` — ordered deterministic cases.

Each vector contains:

- `id` — unique stable case identifier within the repository;
- `description` — short human-readable purpose;
- `configuration` — public, language-neutral acquisition configuration;
- `native_input` — raw device/register state presented to the driver/emulator;
- `expected` — normalized `Measurement` semantics.

The expected measurement contains:

- `resistance_ohms` — trustworthy resistance or `null` for a fault result;
- `status` — `ok`, `warning`, or `fault`;
- `standard_uncertainty_ohms` — quantified acquisition uncertainty or `null`;
- `diagnostics` — normalized diagnostic expectations.

Each expected diagnostic contains a normalized `code`, a `severity`, and zero
or more `native_evidence` entries. Canonical `rtd-acquire` diagnostic message
text is deliberately omitted because it is derived from `DiagnosticCode` and is
tested by the implementation itself.

Native evidence contains the device/protocol terminology needed to preserve the
original observation. Composite evidence is represented as multiple entries;
combined vendor identifiers must not be invented.

## Numeric comparison

Numeric values in vectors are reference values, not universal bit-for-bit
requirements. A conformance runner selects the numeric acceptance profile. The
initial Python implementation uses binary64 arithmetic; the later embedded C
implementation may use binary32. Acceptance profiles will be frozen before C
cross-language conformance is declared complete.

## MAX31865 native input

The initial MAX31865 vectors use two native fields:

- `rtd_register` — the complete 16-bit value formed by registers `01h` and
  `02h`, including the RTD-LSB fault-summary bit;
- `fault_status_register` — the complete 8-bit Fault Status register `07h`.

The MAX31865 encodes the resistance ratio as a 15-bit value. The resistance
reference value is therefore derived from the upper 15 bits as:

```text
R_RTD = (rtd_register >> 1) / 32768 * R_REF
```

The low fault-summary bit is evidence that one or more native faults were
latched; normalized diagnostics are derived from the specific Fault Status
register bits rather than duplicating the summary bit as another diagnostic.
