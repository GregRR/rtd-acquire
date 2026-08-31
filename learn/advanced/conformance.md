# Understanding conformance

**Introduced in:** `rtd-acquire 0.1.0a1`

`rtd-acquire` is intended to have independent Python and portable C
implementations of shared behavior. Conformance data provides a language-neutral
contract between them.

The important idea is that conformance describes **observable behavior**, not
one language's internal code structure.

For the current MAX31865 work, vectors can describe operations such as:

- native measurement/register decoding; and
- resistance-threshold encoding.

Python and C now consume both current MAX31865 vector families and are compared
against the same expected observable results.

## Portable C threshold conformance

**Introduced in:** `rtd-acquire 0.2.0`

The threshold vector family is the first active cross-language conformance
gate. It covers directional rounding, zero-ohm low thresholds, the highest
representable high threshold, and rejection of the unrepresentable top band.
Because the observable outputs are exact 16-bit register values, this family
remains exact and does not use the floating-point resistance tolerance.

## Portable C measurement-decode conformance

**Introduced in:** `rtd-acquire 0.2.0`

The MAX31865 measurement-decode vectors also execute against the independent C
decoder. They compare derived status, resistance presence/value, normalized
diagnostic code and severity, and preserved native evidence.

### Binary64/binary32 numeric profile

**Introduced in:** `rtd-acquire 0.2.0`

The frozen `python-binary64-c-binary32` profile applies a relative tolerance of
`2^-22` (`2.384185791015625e-7`) to nonzero resistance values and requires
expected zero to remain exactly zero. Status, resistance presence/absence,
diagnostics, native evidence, integer fields, and threshold-register outputs
remain exact.

The tolerance is deliberately computational rather than metrological: it
accounts for binary32 representation/arithmetic differences and does not add
sensor accuracy, reference-resistor tolerance, or measurement uncertainty. The
vector set includes a non-binary32-exact reference resistance so this path is
actually exercised.

Cross-language vector configurations must also remain valid or invalid after
conversion to binary32. The profile does not make configuration comparisons
fuzzy merely to preserve distinctions that a binary32 C target cannot represent.

## High-scale MAX31865 coverage

**Introduced in:** `rtd-acquire 0.3.0`

The shared MAX31865 vectors also exercise a 4.3 kΩ reference network. One
measurement-decode case sits at the nearest representable resistance below the
Pt1000 full-characteristic upper envelope, while one threshold case encodes the
Pt1000 low/high envelope endpoints using the existing directional-rounding
contract. Both cases execute against Python and portable C.

This is software conformance evidence for the high-scale configuration. It does
not by itself establish electrical compatibility or physical validation; those
claims are tracked separately.

## What conformance does not prove

Passing deterministic vectors does not by itself prove that physical hardware
has been wired correctly, that a host transport works on real hardware, or that
the analog acquisition circuit meets an accuracy claim. Those require separate
hardware validation.
