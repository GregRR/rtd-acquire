# Hardware validation

Unit tests, emulators, conformance vectors, host adapter tests, and toolchain
compilation establish software behavior. They do not establish that a real
converter, wiring setup, platform transport, and RTD work together correctly.

The project therefore keeps physical validation as a separate gate.

## Raspberry Pi 4 / Python target

The first Python target is a Raspberry Pi 4 using the real Linux
`LinuxSpidevDevice` path with a MAX31865, known resistance references, and a
real Pt100.

The validation procedure requires, among other things:

- successful communication through the real Linux SPI adapter;
- repeated known-resistance measurements judged against a predeclared
  acceptance budget;
- repeatable real-Pt100 resistance measurements;
- at least one safe native-fault observation and correct normalization; and
- no unexplained acquisition failures during the run.

## HERO / portable-C target

**HERO comparison procedure introduced in:** `rtd-acquire 0.2.0`

The first embedded comparison target is the Arduino AVR / UNO-class adapter on
an inventr.io HERO. The HERO leg uses the same physical MAX31865, Pt100,
resistance references, and wiring as the Raspberry Pi leg where practical.
Using the same converter board sequentially reduces board-to-board uncertainty.

The HERO procedure requires:

- upload and execution on the real HERO, not merely successful compilation;
- communication through the real Arduino AVR SPI/delay adapters;
- repeated known-resistance measurements inside a predeclared physical budget;
- repeatable real-Pt100 resistance measurements; and
- at least one safe physical fault observed through the C
  `Measurement`/`Diagnostic`/`NativeEvidence` contracts.

The minimal Arduino example is sufficient for basic resistance acquisition but
does not print full diagnostic/native-evidence details. The physical fault step
therefore requires a diagnostic-capable validation harness whose source/build
metadata is retained with the validation record.

## Cross-platform comparison

After each platform passes independently, the 0.2 comparison checks that:

- both known-resistance means meet their individual physical budgets;
- the Pi/Python and HERO/C means agree within a predeclared combined budget;
- real-Pt100 behavior is mutually consistent under stable test conditions; and
- normalized diagnostic severity, resistance trust, and native MAX31865 evidence
  agree for the tested physical fault condition.

The binary64/binary32 software conformance tolerance is not a substitute for a
physical acceptance budget. Real hardware adds reference-resistor, converter,
wiring, resistance-standard, and repeatability uncertainty.

## Do not substitute plausibility for accuracy

A room-temperature Pt100 producing a plausible-looking value is useful as a
sanity check, but it is not an accuracy reference. Known resistance references
and their tolerances/uncertainties are needed for a defensible validation claim.

The canonical detailed procedure remains in
[`docs/HARDWARE_VALIDATION.md`](https://github.com/GregRR/rtd-acquire/blob/main/docs/HARDWARE_VALIDATION.md).

!!! warning "Current status"
    `0.1.0a1` was released before the Raspberry Pi/MAX31865 physical validation
    gate was completed. The 0.2 software implementation and HERO/UNO toolchain
    compile gate are also complete, but the physical Pi-versus-HERO comparison
    remains pending. Implemented/toolchain-verified support must therefore stay
    distinct from physically validated hardware.
