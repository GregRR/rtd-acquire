# Conformance data

This directory contains language-neutral deterministic vectors used to compare
independent `rtd-acquire` implementations.

The first format is versioned under `conformance/v1/`. Vector sets describe
public configuration plus operation-specific native inputs and expected behavior,
including normalized measurement decoding and hardware-register encoding. They
intentionally do not encode a language-specific API shape.

Numeric expected values are reference values. Comparison tolerances belong to
named conformance profiles rather than individual vectors so implementations can
share fixtures without pretending every floating-point representation is
bit-for-bit identical.

Version 1 currently freezes the `python-binary64-c-binary32` profile in
`v1/numeric_profiles.json`. Exact semantic fields remain exact; the profile only
relaxes explicitly identified floating-point outputs.

See [`v1/README.md`](v1/README.md) for the version-1 contract.
