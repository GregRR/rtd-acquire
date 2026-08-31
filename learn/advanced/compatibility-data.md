# Compatibility data

**Introduced in:** `rtd-acquire 0.3.0`

Versioned machine-readable RTD acquisition requirements and evidence vocabulary
live under `compatibility/` in the repository.

This data answers a different question from conformance vectors. Conformance asks
whether independent implementations produce the same observable behavior.
Compatibility data records what resistance range an RTD family requires and what
evidence exists for a hardware/configuration claim.

## Version 1

`compatibility/v1/manifest.json` points to two initial datasets:

- `rtd_families.json` — the six current `rtd-sensor` built-in parity targets,
  anchored to the stable `rtd-sensor 0.8.0` conformance model/characteristic IDs and source-catalog hashes,
  with their complete characteristic temperature spans and required ideal
  resistance envelopes;
- `evidence_model.json` — independent states for manufacturer support, electrical
  compatibility, and `rtd-acquire` physical validation, plus range and
  family/hardware validation depth.

The initial manifest deliberately contains no device/configuration record sets.
Those records are added only after an explicit hardware-family classification
slice; an absent record is not a compatibility claim.

## What the data does not contain

The family requirements do not copy RTD equations, coefficients, or inverse
algorithms from `rtd-sensor`. Acquisition drivers therefore remain model-neutral.

The data also does not turn a successful software test into physical validation.
Real-hardware evidence remains subject to the project's separate
[hardware-validation](hardware-validation.md) procedures.

See the repository's
[`compatibility/README.md`](https://github.com/GregRR/rtd-acquire/blob/main/compatibility/README.md)
and
[`compatibility/v1/README.md`](https://github.com/GregRR/rtd-acquire/blob/main/compatibility/v1/README.md)
for the canonical tracked format.
