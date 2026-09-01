# rtd-acquire physical-validation record

Template version: 1

Complete this record under `.rtd-acquire-local/validation/`. Keep raw captures,
local hardware identifiers, serial output, calculations, and other bench-only
evidence local until a reviewed project summary is ready.

## Scope

- Date:
- Compatibility record ID(s):
- Target evidence depth: `range_validated` / `family_hardware_validated`
- Platform/runtime path:
- `rtd-acquire` commit:
- Git worktree clean (`true` required for final claim-bearing validation):
- Validation operator/organization, if publication is appropriate:

## Predeclared acceptance budget

Complete this section **before examining the final measurement results**.

- Reference-source uncertainty/tolerance:
- MAX31865 `RREF` uncertainty:
- Converter quantization contribution:
- Lead/contact contribution:
- Repeatability criterion:
- Mean-error criterion:
- Cross-platform criterion, if applicable:
- Rationale/reference for the combined budget:

## Hardware and configuration

- Platform model/revision:
- Operating system/toolchain:
- MAX31865 board manufacturer/model/revision:
- Documented or measured `RREF`:
- Wire count/topology:
- Filter frequency:
- SPI device/chip-select and clock:
- Same board/wiring reused across platforms?:
- Relevant board/manual references:

## Resistance references

Record characterized values, uncertainties, connection method, and the family
band each reference is intended to exercise.

| Label | Characterized resistance | Uncertainty/tolerance | Method/instrument | Family band |
| --- | ---: | ---: | --- | --- |
| low |  |  |  | low |
| middle |  |  |  | middle |
| high |  |  |  | high |

## Structured capture artifacts

| Artifact | Purpose | SHA-256 / provenance |
| --- | --- | --- |
|  |  |  |

## Range-validation results

| Band | Reference | Count | Mean | Std. dev. | Min | Max | Signed error | Pass? |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| low |  |  |  |  |  |  |  |  |
| middle |  |  |  |  |  |  |  |  |
| high |  |  |  |  |  |  |  |  |

Record any diagnostics, operation failures, or unexplained behavior here:

## Real-family RTD evidence

Complete only when pursuing `family_hardware_validated`.

- RTD family/model/characteristic:
- Manufacturer/model:
- Probe tolerance/class:
- Independent resistance reference or test environment:
- Measurement count and summary:
- Environmental stability/drift notes:
- Native-fault evidence applicable to this path/topology:

## Platform comparison

Complete when comparing Raspberry Pi/Python with HERO/portable C or another
second physical path.

- Common physical references/configuration:
- Per-platform means and repeatability:
- Inter-platform differences:
- Combined-budget result:
- Diagnostic-semantic comparison:

## Conclusion

- Validation result:
- Supported compatibility-record ID(s):
- Evidence depth justified:
- Claim updates justified:
- Explicitly unvalidated combinations:
- Limitations/open questions:

Do not change manufacturer-support or electrical-compatibility states merely
because physical validation succeeded.
