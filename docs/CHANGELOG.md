# Changelog

All notable changes to this project will be documented in this file.

The project has not yet made a public release.

## Unreleased

- Freeze the first normalized diagnostic object contract around `Diagnostic`,
  `DiagnosticSeverity`, and composite-capable `NativeEvidence`.
- Derive canonical diagnostic messages from `DiagnosticCode` instead of storing
  per-driver normalized message text.
- Freeze the first evidence-based `DiagnosticCode` vocabulary while keeping
  future device-specific additions additive.
- Move design, roadmap, hardware, diagnostics, and changelog files under `docs/`.

- Added a non-frozen candidate diagnostic vocabulary worksheet with normalized
  wording, broad/specific sibling concepts, and explicit exclusions for mere
  state/facility observations.
- Added a candidate-maturity review that separates strong first-enum concepts
  from provisional names that still need targeted cross-vendor review.
- Refined MAX31865 working diagnostic names to describe D5:D3 as explicit
  above/below-threshold comparisons.
- Recorded that SITRANS TH320/TH420 diagnostic specificity can differ by
  interface/configuration: HART input errors collapse broken/shorted faults,
  while configured analog fault currents can preserve the distinction.
- Confirmed from WIKA material that HART exposes problem status plus associated
  error messages, while exact T32 per-condition DD identifiers remain open.
- Expanded the candidate vocabulary with AD7124-specific input-voltage, supply,
  SPI-operation, and memory-integrity diagnostics plus provisional industrial
  redundancy/internal-reference concepts.
- Recorded composite native evidence as an unresolved `Diagnostic` contract
  question instead of inventing synthetic vendor codes.
- Completed the first-driver diagnostic evidence maps for MAX31865 and
  ADS124S08, distinguishing native fault flags from transient state, active
  diagnostic measurements, driver-derived CRC checks, and unsupported inferred
  failures.
- Allow one normalized diagnostic to be supported by a documented combination
  of native flags/states while preserving the contributing native evidence.
- Expanded the pre-0.1 diagnostic survey with Siemens Desigo Essentials EM1.8U, corrected Beckhoff EL32xx open-circuit/status semantics from current process-data documentation, and deepened WIKA T32 monitoring/HART exposure evidence.
- Expanded pre-0.1 diagnostics research with ABB TTH300, Yokogawa YTA610/YTA710, detailed Phoenix Contact fault encoding, and additional Siemens/WIKA exposure evidence.
- Refine diagnostic normalization to preserve the greatest justified shared
  specificity while keeping broader outlier-device diagnostics separate.
- Preserve both normalized `rtd-acquire` wording and native device
  code/message evidence.

### Added

- Initial project architecture and design scaffold.
- Hardware and diagnostic-capability catalog.
- Roadmap for Python, portable C, hobbyist, and industrial acquisition support.
- Add dedicated diagnostic normalization research ledger.
- Expand industrial catalog with Siemens SITRANS TH320/420, Beckhoff EL32xx,
  Phoenix Contact MINI MCR-2-RTD-UI, and WIKA T32.xS.
- Record broad-outlier diagnostic policy without weakening more-specific common
  diagnostic concepts.
- Distinguish device-detectable from backend-observable diagnostic evidence.
- Add Honeywell Unitary HVAC controllers as a Pt1000/Ni1000 TK5000 diagnostic
  and compatibility research target.
- Record WIKA analog alarm exposure as broader than the transmitter's internal
  sensor monitoring and require HART-specific research for finer mappings.
