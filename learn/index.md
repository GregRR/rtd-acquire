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

The current software includes:

- the core `AcquisitionDevice` and `Measurement` contracts;
- normalized acquisition diagnostics with preserved native evidence;
- a platform-independent Python MAX31865 driver and Linux `spidev` adapter;
- deterministic generic simulation and a MAX31865 SPI emulator;
- an independent portable C11 MAX31865 implementation with caller-owned
  result storage;
- an Arduino AVR / HERO platform adapter; and
- language-neutral Python/C conformance vectors with an explicit binary32
  numeric acceptance profile.

## What rtd-acquire is not

`rtd-acquire` does **not** decide whether a resistance belongs to a Pt100,
Pt1000, Ni120, or another RTD model, and it does not convert resistance to
temperature. Those are model-level questions handled by
[`rtd-sensor`](https://gregrr.github.io/rtd-sensor/).

Keeping acquisition separate from RTD interpretation lets each layer do one job
well. Hardware drivers can report resistance without embedding assumptions
about sensor curves, while the same `rtd-sensor` model can consume resistance
from many different acquisition systems.

!!! note "Pre-1.0 status"
    `0.2.0` is a pre-1.0 release, so public APIs may still change before `1.0`.
    Physical Raspberry Pi/MAX31865 and HERO/MAX31865 validation is still
    pending and is documented separately from the software/toolchain gates.

## Let's go

[Full rtd-acquire Documentation](documentation/){ .md-button .md-button--docs }

[Visit the RTD Playground](https://gregrr.github.io/rtd-sensor/playground/){ .md-button .md-button--primary }

The **RTD Playground lives at rtd-sensor** because it explores RTD behavior,
models, and temperature interpretation. Use this site for acquisition and
hardware-facing documentation; use the Playground when you want to experiment
with what an acquired resistance means.

If you already know Python and want the shortest route into the package, start
with [Start Here](start-here.md).
