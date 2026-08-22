# rtd-acquire design

## 1. Purpose

`rtd-acquire` provides the acquisition layer between physical RTD measurement
hardware and RTD interpretation software.

Its primary output is a trustworthy estimate of RTD element resistance in
ohms, together with acquisition-level status, diagnostics, and—where a
well-founded calculation is available—standard uncertainty in ohms.

The project is designed to support both common hobbyist/maker hardware and
professional/industrial acquisition architectures.

## 2. Domain boundary

The separation between `rtd-acquire` and `rtd-sensor` is intentional.

`rtd-acquire` owns:

- communication with acquisition hardware;
- converter, ADC, transmitter, and controller behavior;
- electrical transfer functions that produce resistance;
- reference-resistor, ADC, gain, offset, excitation, and related acquisition
  configuration;
- acquisition-chain calibration;
- measurement-topology and lead compensation when needed to estimate the RTD
  element resistance;
- acquisition diagnostics and native hardware evidence;
- acquisition-derived resistance uncertainty;
- simulated acquisition devices and device emulation.

`rtd-sensor` owns:

- RTD resistance-to-temperature and temperature-to-resistance models;
- Pt, Ni, and other RTD characteristic interpretation;
- Callendar–Van Dusen and other model coefficients;
- RTD-model fitting and calibration;
- RTD tolerance classes;
- model validity ranges;
- model-level and propagated temperature uncertainty.

`rtd-acquire` must not apply temperature offsets or otherwise reinterpret an RTD
resistance as temperature.

## 3. Public terminology

The primary public abstraction is `AcquisitionDevice`.

A hardware-specific implementation such as `MAX31865` implements the
`AcquisitionDevice` contract. The standard operation is:

```python
measurement = device.read()
```

The result is a `Measurement`.

Terminology:

- **AcquisitionDevice** — something capable of producing resistance
  measurements.
- **Measurement** — the result of an acquisition operation.
- **Driver** — hardware-specific implementation details.
- **Transport** — communication such as SPI, I²C, serial, Modbus, or BACnet.
- **Converter** — actual physical conversion hardware, not the top-level
  software abstraction.

An `AcquisitionDevice` does not require or expose a particular transport.

## 4. Measurement contract

The conceptual Python contract is:

```python
Measurement(
    resistance_ohms=...,
    status=...,
    diagnostics=...,
    standard_uncertainty_ohms=...,
)
```

The exact implementation remains subject to normal pre-release refinement, but
the semantics below are design requirements.

### 4.1 Resistance

`resistance_ohms` is the acquisition system's best estimate of the RTD element's
electrical resistance, expressed in ohms.

A failed acquisition uses no magic resistance value. If no trustworthy
resistance is available, the resistance is absent rather than represented by
zero, infinity, or a sentinel value.

### 4.2 Status

Every measurement has one of three statuses:

- **OK** — a usable resistance is available and no known acquisition issue
  requires attention.
- **WARNING** — a usable resistance is available, but one or more acquisition
  conditions should be reported.
- **FAULT** — no trustworthy resistance is available.

`OK` and `WARNING` therefore imply a usable resistance. `FAULT` means the
measurement must not be treated as a trustworthy resistance result.

### 4.3 Diagnostics

Diagnostics carry the specific acquisition-level reason or reasons behind a
warning or fault.

The diagnostic object shape is:

```python
Diagnostic(
    code=...,
    severity=...,
    message=...,
    native_code=...,
    native_message=...,
)
```

Where:

- `code` is the stable, machine-readable `rtd-acquire` diagnostic identity;
- `severity` is `WARNING` or `FAULT`;
- `message` is the normalized, plain-language `rtd-acquire` wording for that
  diagnostic concept;
- `native_code` optionally preserves the originating hardware/protocol code or
  identifier;
- `native_message` optionally preserves the manufacturer's/device's own wording
  when the interface makes it available or the driver can identify it
  unambiguously from published documentation.

Applications make decisions from `code`, not by parsing either message field.
The normalized `message` is intended to be durable and consistent across
hardware, but its text is not a machine-parsing API. Avoid gratuitous wording
changes after release.

A measurement can carry more than one diagnostic.

### 4.4 Diagnostic normalization and no-inference rules

`rtd-acquire` provides its own stable, easy-to-understand diagnostic vocabulary
while preserving the originating device evidence in `native_code` and
`native_message`.

Normalization is evidence-based:

1. Native conditions are grouped only when their documented meanings genuinely
   overlap.
2. The normalized `rtd-acquire` code/message should preserve the greatest useful
   specificity justified by that semantic group.
3. A less-specific outlier device must not force a commonly available specific
   diagnostic to become vague. The outlier receives an appropriately broader
   normalized code instead.
4. Conversely, a driver must never map a broad or ambiguous native condition to
   a more specific physical diagnosis than the evidence establishes.
5. Device-specific troubleshooting causes listed by a vendor are possibilities,
   not observed facts, unless the device explicitly reports that diagnosis.
6. A condition that a device can detect internally is not automatically native
   evidence available to `rtd-acquire`. The backend must have a documented
   observation path—such as a register bit, protocol status/message, process
   data flag, or defined analog fault signal—before it can emit that native
   diagnostic.
7. A normalized diagnostic may be established by a documented combination of
   native states rather than a single vendor bit or code. When that happens,
   preserve the contributing native states in `native_code`/`native_message`
   evidence rather than pretending the device emitted a single native code it
   does not actually have.
8. Diagnostic specificity is a property of the **device + configuration +
   observation interface**, not just the device model. If a transmitter can
   distinguish two faults internally but the selected HART/BACnet/analog/etc.
   exposure collapses them, the backend must use the broader observable
   diagnosis. Conversely, a configured analog signaling scheme may preserve a
   distinction that another interface on the same device does not.

For example, if several devices explicitly and equivalently report a low
reference condition, `rtd-acquire` may expose a specific normalized
`REFERENCE_LOW` concept. A different device that only reports a broad reference
failure should map to a broader `REFERENCE_FAULT` concept; the specific devices
should not be degraded merely to accommodate the outlier.

Likewise, a resistance-high threshold must not silently become a sensor-open
code just because an open circuit is one possible cause. A normalized
sensor-circuit-open diagnostic is appropriate only when the hardware/interface
itself reports an open circuit at that level of specificity.

A device's own internal communication/ASIC diagnostic is also distinct from an
`rtd-acquire` transport operation failing to communicate with the device. The
former is native device evidence; the latter belongs to the operation-error
contract unless a later design decision explicitly mirrors it into a
`Measurement` diagnostic.

Diagnostic messages must not strengthen the evidence beyond the meaning of the
machine-readable code and native evidence. Native vendor wording is retained
where practical so users have an exact device-specific term to search in vendor
documentation and troubleshooting resources.

The initial `DiagnosticCode` vocabulary will be derived bottom-up from the
actual capabilities of planned hardware families documented in `HARDWARE.md`
and the native-diagnostic survey in `DIAGNOSTICS.md`; it will not be guessed
in advance.

### 4.5 Standard uncertainty

`standard_uncertainty_ohms` is optional. When present, it represents standard
uncertainty attributable to the acquisition chain, expressed in ohms.

Possible contributors include ADC/reference behavior, reference-resistor
uncertainty, excitation, amplifier gain/offset, quantization, noise, and
acquisition calibration.

Absence of a quantified uncertainty means that `rtd-acquire` is not providing a
defensible value; it does not mean zero uncertainty.

## 5. Hardware and transport abstraction

The architecture separates device behavior from communication and platform I/O:

```text
AcquisitionDevice
    ↓
Driver
    ↓
Transport / I/O capability
    ↓
platform implementation
```

Design requirements:

1. `AcquisitionDevice` does not assume a transport.
2. Hardware-specific drivers interpret device behavior and native diagnostics.
3. Communication/I/O capabilities are injected rather than hard-coded into
   device logic.
4. Transport/I/O layers perform communication mechanics, not device
   interpretation.
5. Drivers may use SPI, I²C, serial, ADC input, current-loop input, Modbus,
   BACnet, or other mechanisms.
6. The public behavior remains `read() -> Measurement` regardless of transport.
7. Device configuration is separate from individual measurements.
8. Low-level interfaces must be replaceable with deterministic test doubles.

A driver may use zero, one, or several lower-level capabilities; transport is
therefore not a required property of every `AcquisitionDevice`.

## 6. Portable C architecture

The embedded implementation is a portable C core. Arduino/HERO support is a
platform adapter, not the core architecture.

Design requirements:

- no mandatory dynamic allocation;
- caller-controlled/fixed-size storage for measurements and diagnostics;
- small capability-specific HAL interfaces rather than one giant HAL object;
- platform functions injected through callbacks/interface structures;
- separate API-execution errors from acquisition `OK`/`WARNING`/`FAULT`;
- preserve the same measurement and diagnostic semantics as Python;
- support desktop fake HAL implementations for deterministic tests;
- allow an optional Arduino/C++ convenience wrapper later without creating a
  separate implementation.

The initial embedded hardware platform is an Arduino-compatible HERO board.
The core must remain suitable for other MCU families such as STM32, ESP32,
RP2040, and similar platforms.

## 7. Python and C relationship

Python and C are independent implementations of a shared behavioral
specification, not wrappers around one another by default.

When both implementations support the same hardware/configuration, identical
raw/native device states should produce semantically equivalent:

- resistance values within explicitly defined numeric tolerances;
- measurement status;
- generic diagnostics;
- native diagnostic preservation;
- configuration behavior;
- acquisition uncertainty calculations when implemented.

Shared deterministic conformance vectors will exercise both implementations.
Numeric acceptance will distinguish binary64 Python behavior from embedded
binary32 behavior where necessary rather than requiring bit-for-bit equality.

Real-hardware Python-vs-C comparison supplements but does not replace
deterministic conformance testing.

Feature releases do not have to occur in Python/C lockstep.

## 8. Calibration boundary

`rtd-acquire` calibrates the electrical acquisition chain. `rtd-sensor`
calibrates or fits the RTD resistance/temperature model.

Examples that belong in `rtd-acquire`:

- measured reference-resistor values;
- ADC gain/offset correction;
- amplifier gain/offset correction;
- excitation-current correction;
- current-loop scaling;
- electrical zero/span calibration;
- measurement-topology/lead correction used to estimate element resistance.

Examples that belong in `rtd-sensor`:

- fitted `R0`;
- Callendar–Van Dusen coefficients;
- RTD characteristic fitting from temperature/resistance observations;
- RTD model uncertainty and tolerance interpretation.

Acquisition calibration belongs to device/configuration state rather than being
repeated in every measurement.

## 9. RTD compatibility model

Three compatibility claims are distinct:

1. **Manufacturer-supported** — the vendor explicitly documents support.
2. **Electrically compatible** — the configured acquisition chain can measure
   the required resistance range/topology even if not advertised for that RTD.
3. **rtd-acquire validated** — the project has explicit test evidence for the
   RTD/hardware/configuration combination.

Compatibility is configuration-specific rather than a simple property of a
converter chip. Reference resistance, excitation, gain, wiring, input limits,
and other configuration may determine whether a combination is usable.

The project must not turn an electrical-compatibility analysis into a claim of
manufacturer support.

### RTD-model parity goal

As a project-level goal, `rtd-acquire` should provide at least one supported and
validated acquisition path for every RTD family supported by `rtd-sensor`.
Individual devices may legitimately support only a subset.

Current `rtd-sensor` built-ins to track are:

- Pt100;
- Pt500;
- Pt1000;
- Ni120 6720;
- Ni1000 6180;
- Ni1000 TK5000.

A future RTD family added to `rtd-sensor` creates a corresponding compatibility
and validation task for `rtd-acquire`.

Acquisition compatibility should be reasoned about electrically—primarily in
terms of resistance range and acquisition topology—not by embedding
resistance-to-temperature model logic into `rtd-acquire`.

## 10. Future custom acquisition

A future `rtd_acquire.custom` facility should allow configurable acquisition
circuits without embedding RTD model science.

Prefer several explicit electrical models over an unstructured dictionary of
specifications. Candidate models include:

- linear voltage-to-resistance;
- current-to-resistance;
- constant-current excitation;
- voltage divider;
- ratiometric ADC;
- amplified/offset input;
- user-defined transfer function.

The Python implementation may accept callables as an escape hatch; the C
implementation may provide equivalent function-pointer callbacks.

Custom acquisition still returns the normal `Measurement` contract and obeys
the same no-inference diagnostic rule.

Declarative TOML/YAML/JSON configuration is a possible later feature, not a 0.1
requirement.

## 11. Simulation and testing

Simulation is first-class but remains within the acquisition domain.
`rtd-acquire` may simulate resistance sources, converters, ADC behavior, noise,
drift, clipping, communication failures, and acquisition diagnostics. It does
not simulate kettles, HVAC systems, or other physical processes being measured.

Two simulation levels are planned:

- **generic simulated acquisition device** for application/consumer tests;
- **device-specific emulation/fake transports** for exercising real drivers.

Fault injection must reproduce actual native/observable conditions rather than
invent inferred causes. Random behavior must be deterministic when given a
seed and should be disabled by default in normal tests.

Testing tiers:

1. **Unit/conformance** — no physical hardware; deterministic vectors and fake
   transports; runs in ordinary CI.
2. **Hardware-in-the-loop** — real converter + RTD/reference resistance on
   supported test platforms.
3. **Cross-platform physical validation** — compare Python/Raspberry Pi,
   C/HERO, and where practical an independent reference instrument.

Precision reference resistors or a resistance simulator should be used to
separate acquisition validation from RTD temperature-model validation.

## 12. Initial hardware sequence

1. **MAX31865** — first implementation; common dedicated RTD converter.
2. **ADS124S08** — second family; configurable precision ADC/front end that
   exercises excitation, references, gain, channels, calibration, and richer
   diagnostics.
3. **Industrial acquisition** — universal resistance inputs and 4–20 mA
   transmitters, followed by appropriate digital interfaces where raw
   resistance/electrical data can be acquired without duplicating RTD model
   interpretation.
4. **Additional precision ADCs** — including the AD7124 family where useful.
5. **Custom acquisition** — generic electrical transfer models and eventual
   home-built analog front ends.

## 13. Repository organization

The initial repository keeps implementation areas separate without freezing
fine-grained module structure too early:

```text
rtd-acquire/
├── README.md
├── DESIGN.md
├── ROADMAP.md
├── HARDWARE.md
├── CHANGELOG.md
├── LICENSE
├── pyproject.toml
├── src/rtd_acquire/      # Python implementation
├── tests/                # Python/unit/conformance tests
├── c/                    # portable C implementation and HAL adapters
├── conformance/          # language-neutral fixtures/vectors
├── examples/             # integration and hardware examples
└── docs/                 # later user/reference documentation
```

Fine-grained driver/transport package layout should be introduced as real
implementation work requires it rather than pre-created speculatively.

## 14. Deliberately unresolved items

The following should remain explicit design work rather than hidden assumptions:

- exact initial `DiagnosticCode` vocabulary and stable IDs after completing the
  hardware diagnostic survey;
- exact Python constructor/configuration object shapes;
- exact C HAL callback signatures and fixed diagnostic capacity;
- exact binary64/binary32 conformance tolerances;
- initial Python version floor before the first public release (the starter
  scaffold provisionally aligns with `rtd-sensor` at Python >=3.11);
- exact industrial device/interface selected for the first industrial driver;
- exact development board/module selected for ADS124S08 hardware tests;
- uncertainty models that can be defended for each acquisition family.
