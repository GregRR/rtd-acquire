---
title: API Reference
---

# API Reference

This section is a fast lookup for the public Python interfaces in the first
`rtd-acquire` alpha. For concepts and task-oriented explanations, use the main
[Documentation](../documentation/index.md).

## Public API groups

- [Core contracts](core.md) — `AcquisitionDevice`, `Measurement`, diagnostics,
  and exceptions.
- [`max31865`](max31865.md) — MAX31865 configuration, driver, timing, and SPI
  emulator.
- [`simulation`](simulation.md) — deterministic generic acquisition simulation.
- [`transports`](transports.md) — SPI contracts and Linux `spidev` adapter.

!!! note "Alpha API"
    `0.1.0a1` is a prerelease. Public interfaces may change before `0.1.0`.

## From resistance to temperature

The API stops at acquisition. For RTD model APIs and interactive model
experiments, continue to `rtd-sensor`.

[Visit the RTD Playground](https://gregrr.github.io/rtd-sensor/playground/){ .md-button .md-button--primary }
