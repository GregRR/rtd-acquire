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

The locked Python abstraction uses structural typing:

```python
class AcquisitionDevice(Protocol):
    def read(self) -> Measurement:
        ...
```

Hardware implementations do not have to inherit from a common base class. An
object satisfies the contract by providing a compatible `read()` operation.
This keeps drivers, simulations, application test doubles, and future protocol-
based backends substitutable without imposing an inheritance hierarchy.

### 3.1 Operation errors

Device-reported acquisition conditions are represented in a `Measurement`,
including `FAULT` measurements. Exceptions are reserved for cases where the
requested software operation itself cannot complete and therefore cannot return
a measurement.

The initial exception hierarchy is:

```text
RtdAcquireError
├── ConfigurationError
└── AcquisitionError
```

- `ConfigurationError` means caller-supplied acquisition configuration is
  invalid or unsupported. It is distinct from
  `DiagnosticCode.CONFIGURATION_ERROR`, which means the acquisition device
  itself reported a configuration fault.
- `AcquisitionError` means `read()` or another acquisition operation could not
  complete far enough to return a `Measurement`, such as a host-side I/O or
  transport failure. More specific transport/backend exceptions may subclass it
  later.

This distinction prevents software-operation failures from being disguised as
device-reported diagnostic evidence.

## 4. Measurement contract

The locked Python contract is:

```python
class MeasurementStatus(StrEnum):
    OK = "ok"
    WARNING = "warning"
    FAULT = "fault"


@dataclass(frozen=True, slots=True)
class Measurement:
    resistance_ohms: float | None
    diagnostics: tuple[Diagnostic, ...] = ()
    standard_uncertainty_ohms: float | None = None

    @property
    def status(self) -> MeasurementStatus:
        ...
```

`status` is derived from diagnostic severity rather than independently stored.
This prevents contradictory states such as an `OK` measurement carrying a
`FAULT` diagnostic.

### 4.1 Resistance

`resistance_ohms` is the acquisition system's best trustworthy estimate of the
RTD element's electrical resistance, expressed in ohms. When present, it must
be finite and greater than zero.

A failed acquisition uses no magic resistance value. If no trustworthy
resistance is available, the resistance is absent rather than represented by
zero, infinity, `NaN`, or another sentinel value. Raw or otherwise untrusted
converter output belongs in a future debugging/inspection interface rather than
in `Measurement.resistance_ohms`.

### 4.2 Status

Every measurement has one of three statuses:

- **OK** — a usable resistance is available and there are no diagnostics.
- **WARNING** — a usable resistance is available and one or more diagnostics
  are present, all with `WARNING` severity.
- **FAULT** — at least one diagnostic has `FAULT` severity and no trustworthy
  resistance is available.

Status is derived deterministically:

```text
no diagnostics                -> OK
WARNING diagnostics only      -> WARNING
one or more FAULT diagnostics -> FAULT
```

A measurement with no resistance must therefore contain at least one `FAULT`
diagnostic. A fault measurement must not expose `resistance_ohms` or a
`standard_uncertainty_ohms` value as though either remained trustworthy.
WARNING and FAULT diagnostics may coexist; FAULT takes precedence.

### 4.3 Diagnostics

Diagnostics carry the acquisition-level reason or reasons behind a warning or
fault. The diagnostic contract is deliberately split into a normalized
`rtd-acquire` layer and optional device/protocol-native evidence.

The locked Python shape is:

```python
class DiagnosticSeverity(StrEnum):
    WARNING = "warning"
    FAULT = "fault"


@dataclass(frozen=True, slots=True)
class NativeEvidence:
    identifier: str | None = None
    message: str | None = None


@dataclass(frozen=True, slots=True)
class Diagnostic:
    code: DiagnosticCode
    severity: DiagnosticSeverity
    native_evidence: tuple[NativeEvidence, ...] = ()

    @property
    def message(self) -> str:
        return diagnostic_message(self.code)
```

Semantics:

- `code` is the stable normalized machine-readable `rtd-acquire` diagnostic
  identity.
- `severity` is `WARNING` or `FAULT`. Normal operation does not create an `OK`
  diagnostic.
- `message` is the canonical `rtd-acquire` wording for `code`; it is derived
  from the code rather than independently stored on each diagnostic.
- `native_evidence` preserves zero, one, or multiple native observations that
  support the normalized diagnosis.
- `NativeEvidence.identifier` is a string so native identifiers serialize
  consistently across Python, C-facing conformance data, and JSON. Preserve
  recognizable vendor notation such as `D5`, `8A`, `REF_L0`, or `0x7FFD`
  rather than coercing it into a project-specific numeric scheme.
- `NativeEvidence.message` is the concise native diagnostic name or
  device-specific wording useful for manufacturer documentation and debugging.
- Each native-evidence item must contain at least a non-empty identifier or
  message.

A diagnostic may legitimately have no native evidence. For example, a driver
may compare a device-supplied conversion CRC and emit `DATA_CRC_ERROR` when the
comparison fails even though the device has no native CRC-error status bit.

A normalized diagnosis may also require multiple native observations. For a
backend where the manufacturer documents `Overrange + Error` as an open-circuit
condition, represent those as two `NativeEvidence` entries. Never manufacture a
fake combined native identifier that the device did not emit.

Applications make decisions from `code`, not by parsing `message` or native
text. Canonical messages should remain consistent, but message wording is not a
machine-parsing contract.

### 4.4 Diagnostic normalization and no-inference rules

`rtd-acquire` provides its own stable, easy-to-understand diagnostic vocabulary
while preserving the originating device evidence in `native_evidence`.

Normalization is evidence-based:

1. Native conditions are grouped only when their documented meanings genuinely
   overlap.
2. The normalized `rtd-acquire` code/message preserves the greatest useful
   specificity justified by that semantic group.
3. A less-specific outlier device does not force a commonly available specific
   diagnostic to become vague. The outlier receives an appropriately broader
   normalized code instead.
4. Conversely, a driver never maps a broad or ambiguous native condition to a
   more specific physical diagnosis than the evidence establishes.
5. Device-specific troubleshooting causes listed by a vendor are possibilities,
   not observed facts, unless the device explicitly reports that diagnosis.
6. A condition that a device can detect internally is not automatically native
   evidence available to `rtd-acquire`. The backend must have a documented
   observation path—such as a register bit, protocol status/message, process
   data flag, or defined analog fault signal—before it can emit that evidence.
7. Diagnostic specificity is a property of **device + configuration +
   observation interface**, not just the device model.
8. Device-internal SPI/ASIC/protocol diagnostics remain distinct from failure of
   the host transport used by `rtd-acquire`; the latter belongs to the operation
   error contract unless explicitly represented otherwise later.
9. Native evidence is preserved as evidence, not rewritten into invented vendor
   terminology.
10. Canonical normalized messages never strengthen the claim beyond the
    semantics of `code` and its supporting evidence.

For example, several devices may explicitly and equivalently report a low
reference condition and map to `REFERENCE_LOW`. A different device that reports
only a broad reference failure maps to `REFERENCE_FAULT`; the specific devices
are not degraded merely to accommodate the outlier.

Likewise, a resistance-high threshold does not become a sensor-open diagnosis
just because an open circuit is one possible cause. `SENSOR_CIRCUIT_OPEN` is
used only when the device/interface actually supports that level of diagnosis.

The initial `DiagnosticCode` vocabulary is derived bottom-up from the hardware
survey in `DIAGNOSTICS.md`. It is intentionally extensible: later hardware may
add new codes, but existing codes must not be redefined to absorb unrelated
semantics.

### 4.5 Standard uncertainty

`standard_uncertainty_ohms` is optional. When present, it represents standard
uncertainty attributable to the acquisition chain, expressed in ohms.

Possible contributors include ADC/reference behavior, reference-resistor
uncertainty, excitation, amplifier gain/offset, quantization, noise, and
acquisition calibration.

Absence of a quantified uncertainty means that `rtd-acquire` is not providing a
defensible value; it does not mean zero uncertainty. When present, standard
uncertainty must be finite and non-negative. A fault measurement has no
standard uncertainty because it has no trustworthy resistance result to which
the uncertainty could apply.

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

### 5.1 MAX31865 configuration contract

The first MAX31865 public configuration is deliberately limited to stable
electrical facts that are required to interpret the converter correctly:

```python
MAX31865Config(
    reference_resistance_ohms=430.0,
    wire_count=3,
    filter_frequency_hz=60,
    low_fault_threshold_ohms=None,
    high_fault_threshold_ohms=None,
)
```

The fields mean:

- `reference_resistance_ohms` — the actual reference resistance used by the
  acquisition circuit. The MAX31865 datasheet specifies 350 Ω through 10 kΩ
  as its recommended reference-resistor operating range. No Pt100/Pt1000
  default is assumed.
- `wire_count` — the physical 2-, 3-, or 4-wire RTD connection. The MAX31865
  uses a dedicated configuration bit only for 3-wire compensation; 2-wire and
  4-wire operation share the other device setting, but the public contract
  preserves the physical topology rather than collapsing them.
- `filter_frequency_hz` — 50 or 60 Hz mains-rejection notch selection. This is
  explicit rather than inferred from locale.
- `low_fault_threshold_ohms` and `high_fault_threshold_ohms` — optional
  resistance thresholds. The driver translates them to the device's
  ratiometric threshold-register format. When omitted, the driver uses the
  device's full-range defaults rather than inventing RTD-model limits.

Thresholds are acquisition settings, not RTD-model validity limits. They must
be within the converter's representable resistance range and, when both are
provided, the low threshold must be below the high threshold.
Invalid or unsupported values raise the public `ConfigurationError` used for
caller-supplied acquisition configuration.

The configuration intentionally contains no RTD family/model name, nominal
`R0`, temperature range, or temperature-model coefficients. Those belong to
`rtd-sensor`. A MAX31865 configuration is therefore reusable wherever the
electrical resistance range and acquisition circuit are appropriate.

BIAS control, one-shot commands, fault-status clearing, and master-initiated
fault-detection-cycle command bits are operational device state rather than
static circuit configuration and are not fields in this contract. Conversion
policy and settling/timing behavior will be defined with the MAX31865 driver
and transport/timing abstraction rather than encoded prematurely here.

### 5.2 SPI transport contract

SPI drivers receive a configured per-device `SpiDevice` capability rather than
importing a platform SPI library directly. The transport exposes its effective
settings and one full-duplex transaction operation:

```python
class SpiDevice(Protocol):
    @property
    def settings(self) -> SpiSettings:
        ...

    def transfer(self, tx: bytes, /) -> bytes:
        ...
```

One `transfer()` call is one contiguous SPI transaction. The transport owns
chip-select assertion/deassertion and must keep chip select active for the whole
call. A successful transaction returns exactly one received byte for every
transmitted byte. Platform adapters translate host I/O failures into the public
acquisition-operation error boundary.

Chip select is intentionally not modeled as a separate generic GPIO operation
for the first driver. Raspberry Pi SPI controllers commonly manage CS as part of
the SPI peripheral, while Arduino-class adapters can implement the same
transaction contract by asserting/deasserting their chosen CS GPIO around the
transfer. This keeps device logic independent of that platform distinction.

`SpiSettings` records the electrical/protocol settings a driver may need to
validate: CPOL, CPHA, clock frequency, bit order, bits per word, and chip-select
polarity. Device-specific limits remain in device drivers rather than in the
generic SPI contract. For MAX31865 specifically, the datasheet permits either
CPOL value, requires CPHA=1, transfers MSB first, uses active-low CS, and
specifies SCLK up to 5 MHz.

The MAX31865 `DRDY` signal is not part of the initial transport contract. The
first driver can use documented conversion timing; optional data-ready GPIO
support may be added later if it provides a meaningful benefit without becoming
a required capability.

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
The version-1 language-neutral JSON format records public configuration,
raw/native device state, and expected normalized `Measurement` semantics.
Canonical normalized diagnostic messages are derived from `DiagnosticCode` and
therefore are not duplicated in vectors. Composite native evidence remains a
list of individual observations rather than an invented combined vendor code.

Numeric expected values in vectors are reference values. Acceptance tolerances
belong to the conformance runner/profile so binary64 Python and embedded
binary32 implementations can use appropriate numeric policies without
forking the fixture data.

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
├── LICENSE
├── pyproject.toml
├── src/rtd_acquire/      # Python implementation
├── tests/                # Python/unit/conformance tests
├── c/                    # portable C implementation and HAL adapters
├── conformance/          # language-neutral fixtures/vectors
├── examples/             # integration and hardware examples
├── docs/
│   ├── DESIGN.md
│   ├── ROADMAP.md
│   ├── HARDWARE.md
│   ├── DIAGNOSTICS.md
│   └── CHANGELOG.md
└── .rtd-acquire-local/   # ignored local research/experiments
```

Fine-grained driver/transport package layout should be introduced as real
implementation work requires it rather than pre-created speculatively.

## 14. Deliberately unresolved items

The following should remain explicit design work rather than hidden assumptions:

- exact constructor/configuration shapes for hardware families after MAX31865;
- exact C HAL callback signatures and fixed diagnostic capacity;
- exact binary64/binary32 conformance tolerances;
- initial Python version floor before the first public release (the starter
  scaffold provisionally aligns with `rtd-sensor` at Python >=3.11);
- exact industrial device/interface selected for the first industrial driver;
- exact development board/module selected for ADS124S08 hardware tests;
- uncertainty models that can be defended for each acquisition family.
