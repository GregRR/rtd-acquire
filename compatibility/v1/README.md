# Compatibility data format v1

Version 1 uses UTF-8 JSON. It freezes the RTD-family resistance requirements and
the evidence vocabulary used by `rtd-acquire 0.3.0` without importing RTD
conversion equations into acquisition drivers.

`manifest.json` identifies:

- the RTD-family requirement dataset;
- the evidence-model dataset; and
- zero or more device/configuration compatibility record sets.

The first device record set is `max31865.json`. It classifies one conservative
4-wire reference-network configuration for each current RTD-family parity target.
The records keep manufacturer support, electrical compatibility, and physical
project validation independent; they do not turn software conformance into
hardware validation.

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

A device record identifies the exact hardware/configuration being assessed and
retains the evidence described in `docs/DESIGN.md`; the format must not collapse
these three dimensions into a single `supported` Boolean.

For v1 records, `validation_depths` is structurally coupled to
`project_validation`: it must be empty when the state is `not_validated`, and a
`validated` record must carry at least one declared validation depth. This keeps
a range/family validation label from contradicting the record's top-level
physical-validation state.

## Device record-set structure

Every v1 device record-set file uses the same common top-level shape:

- `schema_version` and a stable `record_set_id`;
- exact `device` identity plus device-specific `device_limits`;
- links to `rtd_families.json` and `evidence_model.json`;
- a `sources` collection used by claim evidence; and
- a `records` collection.

Every compatibility record contains exactly an `id`, `rtd_family`,
`configuration`, the three independent `claims`, `validation_depths`, and
explicit `limitations`. Each claim carries a vocabulary `state`, source IDs, and
a rationale. Device-specific configuration and limit fields may vary by record
set, but the common claim/evidence shape is frozen for v1 and enforced across
all manifest-listed record sets by structural tests.

## MAX31865 record set

`max31865.json` records six 4-wire configurations:

- Pt100 with 430 Ω `RREF`;
- Pt500 with 2 kΩ `RREF`;
- Pt1000 with 4.3 kΩ `RREF`;
- Ni120 6720 with 430 Ω `RREF`;
- Ni1000 6180 with 4.3 kΩ `RREF`; and
- Ni1000 TK5000 with 4.3 kΩ `RREF`.

The Analog Devices Rev. 3 datasheet explicitly documents platinum RTDs from
PT100 through PT1000, 2-/3-/4-wire connections, an `RREF` operating range of
350 Ω to 10 kΩ, and ratiometric resistance acquisition. The platinum records
therefore use `documented_supported`. The datasheet discusses nickel RTDs and
other resistive sensors but does not explicitly name the three assessed nickel
families; those manufacturer-support states remain `not_established` rather
than being promoted from generic resistance-input capability.

Engineering checks establish all six listed configurations as electrically
`compatible`: each selected `RREF` is inside the documented range, each complete
family resistance envelope remains below `RREF`, its upper envelope remains
representable by the 15-bit threshold domain, and the resulting 4-wire bias
current stays inside the documented 0.2–5.75 mA range across the envelope even
when the datasheet's maximum per-lead cable resistance is included for the
minimum-current bound. Every record remains `not_validated` until qualifying
physical converter/family evidence is attached.

## Versioning

Incompatible structural or semantic changes require a new compatibility-data
version. New record sets that follow the existing v1 contract may be added to the
v1 manifest without changing the format version. Each
`compatibility_record_sets` entry is an object containing exactly one `path` to
a JSON file in the same version directory; manifest-completeness tests require
every versioned JSON data file to be referenced exactly once.
