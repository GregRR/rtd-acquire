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

## RTD-family envelope validation

**Family-envelope validation introduced in:** `rtd-acquire 0.3.0`

The 0.3 procedure extends physical validation from the original Pt100-focused
platform gates to every current RTD-family compatibility record. It keeps two
validation depths distinct:

- `range_validated` requires characterized resistance references near the low,
  middle, and high parts of the complete family resistance envelope; and
- `family_hardware_validated` additionally requires a real RTD from that family
  on the assessed hardware/wiring path.

For a family envelope `Rmin..Rmax`, define a test point's normalized position as
`(Rtest - Rmin) / (Rmax - Rmin)`. The canonical procedure requires three
distinct precision references in these bands:

- low: 0–10% of the envelope span;
- middle: 45–55%; and
- high: 90–100%.

The nominal 0 °C resistance is not automatically a middle-range test point.
Actual characterized resistor values may differ from convenient planning
values as long as they fall inside the required bands and their uncertainty is
recorded.

Each point uses at least 20 consecutive measurements and a predeclared physical
acceptance budget appropriate to the actual `RREF`, resistance standard,
wiring, and platform. All three points must pass before a record can claim
`range_validated`.

A real-family test then checks the intended sensor family and wiring on physical
hardware, preferably against an independent resistance measurement. It does not
need to reproduce the RTD's entire temperature range because the precision
resistance points already establish full-envelope acquisition coverage.

Validation remains platform- and configuration-scoped: a Pi/Python result does
not silently validate HERO/C, another `RREF`, another wire count, or another RTD
family.

## Recording evidence reproducibly

The 0.3 validation workflow includes tracked templates and small repository
helpers so bench evidence can be retained as structured files rather than
reconstructed from copied terminal output.

Create a local record first:

```sh
uv run --locked python -m validation.create_record <record-id>
```

This creates a versioned `record.md` template and an `environment.json` file
under `.rtd-acquire-local/validation/<record-id>/`. The automatic environment
metadata captures software/runtime facts needed for reproduction but does not
collect the hostname or user name.

For Linux/Python MAX31865 runs, the repository also provides
`validation.capture_max31865`. It writes every acquisition attempt as JSONL,
including normalized diagnostics/native evidence or an acquisition error, plus
a summary JSON file and a capture manifest containing the exact driver/SPI
configuration and SHA-256 file hashes.

The capture helper intentionally does not decide whether a run passes.
Acceptance budgets must still be declared before final measurements and the
reviewed conclusion remains explicit in the validation record.

See the detailed
[`validation/README.md`](https://github.com/GregRR/rtd-acquire/blob/main/validation/README.md)
and canonical procedure for commands and file semantics.

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
    remains pending. The 0.3 MAX31865 family records likewise remain
    `not_validated` until the applicable range/family procedure is completed.
    Implemented/toolchain-verified support must therefore stay distinct from
    physically validated hardware.
