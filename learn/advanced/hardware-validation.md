# Hardware validation

Unit tests, emulators, and conformance vectors establish software behavior.
They do not establish that a real converter, wiring setup, Linux SPI path, and
RTD work together correctly.

The project therefore keeps physical validation as a separate gate.

## Initial target

The first formal target is a Raspberry Pi 4 using the real Linux
`LinuxSpidevDevice` path with a MAX31865, known resistance references, and a
real Pt100.

The validation procedure requires, among other things:

- successful communication through the real Linux SPI adapter;
- repeated known-resistance measurements judged against a predeclared
  acceptance budget;
- repeatable real-Pt100 resistance measurements;
- at least one safe native-fault observation and correct normalization; and
- no unexplained acquisition failures during the run.

## Do not substitute plausibility for accuracy

A room-temperature Pt100 producing a plausible-looking value is useful as a
sanity check, but it is not an accuracy reference. Known resistance references
and their tolerances/uncertainties are needed for a defensible validation claim.

The canonical detailed procedure remains in
[`docs/HARDWARE_VALIDATION.md`](https://github.com/GregRR/rtd-acquire/blob/main/docs/HARDWARE_VALIDATION.md).

!!! warning "Current status"
    `0.1.0a1` was released before this physical validation gate was completed.
    The docs therefore distinguish implemented support from validated hardware.
