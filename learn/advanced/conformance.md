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
does not need the later binary64/binary32 resistance tolerance profile.

## Portable C measurement-decode conformance

**Introduced in:** `rtd-acquire 0.2.0`

The MAX31865 measurement-decode vectors also execute against the independent C
decoder. They compare derived status, resistance presence/value, normalized
diagnostic code and severity, and preserved native evidence. The current seed
resistance values (107.5 ohms, 53.75 ohms, and 0 ohms) are exactly representable
in binary32, so this gate can be exact without defining the broader floating-
point tolerance policy. Non-exact binary64/binary32 cases remain blocked on the
separate 0.2 numeric acceptance profile.

## What conformance does not prove

Passing deterministic vectors does not by itself prove that physical hardware
has been wired correctly, that a host transport works on real hardware, or that
the analog acquisition circuit meets an accuracy claim. Those require separate
hardware validation.
