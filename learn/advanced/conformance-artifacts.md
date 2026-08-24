# Conformance artifacts

**Introduced in:** `rtd-acquire 0.1.0a1`

Versioned, language-neutral conformance data lives under `conformance/` in the
repository.

Current version-1 material uses UTF-8 JSON and a manifest that identifies the
device, operation, and vector-set path.

Each vector has a stable identifier, description, public configuration, and
operation-specific expected behavior.

## Numeric expectations

Floating-point values in the vectors are reference values. Numeric acceptance
profiles belong to the runner/implementation context so a Python binary64 path
and a later embedded binary32 path do not have to duplicate the semantic
fixtures.

Exact integer register outcomes and configuration-error outcomes remain exact
semantic requirements where the vector operation defines them that way.

See the repository's
[`conformance/README.md`](https://github.com/GregRR/rtd-acquire/blob/main/conformance/README.md)
and
[`conformance/v1/README.md`](https://github.com/GregRR/rtd-acquire/blob/main/conformance/v1/README.md)
for the canonical tracked format.
