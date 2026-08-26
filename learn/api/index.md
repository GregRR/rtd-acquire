---
title: API Reference
---

# API Reference

This section is a fast lookup for the public interfaces provided by
`rtd-acquire`. For concepts and task-oriented explanations, use the main
[Documentation](../documentation/index.md).

## Python API

The Python API in the current `rtd-acquire 0.2.0` release includes:

- [Core contracts](core.md) — `AcquisitionDevice`, `Measurement`, diagnostics,
  and exceptions.
- [`max31865`](max31865.md) — MAX31865 configuration, driver, timing, and SPI
  emulator.
- [`simulation`](simulation.md) — deterministic generic acquisition simulation.
- [`transports`](transports.md) — SPI contracts and Linux `spidev` adapter.

## Portable C API

The independent portable C11 implementation is included in `rtd-acquire 0.2.0`.
Its public contracts live in a separate API area so the C interfaces are not
mixed into the Python module reference.

- [Portable C API overview](c/index.md) — portability and ownership rules.
- [Core result contracts](c/core.md) — caller-owned measurement, diagnostic,
  and native-evidence storage.
- [HAL interfaces](c/hal.md) — capability-specific SPI and blocking-delay
  contracts.
- [MAX31865](c/max31865.md) — portable C configuration, threshold encoding,
  native decoding, and fault-checked one-shot acquisition.
- [Arduino AVR / HERO](c/arduino-avr.md) — UNO-class SPI and blocking-delay
  bindings for the portable C driver.

!!! note "Portable C distribution"
    The portable C sources, tests, conformance artifacts, and Arduino AVR / HERO
    adapter ship in the `0.2.0` source distribution and repository. The Python
    wheel intentionally contains only the Python package.

## From resistance to temperature

The API stops at acquisition. For RTD model APIs and interactive model
experiments, continue to `rtd-sensor`.

[Visit the RTD Playground](https://gregrr.github.io/rtd-sensor/playground/){ .md-button .md-button--primary }
