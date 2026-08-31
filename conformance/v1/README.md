# Conformance vector format v1

Version 1 uses UTF-8 JSON and is intended to be readable by Python, C-side host
test harnesses, and other future implementations.

`manifest.json` lists the available vector sets and named numeric profiles.
Each vector-set entry identifies a device, an `operation`, and the vector-set
path. Each vector-set file contains:

- `schema_version` — integer format version; currently `1`;
- `device` — stable device-family identifier;
- `operation` — the language-neutral behavior exercised by that vector set;
- `vectors` — ordered deterministic cases.

Every vector contains a stable `id`, a short `description`, public
`configuration`, and operation-specific expected behavior. Version 1 currently
defines two MAX31865 operations:

- `measurement_decode` — raw device/register state to normalized `Measurement`;
- `threshold_encoding` — public threshold configuration to native threshold
  registers, including configuration rejection when directional rounding cannot
  be preserved.

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

Floating-point values in vectors are reference values, not universal bit-for-bit
requirements. A conformance runner selects a named numeric acceptance profile.
Version 1 freezes `python-binary64-c-binary32` in `numeric_profiles.json`.

That profile requires Python radix-2 `float` with a 53-bit significand and C
radix-2 `float` with a 24-bit significand and the binary32 exponent range. For
MAX31865 `measurement_decode`, expected zero resistance remains exact. Nonzero
resistance uses a relative tolerance of `2^-22`
(`2.384185791015625e-7`) with zero absolute tolerance. Status,
resistance presence/absence, diagnostics, native evidence, integer fields,
threshold-register outcomes, and configuration-error outcomes remain exact.

The tolerance is computational only; it is not a hardware-accuracy or
measurement-uncertainty allowance. Cross-language vector configurations must
also be validation-stable when their floating values are converted to binary32.
A vector must not depend on a distinction that Python binary64 can represent but
C binary32 cannot.

## MAX31865 measurement-decode vectors

The MAX31865 `measurement_decode` vectors use two native fields:

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

The vector set includes a non-binary32-exact reference-resistance case so the
numeric profile is exercised by the cross-language gate instead of being
specified only in prose.

**High-scale coverage added in:** `rtd-acquire 0.3.0`

The measurement-decode set also includes a 4.3 kΩ reference-network case near
the upper Pt1000 full-characteristic resistance envelope. This exercises the
same ratiometric decode path at a materially larger resistance scale without
turning the conformance fixture into an RTD temperature-model test.

## MAX31865 threshold-encoding vectors

The MAX31865 `threshold_encoding` vectors pin the language-neutral conversion
from ohm-valued public thresholds to the converter's 16-bit threshold-register
values. Low thresholds round downward and high thresholds round upward so
quantization does not move a native threshold inside the caller's requested
diagnostic-free window.

A high threshold in the final unrepresentable band below `R_REF` cannot preserve
that upward-rounding guarantee because the 15-bit threshold code has no value
above 32767. Such a configuration has the expected outcome
`configuration_error` rather than being silently clamped downward.

Register outcomes use the complete 16-bit register value, including the unused
least-significant bit position, so implementations do not need to infer whether
the fixture contains an ADC code or a wire/register value.

**High-scale coverage added in:** `rtd-acquire 0.3.0`

The threshold set includes a 4.3 kΩ reference-network case using the Pt1000
full-characteristic resistance-envelope endpoints. The expected register words
pin the same directional rounding rules at the higher scale and are executed by
both the Python and portable C conformance runners.
