# Changelog

All notable changes to this project will be documented in this file.

The first public alpha release was `0.1.0a1`.

## 0.3.0 - Unreleased

### Added

- Publish a GitHub Pages compatibility matrix for the six current MAX31865
  RTD-family configurations, keeping manufacturer support, electrical
  compatibility, and physical project validation visibly independent.
- Add the first machine-readable device compatibility record set, classifying
  conservative 4-wire MAX31865 reference-network configurations for all six
  current RTD-family parity targets while keeping nickel manufacturer support
  unestablished and every physical-validation state explicitly unvalidated.
- Extend shared Python/C MAX31865 conformance with a 4.3 kΩ reference-network
  measurement near the Pt1000 upper resistance envelope and directional
  threshold encoding across the Pt1000 full-characteristic envelope.
- Add versioned machine-readable compatibility data for the six current
  `rtd-sensor` parity targets and the independent manufacturer-support,
  electrical-compatibility, and project-validation claim dimensions.
- Define full-characteristic resistance envelopes for all six current
  `rtd-sensor` built-in RTD families and the evidence needed to distinguish
  manufacturer support, electrical compatibility, range validation, and real
  family/hardware validation.
- Record the TI ADS1220 as a later configurable precision-ADC candidate and the
  PT-100-485/MB as a low-cost raw-resistance Modbus research candidate.
- Record Atlas Scientific EZO-RTD as an evaluated temperature-output device
  outside the current resistance-backend contract.

### Changed

- Record independent verification of the pinned `rtd-sensor v0.8.0` conformance
  catalog hashes and harden compatibility-manifest completeness checks for
  future device/configuration record sets.
- Clarify that `rtd-acquire` is optional when another system already supplies a
  trustworthy RTD-element resistance in ohms, while temperature-only devices do
  not become resistance backends by reverse-converting their output.
- Define one `AcquisitionDevice` as one logical resistance source for future
  multi-channel hardware, with shared physical state below channel-scoped views
  and no generic batch API until concrete device semantics require one.

## 0.2.0 - 2026-08-25

### Added

- Add a complete GitHub Pages documentation and API site with Start Here,
  task-oriented documentation, Python and Portable C API references, advanced
  architecture/conformance material, and links to the `rtd-sensor` RTD
  Playground.
- Add an Arduino AVR / inventr.io HERO platform adapter that binds the portable
  SPI and delay HALs to `SPIClass`, caller-selected chip select, `delay()`, and
  `delayMicroseconds()`, with host contract tests and an Arduino Uno compile
  gate.
- Freeze the first Python-binary64/C-binary32 conformance profile, including a
  tightly bounded MAX31865 resistance tolerance, binary32-stable configuration
  requirements, and a non-exact vector that exercises the profile.
- Complete the portable C MAX31865 driver with a fault-checked one-shot
  SPI/delay acquisition sequence, integer-microsecond timing policy, SPI-setting
  validation, and best-effort VBIAS shutdown on execution failure.
- Add portable C MAX31865 native RTD/fault-register decoding into the shared
  caller-owned result contract, and execute the existing measurement-decode
  conformance vectors against both Python and C.
- Add the first portable C MAX31865 layer with configuration validation,
  static configuration-byte encoding, directional threshold-register encoding,
  and shared threshold-vector execution against both Python and C.
- Add caller-owned portable C `Measurement`/`Diagnostic` result storage,
  derived status and invariant validation without a project-wide fixed
  diagnostic-capacity limit.
- Add a portable C11 blocking-delay HAL with caller-owned context and integral
  microsecond durations, plus a host-side contract test and CI/release gates.
- Add a dedicated GitHub Pages Portable C API area for the SPI and delay HAL
  contracts, and annotate documented features/APIs with their introduction
  versions.

### Changed

- Expand the canonical MAX31865 hardware-validation procedure with the
  physical Arduino AVR/HERO leg and the Python/Raspberry Pi vs C/HERO
  comparison gate, while keeping those hardware-validation items pending.
- Defer the optional Arduino/C++ convenience wrapper after the 0.2 software
  scope review; the Arduino AVR adapter continues to expose the portable C
  MAX31865 API directly until hardware or user feedback demonstrates a need for
  another public layer.
- Harden release source-distribution validation so all portable-C test sources
  and version-1 JSON conformance artifacts in the checkout are required to ship
  in the sdist.
- Replace ambiguous Boolean returns from public C MAX31865 operations with a
  discriminated result enum that separates invalid arguments, configuration,
  caller-storage, SPI, delay, and internal failures; keep the pure
  `config_is_valid` query Boolean.

### Fixed

- Keep GitHub Release asset attachment manual after removing the unreliable
  automatic upload path used immediately after `0.1.0a1`; the release workflow
  retains the exact validated distributions as an Actions artifact and publishes
  those distributions to PyPI.

### Known limitations

- Physical MAX31865 validation on Raspberry Pi 4/Python and HERO/portable C has
  not yet been completed. The 0.2.0 hardware paths are covered by unit,
  conformance, emulator/host-stub, and real Arduino AVR compile/toolchain gates,
  but physical converter/RTD validation remains pending.
- The Arduino adapter targets AVR / UNO R3-class boards, including the
  inventr.io HERO; non-AVR Arduino architectures are not claimed as supported.
- A higher-level Arduino C++ convenience wrapper was evaluated and intentionally
  deferred until hardware or user feedback demonstrates a concrete need.

## 0.1.0a1 - 2026-08-22

### Added

- Implement the platform-independent MAX31865 one-shot acquisition driver,
  native register decoder, and deterministic SPI/register emulator.
- Add a Linux `spidev` transport designed for Raspberry Pi 4/5 without
  SoC-specific register access; physical validation remains tracked separately.
- Add the deterministic generic `SimulatedAcquisitionDevice`.
- Add shared MAX31865 measurement-decode and threshold-encoding conformance
  vectors for independent Python/C implementations.
- Add development quality gates, technical-reference provenance policy, and a
  reproducible real-hardware validation procedure.
- Add a simple `rtd-sensor` Pt100 integration example without creating a runtime
  dependency between the projects.
- Add CI across Python 3.11–3.14 and release automation that validates,
  smoke-tests, and publishes wheel/source distributions.
- Add public alpha installation and hardware-free quickstart instructions.

### Changed

- Permit zero-ohm `Measurement` results when acquisition hardware reports a
  trustworthy electrical zero instead of imposing an RTD-model rule.
- Require each `Measurement` to contain at most one normalized `DiagnosticCode`;
  composite native observations belong in one diagnostic's evidence tuple.
- Define `AcquisitionDevice.read()` as non-reentrant/non-thread-safe unless an
  implementation explicitly documents a stronger guarantee.
- Reject MAX31865 high thresholds that cannot be encoded without rounding the
  native threshold below the caller's requested value.
- Define runtime Linux backend open/setting failures as `AcquisitionError` while
  keeping statically detectable caller configuration failures as
  `ConfigurationError`.

### Earlier foundation work

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

### Foundation additions

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
