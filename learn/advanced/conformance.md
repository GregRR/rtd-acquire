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

A Python implementation and a future C implementation can consume the same
cases and be compared against the same expected semantics.

## What conformance does not prove

Passing deterministic vectors does not by itself prove that physical hardware
has been wired correctly, that a host transport works on real hardware, or that
the analog acquisition circuit meets an accuracy claim. Those require separate
hardware validation.
