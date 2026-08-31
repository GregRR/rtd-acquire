# Compatibility matrix

**Introduced in:** `rtd-acquire 0.3.0`

This page is the human-readable view of the versioned compatibility evidence
stored under `compatibility/v1/`. It summarizes the exact configurations that
have been classified so far without collapsing manufacturer support, electrical
compatibility, and project hardware validation into one `supported` result.

!!! important "Read each evidence column independently"
    `documented_supported` does not mean physically validated, and
    `compatible` does not mean the project has tested the hardware. An unlisted
    device/family/configuration combination is not implicitly compatible or
    incompatible.

## Current MAX31865 classifications

All current records assess a **4-wire** MAX31865 configuration. Other wiring
modes or reference-resistor choices are outside these specific records until
separately classified.

| RTD family | Required resistance envelope | Assessed `RREF` | Manufacturer support | Electrical compatibility | Project validation |
| --- | ---: | ---: | --- | --- | --- |
| Pt100 | 18.52008–390.481125 Ω | 430 Ω | `documented_supported` | `compatible` | `not_validated` |
| Pt500 | 92.6004–1952.405625 Ω | 2 kΩ | `documented_supported` | `compatible` | `not_validated` |
| Pt1000 | 185.2008–3904.81125 Ω | 4.3 kΩ | `documented_supported` | `compatible` | `not_validated` |
| Ni120 North American 6720 | 66.6–380.3099 Ω | 430 Ω | `not_established` | `compatible` | `not_validated` |
| Ni1000 6180 | 695.202595–2891.5625 Ω | 4.3 kΩ | `not_established` | `compatible` | `not_validated` |
| Ni1000 TK5000 | 751.79284–2517.265625 Ω | 4.3 kΩ | `not_established` | `compatible` | `not_validated` |

The platinum manufacturer-support classifications follow the MAX31865 data
sheet's explicit PT100-through-PT1000 scope. The nickel records remain
`not_established` for manufacturer support because generic nickel/resistive-
sensor discussion is not promoted into a named-family vendor claim.

All six listed configurations are electrically `compatible` because the
selected reference resistor and the complete RTD-family resistance envelope
stay within the documented converter limits checked by the compatibility-data
tests. That engineering result is separate from physical testing.

## Evidence states

### Manufacturer support

- `documented_supported` — manufacturer documentation explicitly supports the
  named device/family/configuration claim.
- `documented_unsupported` — manufacturer documentation explicitly rules out
  the claim.
- `not_established` — available manufacturer evidence does not justify either
  positive or negative support.

### Electrical compatibility

- `compatible` — documented limits plus engineering analysis establish that
  the complete assessed configuration fits the acquisition requirements.
- `incompatible` — documented limits or engineering analysis establish a
  conflict.
- `not_assessed` — no electrical conclusion has yet been established.

### Project validation

- `validated` — qualifying reproducible physical evidence exists. A validated
  record must also identify at least one validation depth.
- `not_validated` — qualifying physical evidence has not yet been recorded, and
  validation depths must therefore be empty.

Two validation depths are currently defined: `range_validated` and
`family_hardware_validated`. They describe how far physical evidence reaches;
they do not create a fourth compatibility dimension.

## What is still pending

No MAX31865 record currently claims project hardware validation. The formal
Raspberry Pi/MAX31865, real-RTD/reference-resistance, and Python-versus-portable-C
hardware work remains governed by the
[hardware-validation procedure](hardware-validation.md).

As those tests are completed, compatibility records can gain validation evidence
without changing their independent manufacturer-support or electrical-
compatibility claims.

## Source of truth and provenance

The table above mirrors the tracked version-1 data:

- [`rtd_families.json`](https://github.com/GregRR/rtd-acquire/blob/main/compatibility/v1/rtd_families.json)
  defines the six RTD-family resistance requirements;
- [`evidence_model.json`](https://github.com/GregRR/rtd-acquire/blob/main/compatibility/v1/evidence_model.json)
  defines the evidence vocabulary; and
- [`max31865.json`](https://github.com/GregRR/rtd-acquire/blob/main/compatibility/v1/max31865.json)
  contains the current MAX31865 records and their evidence-source locators.

The RTD-family identities and resistance requirements are pinned to the stable
`rtd-sensor 0.8.0` conformance catalogs, whose hashes were independently
verified against the immutable `v0.8.0` Git tag. The MAX31865 classifications
use the Analog Devices Rev. 3 data sheet as the primary device source.

See [Compatibility data](compatibility-data.md) for the machine-readable format
and versioning rules, [MAX31865](../documentation/hardware/max31865.md) for the
driver-facing view, and the repository's
[technical bibliography](https://github.com/GregRR/rtd-acquire/blob/main/docs/REFERENCES.md)
for the canonical source ledger.
