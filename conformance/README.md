# Conformance data

This directory contains language-neutral deterministic vectors used to compare
independent `rtd-acquire` implementations.

The first format is versioned under `conformance/v1/`. The vector files describe
public configuration, raw/native device state, and the expected normalized
measurement semantics. They intentionally do not encode a language-specific API
shape.

Numeric expected values are reference values. Comparison tolerances belong to
the conformance runner/profile, not to individual vectors, so Python binary64
and embedded binary32 implementations can use appropriate acceptance policies
without duplicating fixtures.

See [`v1/README.md`](v1/README.md) for the version-1 contract.
