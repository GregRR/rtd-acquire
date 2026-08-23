---
title: Learn rtd-acquire
---

# Learn rtd-acquire

`rtd-acquire` is a hardware-agnostic acquisition layer for resistance
temperature detectors (RTDs). It reads acquisition hardware and returns a
trustworthy **resistance measurement**, together with acquisition diagnostics
and, when available, resistance uncertainty.

It deliberately stops at resistance.

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

## What rtd-acquire is

`rtd-acquire` gives applications a common measurement boundary across very
different RTD acquisition paths. A Raspberry Pi talking SPI to a MAX31865, a
future precision ADC backend, or an industrial resistance input can expose the
same core idea: **what resistance did the acquisition system establish, and how
trustworthy is that result?**

The first public alpha includes:

- the core `AcquisitionDevice` and `Measurement` contracts;
- normalized acquisition diagnostics with preserved native evidence;
- a platform-independent MAX31865 driver;
- a Linux `spidev` adapter for Raspberry Pi and other compatible Linux hosts;
- deterministic generic simulation and a MAX31865 SPI emulator; and
- language-neutral conformance vectors for shared behavior.

## What rtd-acquire is not

`rtd-acquire` does **not** decide whether a resistance belongs to a Pt100,
Pt1000, Ni120, or another RTD model, and it does not convert resistance to
temperature. Those are model-level questions handled by
[`rtd-sensor`](https://gregrr.github.io/rtd-sensor/).

Keeping acquisition separate from RTD interpretation lets each layer do one job
well. Hardware drivers can report resistance without embedding assumptions
about sensor curves, while the same `rtd-sensor` model can consume resistance
from many different acquisition systems.

!!! note "Alpha status"
    `0.1.0a1` is the first public alpha. Public APIs may change before `0.1.0`,
    and physical Raspberry Pi/MAX31865 validation is still pending.

## Let's go

[Full rtd-acquire Documentation](documentation/){ .md-button .md-button--docs }

[Visit the RTD Playground](https://gregrr.github.io/rtd-sensor/playground/){ .md-button .md-button--primary }

The **RTD Playground lives at rtd-sensor** because it explores RTD behavior,
models, and temperature interpretation. Use this site for acquisition and
hardware-facing documentation; use the Playground when you want to experiment
with what an acquired resistance means.

If you already know Python and want the shortest route into the package, start
with [Start Here](start-here.md).
