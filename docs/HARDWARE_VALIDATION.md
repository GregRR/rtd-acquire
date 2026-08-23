# MAX31865 hardware validation

This document defines the physical validation gate for the first Python
MAX31865 release path. The procedure is intentionally separate from unit,
conformance, and emulator testing: those tests establish software behavior,
while this procedure establishes that the Linux SPI adapter and real converter
work together on physical hardware.

The first validation target is Raspberry Pi 4. Raspberry Pi 5 uses the same
Linux `spidev` abstraction in `rtd-acquire`, but remains explicitly unvalidated
until the same procedure is run on a physical Pi 5.

## Required hardware

The initial validation requires:

- Raspberry Pi 4 running a supported Raspberry Pi OS release;
- a MAX31865 breakout or evaluation board with a documented reference resistor;
- a Pt100 probe compatible with the board wiring configuration;
- at least two known resistance references in the useful Pt100 resistance
  region, preferably including a value near 100 ohms;
- connection hardware appropriate to the chosen 2-, 3-, or 4-wire setup.

A calibrated decade box or characterized precision resistors are preferred for
known-resistance checks. Record the reference value and its tolerance or
measurement uncertainty rather than treating a nominal resistor marking as
exact.

Do not assume that every MAX31865 board uses a 430-ohm reference resistor.
Record the actual board model and the documented or measured `RREF` value used
for `MAX31865Config.reference_resistance_ohms`.

## Record before testing

Keep the validation record under `.rtd-acquire-local/` until the result is ready
to summarize in tracked documentation. Record at least:

- date;
- Raspberry Pi model and revision;
- Raspberry Pi OS and kernel version;
- Python version;
- `rtd-acquire` commit SHA;
- MAX31865 board manufacturer/model/revision;
- documented or measured reference resistance;
- Pt100 manufacturer/model and wire count if known;
- known-resistance reference values and tolerances/uncertainties;
- SPI device path and configured clock frequency.

If a board-specific manual or schematic is used, add it to `REFERENCES.md` in
the same change that records validation conclusions.

## 1. Verify Linux SPI access

Enable SPI in Raspberry Pi OS and reboot if required. Confirm that the expected
Linux device node exists, normally `/dev/spidev0.0` for SPI0 CE0.

The first hardware test should use the same `LinuxSpidevDevice` path that normal
applications will use. Do not substitute direct register access or a different
GPIO/SPI library for the validation run.

## 2. Verify known resistances

Connect each known resistance reference using the board manufacturer's
recommended RTD-input wiring. For each reference:

1. configure the actual board reference resistance and wiring mode;
2. collect at least 20 consecutive `MAX31865.read()` measurements;
3. record each resistance and diagnostic result;
4. verify that no unexplained acquisition exceptions or fault diagnostics occur;
5. compare the mean acquired resistance with the characterized reference.

Before declaring the result a pass, define an acceptance budget that accounts
for the known-resistance uncertainty/tolerance, MAX31865 ratio quantization,
reference-resistor uncertainty, and material connection/lead resistance. The
converter quantization step in resistance is:

```text
RREF / 32768
```

Do not claim accuracy tighter than the physical references and board components
support. A result outside the declared budget is a validation failure to
investigate, not something to widen post hoc without evidence.

## 3. Verify a real Pt100

Connect the Pt100 using its actual 2-, 3-, or 4-wire topology and collect at
least 20 consecutive measurements at a stable ambient condition.

The primary `rtd-acquire` validation result is the resistance and acquisition
diagnostics. Temperature conversion may be performed separately with
`rtd-sensor` as a sanity check, but it is not a substitute for resistance-level
validation and does not move RTD-model interpretation into `rtd-acquire`.

Record whether the observed resistance is stable and physically plausible for
the test condition. Do not use room-temperature plausibility alone as an
accuracy reference.

## 4. Verify native fault reporting

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

## 5. Pi 4 validation gate

The Raspberry Pi 4 Linux SPI and real-Pt100 roadmap items may be checked off
only when all of the following are recorded:

- real `LinuxSpidevDevice` communication with a MAX31865 succeeds;
- repeated known-resistance measurements meet a predeclared acceptance budget;
- a real Pt100 produces repeatable trustworthy resistance measurements;
- at least one real native-fault path is observed and mapped correctly;
- no unexplained acquisition failures occur during the validation run.

A concise tracked summary should identify the tested hardware/configuration and
link the conclusions to the relevant implementation and reference sources.
Raw logs may remain in `.rtd-acquire-local/`.

## 6. Raspberry Pi 5 status

Passing this procedure on Pi 4 does not make Pi 5 hardware-validated. Pi 5 may
continue to be described as supported by the Linux `spidev` architecture but
unvalidated on physical hardware until this procedure is repeated on a Pi 5.
