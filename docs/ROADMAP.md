# rtd-acquire roadmap

This roadmap is outcome-oriented. Device support should be added when it
broadens validated acquisition capability or meaningfully tests the hardware-
agnostic architecture, not simply to accumulate driver names.

## Pre-0.1 — Foundation and evidence

- [x] Complete the initial hardware catalog and `DIAGNOSTICS.md` native-
      diagnostic survey, including exact native codes/messages, semantics,
      ambiguity, and source references for representative hobbyist, precision-
      ADC, HVAC, industrial-I/O, and industrial-transmitter families.
- [x] Group genuinely equivalent native diagnostics and document broader
      outliers separately; do not reduce commonly available specificity merely
      to accommodate a coarser device.
- [x] For every diagnostic intended for a backend, document the software-
      observable exposure path; do not confuse a device's internal/local fault
      detection with evidence actually available over SPI/HART/BACnet/EtherCAT/
      analog signaling.
- [x] Exhaustively classify MAX31865 native fault/status evidence and ADS124S08
      STATUS/diagnostic facilities, including transient states, active tests,
      driver-derived CRC checks, and facilities that do **not** expose a native
      failure flag.
- [x] Draft and cross-vendor-review a non-frozen candidate normalized vocabulary,
      separating mature semantic groups from provisional device-specific or
      model-range naming questions.
- [x] Resolve structured preservation of composite native evidence with a
      tuple of `NativeEvidence` items; do not invent combined vendor codes.
- [x] Derive and freeze the first normalized `DiagnosticCode` vocabulary and
      canonical `rtd-acquire` messages from the cross-vendor survey.
- [x] Freeze the first Python diagnostic contract using `Diagnostic`,
      `DiagnosticSeverity`, and composite-capable `NativeEvidence`; normalized
      messages are derived from `DiagnosticCode`.
- [x] Freeze the first Python `Measurement` public contract around the locked
      diagnostic objects and derived status invariants.
- [x] Define the first MAX31865 configuration contract.
- [x] Define the minimal Python SPI transaction/settings abstraction; keep
      chip select transport-owned so hardware-CS and GPIO-CS platforms share the
      same driver contract.
- [x] Define the equivalent capability-specific portable C SPI HAL before the
      embedded implementation, with settings, caller-owned context, and a
      transaction callback that owns chip select.
- [x] Define initial shared conformance-vector format and seed it with
      deterministic MAX31865 OK/WARNING/FAULT cases.
- [x] Establish quality gates for pytest, ruff, mypy strict, portable-C
      compile/tests, `git diff --check`, and cross-language conformance when a C
      implementation becomes applicable.

## 0.1 — Python + MAX31865

Goal: deliver a useful Python acquisition library on real Raspberry Pi
hardware while proving the core public contracts.

- [x] Implement the locked Python `Measurement`, status, and diagnostic core
      contracts with invariant tests.
- [x] Implement `AcquisitionDevice` and acquisition exceptions/errors.
- [ ] Implement MAX31865 device behavior independently of a Raspberry Pi SPI
      library.
  - [x] Decode the native RTD ratio and D7-D2 fault status into the locked
        `Measurement`/diagnostic contract.
  - [ ] Add SPI register access, configuration writes, and conversion timing.
- [ ] Add Raspberry Pi 4 SPI/GPIO integration.
- [ ] Support appropriate 2-/3-/4-wire MAX31865 configurations.
- [x] Preserve MAX31865 native diagnostic evidence without over-interpreting it.
- [ ] Add deterministic MAX31865 register/fault emulation.
- [ ] Add a generic simulated acquisition device.
- [x] Build shared deterministic MAX31865 conformance vectors and execute them
      against the Python register decoder.
- [ ] Validate with real Pt100 hardware and known resistance references.
- [ ] Publish a simple `rtd-sensor` integration example while keeping the
      packages architecturally independent.

## 0.2 — Portable C + HERO

Goal: implement the same acquisition semantics in portable embedded C and
validate them independently against Python.

- [ ] Define caller-owned C measurement/diagnostic storage.
- [ ] Define capability-specific C HAL interfaces required by MAX31865.
- [ ] Implement the MAX31865 driver in portable C without mandatory heap use.
- [ ] Add an Arduino-compatible HERO platform adapter.
- [ ] Run shared conformance vectors against Python and C implementations.
- [ ] Define binary64/binary32 numeric acceptance profiles.
- [ ] Perform real-hardware Python/Raspberry Pi vs C/HERO comparison.
- [ ] Add an optional thin Arduino/C++ convenience wrapper only if it improves
      usability without duplicating the C driver.

## 0.3 — RTD-family acquisition validation

Goal: ensure the project as a whole provides practical acquisition paths for
all current `rtd-sensor` built-in RTD families.

- [ ] Document resistance requirements for Pt100, Pt500, Pt1000, Ni120,
      Ni1000 6180, and Ni1000 TK5000 without importing temperature-model logic
      into acquisition drivers.
- [ ] Classify tested combinations as manufacturer-supported, electrically
      compatible, and/or `rtd-acquire` validated.
- [ ] Validate appropriate MAX31865 configurations beyond Pt100 where the
      electrical range and reference network support them.
- [ ] Add precision reference-resistance validation across representative
      low/mid/high operating points.
- [ ] Document unsupported or unvalidated combinations explicitly.

## 0.4 — Configurable precision ADC: ADS124S08

Goal: prove that the architecture handles a substantially more configurable
acquisition front end rather than only dedicated RTD converters.

- [ ] Select a suitable ADS124S08 development board/module with required pins,
      reference, and excitation capabilities exposed.
- [ ] Model excitation currents, reference selection/resistance, PGA gain,
      channels, wiring topology, and relevant calibration.
- [ ] Implement Python ADS124S08 acquisition.
- [ ] Map only datasheet-supported status/diagnostic evidence.
- [ ] Validate platinum and nickel RTD acquisition configurations where
      electrically appropriate.
- [ ] Extend shared conformance vectors.
- [ ] Implement portable C support when useful; it need not block the initial
      Python hardware release.

## 0.5 — Industrial analog acquisition

Goal: cover common process/HVAC acquisition architectures rather than only
board-level converter ICs.

- [ ] Implement a representative 4–20 mA RTD transmitter path, beginning with
      KFD0-TR-1 or another well-documented device where raw/non-linearized
      acquisition can preserve the `rtd-acquire`/`rtd-sensor` boundary.
- [ ] Define analog current-input configuration and electrical calibration.
- [ ] Survey and select representative industrial universal/resistance inputs;
      Siemens SITRANS TH320/420, Siemens Desigo Essentials EM1.8U, Beckhoff
      EL32xx, Phoenix Contact MINI MCR-2-RTD-UI, ABB TTH300, and Yokogawa
      YTA610/YTA710 are current
      high-value catalog/reference candidates because they expose
      resistance-oriented acquisition rather than requiring temperature output.
      Honeywell Unitary controllers are an additional HVAC parity target because
      they explicitly support Pt1000 and Ni1000 TK5000 plus custom resistive
      inputs; confirm raw-resistance and fault-status exposure before selecting a
      backend.
- [ ] Validate at least one Ni1000/Pt1000-oriented HVAC/building-automation
      acquisition path.
- [ ] Extend diagnostic catalog with only status information actually exposed
      through the chosen devices/interfaces.

## 0.6 — Industrial digital acquisition

Goal: acquire trustworthy resistance/electrical observations through common
industrial protocols without making `rtd-acquire` a general automation stack.

- [ ] Add a representative Modbus path where the device exposes appropriate raw
      resistance/electrical data. Siemens Desigo Essentials EM1.8U is now the
      leading HVAC/building candidate because it documents raw-ohms registers and
      separate per-channel reliability registers.
- [ ] Add BACnet support where a chosen HVAC/building device exposes suitable
      acquisition data and diagnostic status.
- [ ] Evaluate HART integration for transmitter diagnostics/data where useful.
- [ ] Keep protocol transports separate from device interpretation.

## Later — Broader front ends and custom acquisition

- [ ] Add AD7124-4/AD7124-8 support if demand or validation value justifies it.
- [ ] Add explicit generic electrical acquisition models: voltage, current,
      constant-current, divider, ratiometric, and amplified/offset inputs.
- [ ] Add Python user-defined transfer functions.
- [ ] Add equivalent C function-pointer custom transfer support.
- [ ] Design and validate a custom op-amp/ADC RTD acquisition circuit.
- [ ] Consider declarative custom-acquisition configuration after the object/API
      contracts are mature.

## Ongoing compatibility rule

Whenever `rtd-sensor` adds a supported RTD family, open a corresponding
`rtd-acquire` compatibility task. The goal is at least one documented,
validated acquisition path for every RTD family supported by `rtd-sensor`.
