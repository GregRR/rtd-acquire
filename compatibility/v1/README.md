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

### Upstream pin verification

The `rtd-sensor v0.8.0` catalog pin was independently verified against the
actual Git objects at the immutable `v0.8.0` tag on 2026-08-30. The verified
SHA-256 digests are:

- `conformance/v1/models.json` —
  `7b24f9c5538d090c75e925677347e3b317d4fc6d74d1bb6c9aa885ddc368b3d3`
- `conformance/v1/characteristics.json` —
  `30b2474c511bb057d8440882378d8056035ee8da77dd1417901b9fcb5ca2919b`

Maintainers must repeat this independent check whenever `repository_ref` or
either stored catalog digest changes. `docs/DEVELOPMENT.md` records the
reproducible verification procedure.

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
v1 manifest without changing the format version. Each
`compatibility_record_sets` entry is an object containing exactly one `path` to
a JSON file in the same version directory; manifest-completeness tests require
every versioned JSON data file to be referenced exactly once.
