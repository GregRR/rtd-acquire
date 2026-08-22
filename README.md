# rtd-acquire

`rtd-acquire` is a hardware-agnostic acquisition layer for resistance
temperature detectors (RTDs). Its job is to obtain the best trustworthy
estimate of an RTD element's resistance from real or simulated acquisition
hardware and report acquisition-level diagnostics.

It intentionally stops at resistance. RTD characteristic interpretation,
resistance-to-temperature conversion, tolerance, model calibration, and
model-level uncertainty belong in `rtd-sensor`.

```text
physical RTD
    ↓
converter / ADC / transmitter / controller
    ↓
rtd-acquire
    ↓
resistance + acquisition diagnostics
    ↓
rtd-sensor
    ↓
temperature / RTD-model interpretation
```

## Initial targets

The first implementation target is the Analog Devices MAX31865, with Python
hardware testing on a Raspberry Pi 4 Model B and portable C hardware testing on
Arduino-compatible HERO boards.

The second planned hardware family is the TI ADS124S08 precision ADC/front end.
Later targets cover industrial resistance inputs, 4–20 mA transmitters,
industrial digital interfaces, and configurable custom acquisition circuits.

See:

- [DESIGN.md](docs/DESIGN.md) — architecture and project contracts
- [ROADMAP.md](docs/ROADMAP.md) — implementation sequence
- [HARDWARE.md](docs/HARDWARE.md) — acquisition hardware catalog
- [DIAGNOSTICS.md](docs/DIAGNOSTICS.md) — native diagnostic survey and normalization research

## Status

Pre-implementation design scaffold. Public APIs are not yet released or stable.

## License

Mozilla Public License 2.0 (MPL-2.0), matching `rtd-sensor`.
