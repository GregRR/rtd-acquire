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

## Part D — 0.3 RTD-family envelope validation

**Family-envelope procedure introduced in:** `rtd-acquire 0.3.0`

This part extends the MAX31865 physical procedure from the original Pt100-focused
platform gates to the six current RTD-family parity targets. It deliberately
separates two physical-evidence depths already defined by the compatibility
model:

- `range_validated` uses characterized resistance sources to exercise the
  assessed acquisition configuration near the low, middle, and high portions of
  a family's complete required resistance envelope; and
- `family_hardware_validated` additionally exercises a real RTD from the named
  family with the applicable wiring and native-fault behavior recorded.

Precision-resistance substitution is therefore sufficient for range evidence but
cannot by itself establish family/hardware validation. Conversely, one
plausible-looking real RTD reading does not establish range validation.

Physical evidence is also platform-scoped. A successful Raspberry Pi/Python run
must not be presented as HERO/C validation, or vice versa, unless the applicable
platform leg was actually executed and recorded.

### 14. Select low, middle, and high precision-resistance points

For each family/configuration being range-validated, use the required resistance
envelope in `compatibility/v1/rtd_families.json` and the exact assessed
configuration in the applicable compatibility record set. Do not derive these
points from nominal `R0` alone and do not reproduce RTD temperature-model
coefficients in the validation harness.

Define normalized position inside the required envelope as:

```text
position = (Rtest - Rmin) / (Rmax - Rmin)
```

Select three **distinct characterized resistance references** meeting all of the
following:

- **low:** `0.00 <= position <= 0.10`;
- **middle:** `0.45 <= position <= 0.55`; and
- **high:** `0.90 <= position <= 1.00`.

The actual reference need not equal a mathematically convenient target. Record
its characterized resistance and uncertainty and verify that its normalized
position falls inside the required band. The nominal resistance at 0 °C may be
an additional useful point, but it does not replace the middle point unless it
actually lies in the middle band.

The following 5%/50%/95% values are planning examples, not required resistor
values. They are derived only from the tracked acquisition envelopes and are
included to make test-equipment selection easier:

| RTD family | Required envelope | Assessed `RREF` | Example low (5%) | Example middle (50%) | Example high (95%) |
| --- | ---: | ---: | ---: | ---: | ---: |
| Pt100 | 18.52008–390.481125 Ω | 430 Ω | 37.118 Ω | 204.501 Ω | 371.883 Ω |
| Pt500 | 92.6004–1952.405625 Ω | 2 kΩ | 185.591 Ω | 1022.503 Ω | 1859.415 Ω |
| Pt1000 | 185.2008–3904.81125 Ω | 4.3 kΩ | 371.181 Ω | 2045.006 Ω | 3718.831 Ω |
| Ni120 North American 6720 | 66.6–380.3099 Ω | 430 Ω | 82.285 Ω | 223.455 Ω | 364.624 Ω |
| Ni1000 6180 | 695.202595–2891.5625 Ω | 4.3 kΩ | 805.021 Ω | 1793.383 Ω | 2781.745 Ω |
| Ni1000 TK5000 | 751.79284–2517.265625 Ω | 4.3 kΩ | 840.066 Ω | 1634.529 Ω | 2428.992 Ω |

If a precision source is reused across multiple family records, it counts for
each record only when it independently satisfies that record's low/middle/high
position rule and uses the exact assessed configuration. Reuse must be explicit
in the validation evidence rather than inferred afterward.

### 15. Characterize the resistance source and acceptance budget

Prefer a calibrated decade box, resistance calibrator, or individually
characterized precision resistors. Where practical, characterize the source
with a four-terminal measurement so lead/contact resistance is not silently
folded into the reference value. Record the instrument or calibration source,
measurement date, characterized value, tolerance/uncertainty, and connection
method.

Predeclare an acceptance budget for **each family/configuration and platform**
before examining the final results. In addition to the general budget terms
above, account for the fact that different assessed `RREF` networks have
different converter quantization steps (`RREF / 32768`) and may have different
reference-resistor uncertainty.

Do not use one absolute ohm tolerance across 430 Ω, 2 kΩ, and 4.3 kΩ reference
networks merely for convenience. The declared budget must be defensible for the
specific physical configuration and reference equipment being tested.

### 16. Run precision range validation

For each family/configuration and for each selected low/middle/high resistance
reference:

1. configure the exact assessed `RREF` and 4-wire topology from the compatibility
   record;
2. connect the characterized resistance source using the board manufacturer's
   recommended four-wire/Kelvin arrangement where available;
3. collect at least 20 consecutive measurements through the platform path being
   validated;
4. record every resistance result, acquisition/operation result, measurement
   status, and diagnostic/native evidence;
5. verify that no unexplained acquisition failures or fault diagnostics occur;
6. compute the mean, standard deviation, minimum, maximum, and signed error from
   the characterized reference; and
7. verify the mean error and repeatability against the predeclared budget.

A family/configuration may claim `range_validated` only when **all three**
low/middle/high points pass. A missing or failed point leaves that record without
range-validation depth even if the other points look excellent.

The low/middle/high substitution tests validate the resistance-acquisition path
across the required envelope. They do not demonstrate RTD temperature-model
accuracy and do not require `rtd-sensor` at runtime.

### 17. Run real-family hardware validation

To add `family_hardware_validated` for a record, connect a real RTD matching the
recorded family identity and the assessed 4-wire configuration. Record the RTD
manufacturer/model, characteristic/family designation, wiring, and any relevant
probe tolerance/class information available from the manufacturer.

At a stable test condition:

1. acquire at least 20 consecutive resistance measurements;
2. compare the acquired resistance with an independent resistance reference
   where practical, such as a calibrated four-wire ohmmeter measurement made
   under sufficiently stable conditions;
3. record repeatability, diagnostics, and any material environmental drift;
4. exercise or reference at least one safe native-fault observation applicable
   to the same converter/path/topology; and
5. verify the result against a predeclared family-hardware acceptance budget.

A calibrated temperature environment plus the external RTD model may be useful
additional evidence, but it must remain clearly identified as an indirect
resistance reference. `rtd-acquire` validation should not silently turn a
resistance-to-temperature model into acquisition truth.

Family/hardware validation does **not** require physically driving the RTD across
its entire published temperature characteristic. The precision-resistance
low/middle/high tests provide the full-envelope acquisition evidence; the real
RTD test establishes that the intended sensor family, wiring, converter, and
software path work together on physical hardware.

A converter-level native-fault observation may be referenced by more than one
family record only when it was produced with the same applicable converter,
platform path, and wiring topology and the shared evidence is explicitly
identified. Do not imply that an untested topology inherited the result.

### 18. Promote compatibility evidence only after the gate passes

After the physical evidence is finalized, update only the claim depth actually
supported:

- low/middle/high precision-reference success may add `range_validated`;
- qualifying real-family RTD evidence may add `family_hardware_validated`; and
- `project_validation` becomes `validated` only when at least one qualifying
  validation depth and its reproducible evidence are attached.

Do not change manufacturer-support or electrical-compatibility states merely
because physical validation succeeded. Likewise, do not generalize a validation
result to untested platforms, wire counts, reference-resistor values, board
revisions, or RTD families.

The tracked summary should identify the exact compatibility-record ID(s) covered,
platform/runtime path, hardware revisions, resistance points, acceptance
budgets, and evidence artifacts. Raw captures and calculations may remain under
`.rtd-acquire-local/` until they are summarized in tracked project evidence.

## Part E — Reproducible validation records and capture helpers

The tracked `validation/v1/` template and helpers standardize local evidence
capture without making raw bench data part of the public repository. Actual
runs remain under `.rtd-acquire-local/validation/` until they are reviewed and
ready to summarize.

### 19. Initialize the local record before final measurements

From the repository root, create a record directory with:

```sh
uv run --locked python -m validation.create_record <record-id>
```

The initializer creates:

- `record.md`, copied from the versioned validation template; and
- `environment.json`, which records UTC creation time, the current
  `rtd-acquire` commit/package version, whether the Git worktree is clean,
  Python implementation/version, operating system family/kernel release, and
  machine architecture.

The environment helper intentionally does **not** collect the hostname, user
name, or dirty-worktree filenames. Board serial numbers, local paths, operator
identity, and other local hardware details should be added manually only when
they are needed for the validation record and appropriate to retain.

For final claim-bearing validation, require `git_worktree_clean` to be `true`.
A dirty record can still be useful during bench development, but the recorded
commit alone does not uniquely identify the code that produced those
measurements.

Fill in the acceptance budget, hardware configuration, resistance-source
characterization, compatibility-record IDs, and intended validation depth in
`record.md` before examining the final result set. The helper does not invent
those scientific decisions.

### 20. Capture Linux/Python MAX31865 measurements structurally

For Raspberry Pi/Linux MAX31865 work, first ensure the optional hardware
dependency is installed:

```sh
uv sync --locked --extra raspberry-pi
```

Then capture one labeled run into the initialized record directory:

```sh
uv run --locked python -m validation.capture_max31865 \
  .rtd-acquire-local/validation/<record-id> \
  <capture-label> \
  --spi-path /dev/spidev0.0 \
  --reference-resistance-ohms 430 \
  --wire-count 4 \
  --filter-frequency-hz 60 \
  --count 20
```

Use the actual `RREF`, wire count, filter frequency, SPI path, clock, timing,
and thresholds for the run. Optional CLI arguments allow the non-default SPI
clock, low/high thresholds, and input-filter time constant to be recorded with
the capture.

For each requested acquisition attempt, the helper writes either the complete
public `Measurement` payload or a captured `RtdAcquireError`. It therefore
preserves successful resistances, status, normalized diagnostics, canonical
messages, native evidence, and acquisition-operation failures instead of only
printing the successful values.

Each label produces three files:

- `<label>.measurements.jsonl` — one timestamped structured acquisition attempt
  per line;
- `<label>.summary.json` — measurement/error counts, status and diagnostic
  counts, and resistance mean/sample-standard-deviation/minimum/maximum; and
- `<label>.capture.json` — the exact capture configuration and SHA-256 digests
  of the raw and summary files.

The helper requires the initialized record's `record.md` and version-1
`environment.json` before it writes a capture. All generated JSON is UTF-8 with
finite numeric values and deterministic line endings. If a new capture fails
while writing its three output files, those newly created partial files are
removed so the same label can be retried.

The helper refuses to overwrite an existing label. This makes accidental loss
or silent replacement of completed physical evidence visible.

### 21. Keep acceptance and promotion decisions explicit

The capture summary is evidence, not a pass/fail oracle. Copy the relevant
statistics into `record.md`, compare them with the **predeclared** acceptance
budget, attach any external calculations or instrument records, and record the
conclusion explicitly.

The helper does not:

- decide whether mean error or repeatability is acceptable;
- infer an RTD temperature model;
- convert resistance to temperature;
- promote `project_validation`;
- add `range_validated` or `family_hardware_validated`; or
- alter manufacturer-support/electrical-compatibility claims.

HERO/C serial output, instrument exports, calibration certificates, and custom
validation harnesses may be stored alongside these files in the same local
record directory. The tracked template provides common sections for those
artifacts even when the Python capture helper is not applicable.
