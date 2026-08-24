# Conformance artifacts

**Introduced in:** `rtd-acquire 0.1.0a1`

Versioned, language-neutral conformance data lives under `conformance/` in the
repository.

Current version-1 material uses UTF-8 JSON and a manifest that identifies the
device, operation, vector-set path, and named numeric profiles.

Each vector has a stable identifier, description, public configuration, and
operation-specific expected behavior.

## Numeric expectations

Floating-point values in the vectors are reference values. Numeric acceptance
profiles belong to the runner/implementation context so independent
implementations do not have to duplicate semantic fixtures.

**Binary64/binary32 profile introduced in:** `rtd-acquire 0.2.0`

Version 1 freezes `python-binary64-c-binary32` in
`conformance/v1/numeric_profiles.json`. It gives nonzero MAX31865 decoded
resistance a relative tolerance of `2^-22`, requires expected zero exactly, and
leaves all integer/register, diagnostic, status, presence, and configuration
outcomes exact. Cross-language vector configurations are required to be
binary32-stable at validation boundaries.

See the repository's
[`conformance/README.md`](https://github.com/GregRR/rtd-acquire/blob/main/conformance/README.md)
and
[`conformance/v1/README.md`](https://github.com/GregRR/rtd-acquire/blob/main/conformance/v1/README.md)
for the canonical tracked format.
