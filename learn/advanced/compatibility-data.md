# Compatibility data

**Introduced in:** `rtd-acquire 0.3.0`

Versioned machine-readable RTD acquisition requirements and evidence vocabulary
live under `compatibility/` in the repository.

This data answers a different question from conformance vectors. Conformance asks
whether independent implementations produce the same observable behavior.
Compatibility data records what resistance range an RTD family requires and what
evidence exists for a hardware/configuration claim.

For the human-readable status view, see the
[Compatibility matrix](compatibility-matrix.md).

## Version 1

`compatibility/v1/manifest.json` points to the two foundational datasets plus
the first device/configuration record set:

- `rtd_families.json` — the six current `rtd-sensor` built-in parity targets,
  anchored to the stable `rtd-sensor 0.8.0` conformance model/characteristic IDs and source-catalog hashes,
  with their complete characteristic temperature spans and required ideal
  resistance envelopes;
- `evidence_model.json` — independent states for manufacturer support, electrical
  compatibility, and `rtd-acquire` physical validation, plus range and
  family/hardware validation depth; and
- `max31865.json` — six conservative 4-wire MAX31865 reference-network
  classifications, one for each current RTD-family parity target.

The MAX31865 records classify Pt100/Pt500/Pt1000 manufacturer support as
`documented_supported`, while Ni120/Ni1000 6180/Ni1000 TK5000 remain
`not_established` because generic resistance-input capability is not promoted
to a named RTD-family vendor claim. All six assessed configurations are
electrically `compatible` over their full required resistance envelopes and all
remain `not_validated` pending qualifying physical hardware evidence.

For record consistency, `validation_depths` must be empty when project
validation is `not_validated`; a future `validated` record must declare at least
one validation depth. An absent device/family record remains neither an implicit
support nor incompatibility claim.

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
