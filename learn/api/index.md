---
title: API Reference
---

# API Reference

This section is a fast lookup for the public interfaces provided by
`rtd-acquire`. For concepts and task-oriented explanations, use the main
[Documentation](../documentation/index.md).

## Python API

The released Python API currently reflects `rtd-acquire 0.1.0a1`:

- [Core contracts](core.md) — `AcquisitionDevice`, `Measurement`, diagnostics,
  and exceptions.
- [`max31865`](max31865.md) — MAX31865 configuration, driver, timing, and SPI
  emulator.
- [`simulation`](simulation.md) — deterministic generic acquisition simulation.
- [`transports`](transports.md) — SPI contracts and Linux `spidev` adapter.

## Portable C API

The independent portable C11 implementation is being developed for
`rtd-acquire 0.2.0`. Its public contracts live in a separate API area so the C
interfaces are not mixed into the Python module reference.

- [Portable C API overview](c/index.md) — portability and ownership rules.
- [Core result contracts](c/core.md) — caller-owned measurement, diagnostic,
  and native-evidence storage.
- [HAL interfaces](c/hal.md) — capability-specific SPI and blocking-delay
  contracts.
- [MAX31865](c/max31865.md) — portable C configuration, threshold encoding,
  and native measurement decoding.

!!! note "Development API"
    `0.2.0` is under development. The portable C interfaces documented here are
    present on the development branch but are not part of the released
    `0.1.0a1` package.

## From resistance to temperature

The API stops at acquisition. For RTD model APIs and interactive model
experiments, continue to `rtd-sensor`.

[Visit the RTD Playground](https://gregrr.github.io/rtd-sensor/playground/){ .md-button .md-button--primary }
