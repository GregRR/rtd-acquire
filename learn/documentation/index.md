---
title: Documentation
---

# Documentation

This section explains how to use `rtd-acquire` and, just as importantly, where
its responsibility ends.

## Using rtd-acquire

Start here for installation, the `Measurement` result contract, device reads,
and error handling.

- [Using rtd-acquire](using/index.md)
- [Installation](using/installation.md)
- [Measurement results](using/measurement-results.md)
- [Reading devices](using/reading-devices.md)
- [Errors and status](using/errors-status.md)

## Hardware & acquisition

Understand the hardware-facing side of the package and the first supported
converter path.

- [Hardware & acquisition](hardware/index.md)
- [MAX31865](hardware/max31865.md)
- [Raspberry Pi / Linux SPI](hardware/raspberry-pi.md)
- [Arduino AVR / HERO adapter](../api/c/arduino-avr.md)
- [The acquisition boundary](hardware/acquisition-boundary.md)

## Measurement & diagnostics

Learn how trustworthy resistance, warning/fault state, normalized diagnostics,
native evidence, and resistance uncertainty fit together.

- [Measurement & diagnostics](measurement-diagnostics/index.md)
- [Diagnostics](measurement-diagnostics/diagnostics.md)
- [Native evidence](measurement-diagnostics/native-evidence.md)
- [Resistance uncertainty](measurement-diagnostics/uncertainty.md)

## Simulation & testing

Exercise application logic and the MAX31865 driver without physical hardware.

- [Simulation & testing](simulation-testing/index.md)
- [Simulated acquisition](simulation-testing/simulation.md)
- [MAX31865 SPI emulator](simulation-testing/max31865-emulator.md)

## Integration

Connect acquisition to `rtd-sensor` or implement another acquisition device
without collapsing the package boundaries.

- [Integration](integration/index.md)
- [rtd-sensor](integration/rtd-sensor.md)
- [Implementing acquisition devices](integration/acquisition-devices.md)
- [Hardware integration](integration/hardware-integration.md)

[Visit the RTD Playground](https://gregrr.github.io/rtd-sensor/playground/){ .md-button .md-button--primary }

The Playground remains part of **rtd-sensor**. It is the companion place to
experiment with RTD curves and temperature interpretation after `rtd-acquire`
has produced a resistance.
