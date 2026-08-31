# Compatibility data format v1

Version 1 uses UTF-8 JSON. It freezes the RTD-family resistance requirements and
the evidence vocabulary used by `rtd-acquire 0.3.0` without importing RTD
conversion equations into acquisition drivers.

`manifest.json` identifies:

- the RTD-family requirement dataset;
- the evidence-model dataset; and
- zero or more device/configuration compatibility record sets.

The initial version intentionally starts with no device record sets. MAX31865
classification is a separate roadmap slice so creating the data format does not
silently promote existing hardware assumptions into compatibility claims.

## RTD-family requirements

`rtd_families.json` contains the six current `rtd-sensor` built-in parity
targets. It anchors those identities to the stable `rtd-sensor 0.8.0` conformance
contract at the `v0.8.0` repository tag, including the source catalog hashes, rather than inventing a second naming system. Each family records:

- the companion project's stable `model_id` and display name;
- the companion `characteristic_id`;
- nominal resistance at 0 °C;
- the complete supported characteristic temperature interval;
- the required ideal resistance envelope for acquisition compatibility; and
- whether the stored envelope bounds are exact or deliberately rounded.

These values are acquisition requirements only. The dataset does not contain
Callendar-Van Dusen coefficients, nickel polynomials, inverse algorithms, or any
other resistance-to-temperature model logic.

## Evidence model

`evidence_model.json` keeps three claim dimensions independent:

- `manufacturer_support` — what vendor documentation explicitly supports;
- `electrical_compatibility` — what documented limits and engineering analysis
  establish for the complete configured acquisition chain; and
- `project_validation` — what reproducible physical testing by this project has
  established.

The manufacturer dimension uses `not_established` when the recorded evidence
does not justify either a positive or explicit negative vendor claim. Electrical
compatibility uses `not_assessed` until engineering analysis establishes an
outcome. Project validation records whether qualifying physical evidence exists
and, when it does, may identify `range_validated` and/or
`family_hardware_validated` depth.

A future device record must identify the exact hardware/configuration being
assessed and retain the evidence described in `docs/DESIGN.md`; the format must
not collapse these three dimensions into a single `supported` Boolean.

## Versioning

Incompatible structural or semantic changes require a new compatibility-data
version. New record sets that follow the existing v1 contract may be added to the
v1 manifest without changing the format version.
