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

Python and C consume the same threshold-encoding cases and are compared against
the same exact register outputs. Measurement-decode vectors still execute only
against Python until the C native decoder is implemented.

## Portable C threshold conformance

**Introduced in:** `rtd-acquire 0.2.0`

The threshold vector family is the first active cross-language conformance
gate. It covers directional rounding, zero-ohm low thresholds, the highest
representable high threshold, and rejection of the unrepresentable top band.
Because the observable outputs are exact 16-bit register values, this family
does not need the later binary64/binary32 resistance tolerance profile.

## What conformance does not prove

Passing deterministic vectors does not by itself prove that physical hardware
has been wired correctly, that a host transport works on real hardware, or that
the analog acquisition circuit meets an accuracy claim. Those require separate
hardware validation.
