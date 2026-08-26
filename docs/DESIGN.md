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

The base `AcquisitionDevice` contract does **not** require `read()` to be
thread-safe or reentrant. Callers must serialize operations on a device instance
unless that implementation explicitly documents a stronger guarantee. Likewise,
if multiple device instances share one stateful transport, serialization belongs
to the caller, transport, or platform adapter rather than being silently added
to every acquisition driver.

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

The portable C API preserves the same distinction without requiring C to mimic
Python exceptions. Device-specific operation result enums may expose more
specific causes when that improves embedded error handling. For MAX31865,
`rtd_acquire_max31865_result_t` separates invalid programmer arguments,
caller/configuration errors, insufficient caller-owned result storage, SPI I/O
failure, delay failure, and defensive internal-invariant failure. SPI and delay
failures are both C forms of Python's acquisition-operation failure; invalid
static device configuration or incompatible SPI settings are configuration
failures. `rtd_acquire_max31865_config_is_valid()` remains a Boolean predicate
because its only question is whether one static configuration is valid.

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
be finite and non-negative.

A failed acquisition uses no magic resistance value. If no trustworthy
resistance is available, the resistance is absent rather than represented by
infinity, `NaN`, or another sentinel value. Zero ohms is a valid electrical
result when the acquisition hardware actually measures it; `rtd-acquire` does
not reject that value merely because it would be invalid for a particular RTD
model. Raw or otherwise untrusted converter output belongs in a future debugging/inspection interface rather than
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
- A `Measurement` contains at most one `Diagnostic` for each `DiagnosticCode`.
  When several native observations support the same normalized condition, they
  are combined in that diagnostic's `native_evidence` tuple rather than emitted
  as duplicate normalized diagnostics.
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
provided, the low threshold must be below the high threshold. Low thresholds
round downward and high thresholds round upward so quantization cannot move a
native threshold into the caller's requested diagnostic-free window. Because
the 15-bit native threshold code has no value above 32767, a high threshold in
the final unrepresentable band below `R_REF` is rejected rather than silently
clamped downward. Invalid, unsupported, or directionally unrepresentable values
raise the public `ConfigurationError` used for caller-supplied acquisition
configuration.

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

#### 5.2.1 Linux `spidev` adapter

`LinuxSpidevDevice` implements `SpiDevice` through the Linux kernel `spidev`
userspace interface. It opens a caller-selected device node or stable symlink
with Python `spidev` 3.8+ `open_path()`, applies the requested `SpiSettings`, and
uses one `xfer2()` call for each `transfer()` so chip select remains asserted
for the complete transaction. Host/kernel failures are translated to the public
`AcquisitionError` boundary.

The adapter is Linux-generic rather than Raspberry-Pi-specific. Raspberry Pi 4
and Raspberry Pi 5 reach the same userspace `/dev/spidev*` contract through
their respective kernel controller drivers, so `rtd-acquire` does not inspect
BCM2711/RP1 registers or branch on Pi model. This makes the implementation
architecturally suitable for both generations while allowing physical
validation status to remain explicit and separate.

The first documented Pi connection uses SPI0 with its standard 40-pin-header
MOSI/MISO/SCLK/CE lines. SPI must be enabled by the operating system before the
device node is available. The adapter accepts a full device path rather than
hard-coding `/dev/spidev0.0`, permitting alternate chip selects and stable udev
symlinks where bus numbering is not guaranteed.

The `spidev` Python package is an optional dependency. Importing
`rtd_acquire.transports` does not require it; a missing backend is reported only
when `LinuxSpidevDevice` is instantiated.

`SpiSettings` and device drivers raise `ConfigurationError` for incompatibilities
they can determine before touching the operating-system backend. Once
`LinuxSpidevDevice` asks the kernel/backend to open or apply settings, a failure
is treated as an operational `AcquisitionError`: the adapter does not infer from
platform-specific errno values whether the root cause was permissions, device
availability, controller capability, or a rejected setting. A future backend may
provide a more specific subclass only when it has a documented and portable
distinction.

### 5.3 MAX31865 native decode contract

MAX31865 register interpretation is independent of SPI/platform code. The RTD
register is a 16-bit wire value whose least-significant bit is the device fault
flag; the resistance code is the remaining 15-bit value. `rtd-acquire` computes
resistance as:

```text
R_RTD = (ADC code / 32768) * R_REF
```

A zero ADC code therefore represents a zero-ohm electrical result. The generic
`Measurement` contract permits zero resistance so acquisition does not silently
impose an RTD-model validity rule.

Fault Status bits D7 through D2 are normalized at the specificity actually
reported by the MAX31865. D7/D6 threshold crossings are warnings and retain the
resistance result. D5/D4/D3 electrical comparison faults and D2 combined input
voltage fault are faults and suppress the normal resistance result. Datasheet
troubleshooting causes are not promoted into normalized diagnoses.

The shared MAX31865 measurement-decode conformance vectors execute directly
against this decode layer. A separate language-neutral threshold-encoding vector
set pins configuration-to-register behavior, including directional rounding and
the unrepresentable high-threshold boundary, for reuse by the portable-C
implementation.

### 5.4 MAX31865 one-shot acquisition sequence

The first public MAX31865 driver uses documented one-shot conversion timing and
runs a fresh automatic fault-detection cycle for every `read()`. Each read is
therefore deterministic with respect to driver-owned registers rather than
assuming that a previous process left the converter configured correctly.

The operation is:

1. write the high/low threshold registers from `MAX31865Config`, restoring the
   device full-range defaults when application thresholds are omitted;
2. write the static wire/filter bits, enable VBIAS, and clear latched faults;
3. allow the input network to settle;
4. run the documented automatic fault-detection cycle so D5-D3 evidence is
   refreshed;
5. allow the input network to settle again after fault detection;
6. trigger one one-shot conversion and wait the datasheet maximum conversion
   time for the selected 50/60 Hz filter;
7. read the two RTD data bytes and, when their fault flag is set, read the Fault
   Status register;
8. return VBIAS to off;
9. pass the captured native registers to the platform-independent decode layer.

Threshold conversion is directional. High thresholds round upward and low
thresholds round downward to the nearest representable 15-bit ratio code so
quantization does not make a fault trigger *inside* the resistance window the
caller requested.

`MAX31865Timing` models the external input-filter time constant separately from
static converter configuration. The default is 1 ms, matching the conservative
time constant used in the datasheet startup characterization. Hardware with a
larger external RC time constant must supply its actual value. Bias settling is
computed as 10.5 time constants plus 1 ms; post-fault settling uses five time
constants plus 1 ms. Automatic fault detection waits its 600 us datasheet
maximum, and one-shot conversion waits 55 ms for the 60 Hz notch or 66 ms for
the 50 Hz notch.

The driver validates that its injected `SpiDevice` uses CPHA=1, MSB-first
8-bit words, active-low chip select, and a clock no faster than 5 MHz. Either
CPOL value remains valid. SPI response-shape violations are acquisition errors.

### 5.5 Deterministic MAX31865 SPI emulator

`MAX31865SpiEmulator` is a device-specific deterministic test transport that
implements the same `SpiDevice` contract consumed by the real driver. It takes
a native 15-bit RTD code and an optional native D7-D2 Fault Status value, then
models the register transactions used by the current driver: threshold writes,
fault clearing, automatic fault-cycle re-latching, one-shot RTD reads, and
Fault Status reads.

The emulator deliberately accepts native converter state rather than a
temperature or RTD model. It does not infer why a fault bit is present and does
not simulate analog settling, noise, temperature, or physical sensor behavior.
That keeps it useful for exercising the real register/driver path without
turning a deterministic fake into an unvalidated electrical model.

The fault-clear/fault-cycle distinction is intentional. A configured native
fault condition is cleared when the driver issues the documented clear command
and is re-latched when the subsequent automatic fault-detection cycle runs.
This lets CI exercise the actual driver sequence rather than bypassing it with
a scripted list of SPI responses.

### 5.6 Generic simulated acquisition device

`SimulatedAcquisitionDevice` operates at the public `AcquisitionDevice` layer
rather than pretending to be any particular converter or transport. It replays
a deterministic non-empty script whose entries are either validated
`Measurement` objects or explicit `SimulatedAcquisitionFailure` records.

A measurement entry is returned unchanged, preserving its resistance,
diagnostics, and quantified acquisition uncertainty. A simulated acquisition
failure raises `AcquisitionError` and then advances the script, matching the
same operation-error boundary used by real drivers. Reading beyond a finite
script raises `AcquisitionError` by default; callers may request deterministic
repetition of the entire script. The simulator can also be reset to its first
entry.

This generic simulator is complementary to `MAX31865SpiEmulator`: the latter
exercises the real device driver through native register/SPI behavior, while
the former gives applications a hardware-free acquisition source without any
device emulation. Neither layer simulates an RTD temperature model or physical
process. Future noise, drift, clipping, or other acquisition-behavior models may
build on this boundary, but stochastic behavior should be explicitly controlled
and reproducible rather than silently introduced into the base simulator.

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

### 6.1 Portable C SPI HAL contract

The first capability-specific C HAL is `rtd_acquire_spi_t`. It carries:

- caller-owned opaque context;
- effective SPI settings corresponding to the Python `SpiSettings` semantics;
- one transfer callback for a complete full-duplex transaction.

The transfer callback owns chip-select assertion/deassertion for the entire
transaction, matching the Python transport boundary. A platform adapter may use
hardware chip select or manually drive a GPIO internally; the MAX31865 driver
does not need to know which mechanism is used.

The HAL reports host/platform transfer success separately from device-reported
acquisition diagnostics. It requires no dynamic allocation and no
Arduino-specific headers. The first contract test compiles and executes with an
ordinary C11 host compiler so the current Arduino AVR / HERO adapter and
future STM32, ESP32, RP2040, and Linux adapters can implement the same
capability.

### 6.2 Portable C delay HAL contract

The MAX31865 core needs one additional platform capability: a blocking relative
delay. It does not require a wall clock, monotonic timestamp source, timer
allocation, or a generic GPIO API for the first implementation.

`rtd_acquire_delay_t` therefore contains only caller-owned opaque context and a
`delay_us` callback. Durations are unsigned integer microseconds rather than
floating-point seconds so the HAL has a deterministic language- and
platform-neutral unit and does not force an embedded adapter to provide
floating-point timing support.

A successful callback return means the requested interval has elapsed; an
adapter may delay longer but must not return early. The MAX31865 C driver
conservatively rounds computed minimum delays upward to whole microseconds before
calling the HAL. Platform inability to perform the delay is reported through the
HAL result and remains an API/acquisition-operation error, not a device
diagnostic.

The delay HAL requires no dynamic allocation or Arduino-specific headers. The
Arduino AVR / HERO adapter implements it with Arduino timing facilities, while
desktop tests inject a fake callback that records requested durations without
sleeping.

### 6.3 Portable C measurement and diagnostic storage

The C result contract preserves the Python `Measurement`, `Diagnostic`,
`DiagnosticSeverity`, `DiagnosticCode`, `NativeEvidence`, and derived-status
semantics without requiring heap allocation or a project-wide fixed diagnostic
maximum.

`rtd_acquire_measurement_t` binds caller-supplied arrays for normalized
diagnostics and native-evidence records. Each caller chooses capacities suitable
for its platform and use case. The completed result records the used counts, and
each diagnostic refers to a contiguous range within the measurement's shared
native-evidence array. This avoids nested allocation while preserving composite
native evidence.

Optional numeric values use explicit presence flags rather than `NaN`, infinity,
or another sentinel. The C scalar type is `rtd_acquire_real_t`, currently an
alias for C `float`. Cross-language conformance for the supported
Python-binary64/C-binary32 case is governed by the frozen
`python-binary64-c-binary32` numeric profile; C conformance runners assert the
required binary32-style `float` properties before applying that profile.

Status remains derived rather than stored independently:

```text
no diagnostics                -> OK
WARNING diagnostics only      -> WARNING
one or more FAULT diagnostics -> FAULT
```

The C validation helper enforces the same core invariants as Python: finite
non-negative resistance and uncertainty when present, no duplicate normalized
diagnostic codes, valid native-evidence ranges, non-empty native evidence, no
resistance or uncertainty on a fault result, and at least one fault diagnostic
when no trustworthy resistance is available.

Native-evidence identifier/message pointers are non-owning references. The
caller, driver, or platform adapter that supplies the referenced text must keep
it alive for as long as the completed measurement is consumed. Device drivers
with fixed native terms can satisfy this naturally with static string literals.

The mutable initialized/reset state is assembly storage, not itself a completed
valid `Measurement`; a driver fills it and returns only a semantically valid
result or a separate execution/storage error.

### 6.4 Portable C MAX31865 configuration layer

The MAX31865 C configuration layer is pure device logic with no HAL side
effects. `rtd_acquire_max31865_config_t` mirrors the electrical facts in the
Python `MAX31865Config` contract using explicit presence flags for optional
thresholds. It validates the same reference-resistance range, 2-/3-/4-wire
topologies, 50/60 Hz filter choices, threshold ordering, and high-threshold
representability rule.

The layer also derives the static MAX31865 configuration bits for three-wire
compensation and 50 Hz filtering. Operational bits such as BIAS, one-shot,
fault-cycle control, and fault clearing are acquisition-sequence concerns rather
than static configuration. Their register addresses and operational masks remain
private implementation constants rather than becoming public configuration/API
surface.

Public MAX31865 operations return `rtd_acquire_max31865_result_t` rather than a
bare success Boolean. This result vocabulary was frozen before acquisition
sequencing so the sequence can report configuration, SPI, delay, and
caller-storage failures without retrofitting the public signatures later. The
acquisition entry point validates `rtd_acquire_spi_t.settings` before its first
transfer; settings incompatible with MAX31865 requirements are reported as
`RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR` rather than being folded into
SPI I/O failure.

Threshold encoding preserves the shared directional rule: low thresholds round
downward and high thresholds round upward so quantization cannot move a native
threshold inside the caller's requested diagnostic-free window. A high
threshold in the final unrepresentable band below `R_REF` is rejected rather
than clamped downward. Disabled thresholds use the device defaults `0x0000`
(low) and `0xFFFF` (high).

This threshold behavior is exact at the 16-bit register-output boundary, so the
language-neutral threshold vectors require exact register outcomes in both
Python and C. The separate binary64/binary32 profile applies to floating-point
measurement outputs; it does not relax integer threshold-register results.

### 6.5 Portable C MAX31865 native decoding

The second MAX31865 C slice decodes one native RTD register plus one fault-status
register into the caller-owned `rtd_acquire_measurement_t` contract. The mapping
uses the same six documented MAX31865 fault bits as Python, in D7-through-D2
order. Each normalized diagnostic retains one non-owning native-evidence record
with the documented bit identifier and device message. Reserved D1/D0 bits do
not create inferred diagnostics.

The native high- and low-resistance threshold bits are warnings, so a trustworthy
resistance remains present when they are the only diagnostics. Any D5-through-D2
FAULT diagnostic removes resistance and uncertainty from the completed result,
matching the shared trust invariant rather than returning a magic value. The RTD
resistance is computed from the 15-bit ADC code and configured reference
resistance; the RTD-register fault-indicator bit is excluded by shifting the
register right by one bit.

The decoder performs no allocation. A MAX31865 native register state can produce
at most six normalized diagnostics and six native-evidence records, but those are
device-specific worst-case requirements rather than a project-wide fixed result
capacity. The public `RTD_ACQUIRE_MAX31865_MAX_DIAGNOSTICS` and
`RTD_ACQUIRE_MAX31865_MAX_NATIVE_EVIDENCE` constants let callers reserve enough
fixed storage when they want to guarantee that any native MAX31865 fault state
can be represented. If supplied result storage is insufficient, decoding fails as
an operation/storage error instead of silently dropping evidence.

The version-1 measurement-decode vectors execute against both Python and C
under the frozen `python-binary64-c-binary32` numeric profile. The vector set
includes a non-binary32-exact reference resistance so the tolerance path is
exercised rather than relying only on specially exact seed values.

### 6.6 Portable C MAX31865 one-shot acquisition sequence

The portable C MAX31865 driver completes the same fault-checked one-shot
operation defined for Python while consuming only the frozen SPI and blocking-
delay HALs. `rtd_acquire_max31865_read()` accepts caller-owned SPI, delay,
configuration, timing, and measurement objects; it allocates no storage.

The C timing policy stores the external input-filter time constant as whole
microseconds. Callers whose physical time constant is not an integral number of
microseconds must round upward before supplying it. The driver then requests
the same conservative minimum delays as Python: 10.5 time constants plus 1 ms
after enabling VBIAS, 600 us for automatic fault detection, five time constants
plus 1 ms after fault detection, and 55 ms or 66 ms for 60 Hz or 50 Hz one-shot
conversion respectively. Half-microsecond results from the 10.5 multiplier are
rounded upward before reaching the integer delay HAL. Timing values whose
required waits cannot be represented by `uint32_t` are rejected as configuration
errors before I/O.

Before the first transfer, the acquisition entry point validates the SPI HAL and
its effective settings: CPOL must be 0 or 1, CPHA must be 1, clock frequency
must be greater than zero and no more than 5 MHz, transfers must be MSB-first
with 8-bit words, and chip select must be active low. Incompatible settings are
configuration errors rather than SPI I/O failures. Operational register
addresses and BIAS/one-shot/fault-cycle masks remain private implementation
constants.

Each read restores threshold registers, enables VBIAS while clearing faults,
waits for settling, performs automatic fault detection, waits again, triggers
one conversion, reads the RTD register, conditionally reads Fault Status when
the RTD fault flag is set, and writes the static base configuration to turn
VBIAS off before decoding the captured registers.

If SPI or delay execution fails after the driver has started the biased
sequence, it makes a best-effort write of the static base configuration to turn
VBIAS off while preserving the original `SPI_IO_ERROR` or `DELAY_ERROR`. A
failure of the normal final bias-off write is itself `SPI_IO_ERROR`. The caller's
`Measurement` is not rewritten for SPI, delay, or final-bias-off failures;
decoding occurs only after I/O and normal bias shutdown complete. If the native
fault state requires more diagnostics/evidence than the caller supplied, the
completed I/O operation can still return `INSUFFICIENT_STORAGE`; the decoder
leaves the previous measurement contents untouched rather than truncating
evidence.

### 6.7 Arduino AVR / HERO platform adapter

The first concrete embedded platform adapter targets the Arduino AVR core used
by UNO R3-class boards, including the inventr.io HERO board. HERO is published
as a derivative of the Arduino UNO R3 reference design, so `arduino:avr:uno` is
the build target used for compile validation. The adapter is intentionally
platform-specific C++ around the portable C11 core; Arduino headers do not enter
`c/include/rtd_acquire/` or `c/src/`.

`rtd_acquire_arduino_avr_spi_init()` binds an Arduino `SPIClass` instance and a
caller-selected chip-select GPIO to `rtd_acquire_spi_t`. It maps CPOL/CPHA to
Arduino SPI modes, maps bit order, supports 8-bit words, owns chip-select
assertion/deassertion around each `beginTransaction()`/`endTransaction()` pair,
and calls `SPI.begin()` during adapter initialization. The caller may release
that initialization reference with `rtd_acquire_arduino_avr_spi_end()`.

The portable SPI contract records **effective** settings. ArduinoCore-avr's
`SPISettings` selects the fastest discrete AVR SPI divider that is no faster
than the requested rate, or the slowest available divider when the request is
below the hardware minimum. The adapter mirrors that divider selection when it
populates `rtd_acquire_spi_t.settings.clock_frequency_hz`. The requested rate is
still passed to `SPISettings` for each transaction, letting Arduino configure
the same divider. This matters for device validation: a 5 MHz MAX31865 request
on a 16 MHz HERO/UNO becomes an effective 4 MHz SPI clock rather than being
reported inaccurately as 5 MHz.

Arduino's byte `SPI.transfer()` API does not expose a transport-status result.
After adapter argument validation, a completed Arduino transaction therefore
maps to `RTD_ACQUIRE_SPI_OK`; the adapter cannot manufacture a host I/O error
that the platform API does not report. Device-reported MAX31865 faults remain in
`Measurement` diagnostics as usual.

`rtd_acquire_arduino_avr_delay_init()` binds the blocking delay HAL to Arduino's
`delay()` and `delayMicroseconds()` facilities. Because AVR `unsigned int` is
narrower than the HAL's `uint32_t` microsecond duration, the adapter sends whole
milliseconds through `delay()` and only the sub-millisecond remainder through
`delayMicroseconds()`. This preserves the HAL's full duration range without
truncating MAX31865 conversion or settling waits.

A strict host C++11 adapter test uses minimal Arduino/SPI stubs to verify
settings mapping, effective-clock reporting, transaction/chip-select ordering,
and long-delay splitting without requiring Arduino headers. CI additionally
stages the same portable C sources and adapter as a temporary Arduino library
and compiles the example sketch for `arduino:avr:uno` using Arduino AVR Boards
1.8.8. That compile gate validates toolchain/API compatibility but is not
physical HERO/MAX31865 validation; the real-hardware comparison remains a
separate unchecked roadmap item.

The optional Arduino/C++ convenience wrapper was evaluated at the 0.2 software
scope-freeze checkpoint and deliberately deferred. The current adapter already
lets Arduino sketches consume the portable C MAX31865 driver directly, and no
real-hardware or user-feedback evidence yet demonstrates that a second object
model would improve usability enough to justify another public API. A future
wrapper may be added if that evidence emerges, but it must delegate to the C
driver rather than duplicate acquisition, diagnostic, or timing semantics.

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

The first frozen numeric profile is stored in
`conformance/v1/numeric_profiles.json` as
`python-binary64-c-binary32`. It applies when Python uses radix-2, 53-bit
significand `float` and the C conformance target uses radix-2, 24-bit
significand `float` with the binary32 exponent range. The host C conformance
runners assert those C properties before executing the profile. A C target with
a different floating representation can still implement the portable API, but
it needs an explicitly defined target profile before claiming this particular
cross-language numeric conformance.

For MAX31865 `measurement_decode`, status, resistance presence/absence,
diagnostics, native evidence, and all integer/native fields remain exact
semantic requirements. Expected zero resistance must match zero exactly. A
nonzero C resistance is accepted when its relative difference from the
language-neutral reference value is no greater than `2^-22`
(`2.384185791015625e-7`), with zero absolute tolerance. This is two binary32
machine epsilons: a deliberately tight computational allowance for binary32
input representation and arithmetic rounding, not a sensor-accuracy or
hardware-uncertainty allowance. It remains far below one MAX31865 ADC count
throughout the supported reference-resistance range.

Configuration acceptance is not made fuzzy by this profile. Cross-language
vectors that require exact acceptance/rejection must remain stable when their
floating configuration values are represented as binary32. In ordinary public
API use, Python can distinguish decimal threshold values that collapse to the
same C `float`; the C implementation validates the values it can actually
represent and may therefore reject such a configuration. Sub-binary32
configuration distinctions are outside this shared profile rather than being
hidden behind widened comparisons.

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
Individual devices may legitimately support only a subset. The canonical list of
current parity targets lives in `HARDWARE.md`; it is intentionally not duplicated
here.

A future RTD family added to `rtd-sensor` creates a compatibility and validation
review in `rtd-acquire`. That review defaults to classifying existing acquisition
paths for manufacturer support, electrical compatibility, and project validation.
It does **not** by itself require a new hardware driver. New driver work is added
only when existing acquisition paths cannot provide suitable practical coverage
or when the normal roadmap criteria independently justify it.

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
│   ├── REFERENCES.md
│   └── CHANGELOG.md
└── .rtd-acquire-local/   # ignored local research/experiments
```

Fine-grained driver/transport package layout should be introduced as real
implementation work requires it rather than pre-created speculatively.

## 14. Deliberately unresolved items

The following should remain explicit design work rather than hidden assumptions:

- exact constructor/configuration shapes for hardware families after MAX31865;
- exact HAL signatures for capabilities beyond the frozen SPI and delay
  interfaces;
- exact industrial device/interface selected for the first industrial driver;
- exact development board/module selected for ADS124S08 hardware tests;
- uncertainty models that can be defended for each acquisition family.
