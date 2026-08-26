# MAX31865 hardware validation

This document defines the physical validation gates for the MAX31865 acquisition
paths in `rtd-acquire`. The procedure is intentionally separate from unit,
conformance, emulator, host-stub, and compile/toolchain testing: those tests
establish software behavior, while this procedure establishes that the software,
platform adapter, converter, wiring, and resistance source work together on
physical hardware.

The first Python validation target is Raspberry Pi 4 using the real Linux
`spidev` path. The first portable-C comparison target is the Arduino AVR /
UNO-class adapter on an inventr.io HERO. Raspberry Pi 5 uses the same Linux
`spidev` abstraction in `rtd-acquire`, but remains explicitly unvalidated until
the Pi procedure is run on a physical Pi 5.

Neither successful Arduino compilation nor host-side adapter tests count as
physical HERO validation. Likewise, emulator and conformance results do not
count as physical Raspberry Pi/MAX31865 validation.

## Required hardware

The complete Pi-versus-HERO comparison requires:

- Raspberry Pi 4 running a supported Raspberry Pi OS release;
- an inventr.io HERO or equivalent Arduino AVR / UNO R3-class validation board;
- a MAX31865 breakout or evaluation board with a documented reference resistor;
- a Pt100 probe compatible with the board wiring configuration;
- at least two known resistance references in the useful Pt100 resistance
  region, preferably including a value near 100 ohms;
- connection hardware appropriate to the chosen 2-, 3-, or 4-wire setup; and
- the USB/serial connection needed to build, upload, and record HERO results.

When practical, use the **same MAX31865 board, Pt100, resistance references, and
wiring harness sequentially on both platforms**. This reduces converter,
reference-resistor, and wiring differences in the cross-platform comparison. If
separate MAX31865 boards are used, characterize and record each board's `RREF`
and include the additional board-to-board uncertainty in the acceptance budget.

A calibrated decade box or characterized precision resistors are preferred for
known-resistance checks. Record the reference value and its tolerance or
measurement uncertainty rather than treating a nominal resistor marking as
exact.

Do not assume that every MAX31865 board uses a 430-ohm reference resistor.
Record the actual board model and the documented or measured `RREF` value used
by both implementations.

## Record before testing

Keep the validation record under `.rtd-acquire-local/` until the result is ready
to summarize in tracked documentation. Record the common test configuration:

- date;
- `rtd-acquire` commit SHA;
- MAX31865 board manufacturer/model/revision;
- documented or measured reference resistance;
- Pt100 manufacturer/model and wire count if known;
- known-resistance reference values and tolerances/uncertainties;
- whether the same MAX31865 board and wiring were used for both platforms; and
- the predeclared per-platform and cross-platform acceptance budgets.

For the Raspberry Pi/Python leg, also record:

- Raspberry Pi model and revision;
- Raspberry Pi OS and kernel version;
- Python version;
- SPI device path and configured clock frequency; and
- the installed `rtd-acquire` version or source checkout used for the run.

For the HERO/C leg, also record:

- HERO board/revision or exact UNO-class target;
- Arduino CLI and Arduino AVR Boards core versions, or equivalent IDE/toolchain
  versions;
- board target/FQBN (normally `arduino:avr:uno` for HERO validation);
- validation sketch or harness revision;
- chip-select pin;
- requested SPI clock and the adapter-reported effective SPI clock; and
- MAX31865 wiring/filter/timing configuration used by the C driver.

If a board-specific manual or schematic is used, add it to `REFERENCES.md` in
the same change that records validation conclusions.

## Acceptance-budget rules

Define the acceptance budget **before** looking at the final measurements. At a
minimum it should account for:

- known-resistance uncertainty/tolerance;
- MAX31865 ratio quantization;
- MAX31865 reference-resistor uncertainty;
- material connection/lead resistance; and
- when different converter boards are used, board-to-board component
  differences.

The converter quantization step in resistance is:

```text
RREF / 32768
```

Do not claim accuracy tighter than the physical references and board components
support. A result outside the declared budget is a validation failure to
investigate, not something to widen post hoc without evidence.

The software binary64/binary32 conformance tolerance is **not** by itself an
appropriate physical-hardware acceptance budget. Physical comparison includes
converter, reference, wiring, and repeatability effects that are absent from
raw-register conformance testing.

## Part A — Raspberry Pi 4 / Python validation

### 1. Verify Linux SPI access

Enable SPI in Raspberry Pi OS and reboot if required. Confirm that the expected
Linux device node exists, normally `/dev/spidev0.0` for SPI0 CE0.

The first hardware test should use the same `LinuxSpidevDevice` path that normal
applications will use. Do not substitute direct register access or a different
GPIO/SPI library for the validation run.

### 2. Verify known resistances on Raspberry Pi

Connect each known resistance reference using the board manufacturer's
recommended RTD-input wiring. For each reference:

1. configure the actual board reference resistance and wiring mode;
2. collect at least 20 consecutive `MAX31865.read()` measurements;
3. record each resistance and diagnostic result;
4. verify that no unexplained acquisition exceptions or fault diagnostics occur;
5. compute the mean and a simple spread measure such as minimum/maximum or
   standard deviation; and
6. compare the mean acquired resistance with the characterized reference using
   the predeclared budget.

### 3. Verify a real Pt100 on Raspberry Pi

Connect the Pt100 using its actual 2-, 3-, or 4-wire topology and collect at
least 20 consecutive measurements at a stable ambient condition.

The primary `rtd-acquire` validation result is the resistance and acquisition
diagnostics. Temperature conversion may be performed separately with
`rtd-sensor` as a sanity check, but it is not a substitute for resistance-level
validation and does not move RTD-model interpretation into `rtd-acquire`.

Record whether the observed resistance is stable and physically plausible for
the test condition. Do not use room-temperature plausibility alone as an
accuracy reference.

### 4. Verify native fault reporting on Raspberry Pi

Exercise at least one safe, reversible physical fault condition supported by
the board setup, such as disconnecting the RTD input. Confirm that:

- the real MAX31865 reports native fault evidence;
- `rtd-acquire` returns the corresponding normalized diagnostic without
  inventing a more-specific physical cause than the native evidence supports;
- a fault that makes resistance untrustworthy returns
  `Measurement.resistance_ohms is None`.

Record the actual native status bits observed. Do not require a generic
`SENSOR_CIRCUIT_OPEN` result merely because the test physically disconnected a
wire; the MAX31865 diagnostic mapping remains limited to what its status bits
establish.

### 5. Raspberry Pi 4 validation gate

The Raspberry Pi 4 Linux SPI and real-Pt100 roadmap items may be checked off
only when all of the following are recorded:

- real `LinuxSpidevDevice` communication with a MAX31865 succeeds;
- repeated known-resistance measurements meet a predeclared acceptance budget;
- a real Pt100 produces repeatable trustworthy resistance measurements;
- at least one real native-fault path is observed and mapped correctly; and
- no unexplained acquisition failures occur during the validation run.

## Part B — HERO / portable-C validation

### 6. Verify the real Arduino AVR toolchain and upload path

Build the tracked MAX31865 example or a validation harness from the same
`rtd-acquire` commit being validated. For the initial HERO target, use the real
Arduino AVR core and `arduino:avr:uno` target unless the recorded HERO setup
requires a documented alternative.

Compile success is already a software/toolchain gate in CI. Physical validation
adds the requirement that the resulting sketch can be uploaded to the actual
HERO and can communicate with the real MAX31865 through the
`rtd_acquire_arduino_avr_*` SPI and delay adapters.

The tracked `max31865_read.ino` example is sufficient for basic resistance-read
checks after its configuration is matched to the actual board. It is **not** by
itself sufficient for the native-fault comparison because it does not print the
full normalized diagnostic/native-evidence contents. For that portion, use a
validation harness that exposes at least:

- `rtd_acquire_max31865_result_t`;
- `Measurement` status and resistance-presence state;
- each normalized diagnostic code and severity; and
- each native-evidence identifier/message.

If that harness is not tracked, keep its exact source and build metadata under
`.rtd-acquire-local/` with the validation record so the result remains
reproducible.

### 7. Verify known resistances on HERO

Using the same MAX31865 configuration and physical references as the Raspberry
Pi leg where practical, for each known resistance:

1. configure the actual `RREF`, wire count, filter frequency, SPI settings, and
   chip-select pin;
2. collect at least 20 consecutive C-driver measurements;
3. record each operation result, resistance, measurement status, and diagnostic
   result;
4. verify that no unexplained operation failures or fault diagnostics occur;
5. compute the same summary statistics used for the Raspberry Pi leg; and
6. compare the mean with the characterized reference using the predeclared
   HERO acceptance budget.

Record the requested SPI frequency and the effective SPI frequency reported by
the AVR adapter. MAX31865 compatibility is evaluated against the effective
settings actually used by the hardware.

### 8. Verify a real Pt100 on HERO

Connect the same Pt100 and topology used for the Raspberry Pi leg where
practical. Collect at least 20 consecutive measurements at a stable ambient
condition.

Confirm that the C result contract reports stable, trustworthy resistance with
no unexplained diagnostics or operation failures. As on Raspberry Pi,
temperature conversion is only an optional external sanity check and is not the
primary `rtd-acquire` validation result.

### 9. Verify native fault reporting on HERO

Using the diagnostic-capable validation harness, exercise the same safe,
reversible physical fault condition used on Raspberry Pi where practical.
Confirm that:

- the C MAX31865 path observes the real native fault bits;
- normalized diagnostic code/severity and native evidence match the documented
  MAX31865 mapping;
- a warning-only condition retains resistance when the hardware provides a
  trustworthy value; and
- an acquisition-invalidating fault suppresses resistance.

Arduino AVR's byte `SPI.transfer()` API does not expose host transport errors,
so this physical test evaluates converter communication and device-reported
fault semantics; it does not manufacture an SPI I/O error the Arduino API
cannot report.

### 10. HERO validation gate

The HERO side may be described as physically validated only when all of the
following are recorded:

- the real sketch uploads and runs on the physical HERO;
- the Arduino AVR adapter communicates with the physical MAX31865;
- repeated known-resistance measurements meet the predeclared HERO budget;
- a real Pt100 produces repeatable trustworthy resistance measurements;
- at least one real native-fault path is observed and mapped correctly; and
- no unexplained acquisition failures occur during the run.

## Part C — Python/Raspberry Pi vs C/HERO comparison

### 11. Compare the two physical paths

After both platform legs individually pass, compare them using the common
physical references and configuration.

For each known resistance reference:

1. compare both platform means with the characterized reference;
2. compare the two platform means with each other;
3. verify both are inside their predeclared physical acceptance budgets; and
4. verify the inter-platform difference is inside the predeclared combined
   comparison budget.

For the Pt100 run, compare repeatability and resistance level under as nearly
the same stable condition as practical. Do not interpret a small temperature
drift between sequential runs as a software discrepancy without independent
evidence.

For the physical fault run, compare the **semantic result**, not C/Python object
layout:

- normalized diagnostic code;
- WARNING/FAULT severity and derived measurement status;
- resistance present versus suppressed; and
- native MAX31865 evidence.

A successful comparison does not require bit-for-bit equality of Python
binary64 and C binary32 resistance values. It requires both physical paths to
meet their declared budgets and the diagnostic semantics to agree.

### 12. Cross-platform validation gate

The 0.2 roadmap item **Perform real-hardware Python/Raspberry Pi vs C/HERO
comparison** may be checked off only when:

- the Raspberry Pi 4/Python gate passes;
- the HERO/C gate passes;
- the known-resistance comparison passes its predeclared combined budget;
- Pt100 behavior is repeatable and mutually consistent within the physical test
  conditions;
- the tested native-fault semantics agree; and
- the tracked summary clearly identifies hardware, software revisions,
  configuration, acceptance budgets, and any limitations.

A concise tracked summary should identify the tested hardware/configuration and
link the conclusions to the relevant implementation and reference sources. Raw
logs, serial captures, calculations, and local validation harnesses may remain
under `.rtd-acquire-local/`.

### 13. Raspberry Pi 5 status

Passing this procedure on Pi 4 and HERO does not make Pi 5 hardware-validated.
Pi 5 may continue to be described as supported by the Linux `spidev`
architecture but unvalidated on physical hardware until the Raspberry Pi
procedure is repeated on a Pi 5.
