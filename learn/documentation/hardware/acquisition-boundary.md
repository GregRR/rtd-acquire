# The acquisition boundary

The most important architectural rule in `rtd-acquire` is the boundary between
**measuring resistance** and **interpreting an RTD model**.

**Boundary clarification introduced in:** `rtd-acquire 0.3.0`

`rtd-sensor` starts once a trustworthy estimate of RTD-element resistance in
ohms is available. Sometimes `rtd-acquire` must produce that estimate; sometimes
another instrument or interface already provides it.

## Path A — acquisition work is still required

```text
physical RTD
    ↓
raw converter / ADC / electrical observations
    ↓
rtd-acquire
    ↓
resistance + acquisition diagnostics
    ↓
rtd-sensor
    ↓
temperature / RTD-model interpretation
```

A MAX31865 is the current concrete example. Register data, reference-resistor
scaling, configuration, and native faults still need an acquisition layer before
an RTD model should interpret the result.

## Path B — resistance is already available

```text
physical RTD
    ↓
instrument / RTD interface / DAQ
    ↓
trustworthy RTD-element resistance in ohms
    ↓
rtd-sensor
```

This path does not require an `rtd-acquire` driver merely because physical
hardware was involved. A multimeter, resistance bridge, DAQ, industrial input,
or ready-made RTD interface can feed `rtd-sensor` directly when its resistance
semantics are appropriate for the model-layer input.

## Temperature-only devices

```text
physical RTD
    ↓
smart device performs its own RTD interpretation
    ↓
temperature only
```

If the chosen interface exposes only internally calculated temperature, there is
no original resistance result for `rtd-acquire` to preserve or for `rtd-sensor`
to reinterpret. `rtd-acquire` should not reverse that temperature into a
synthetic resistance just to force the device through the acquisition contract.

## Why keep the layers separate?

A MAX31865 does not need to know whether its resistance will later be
interpreted as a Pt100, Pt1000, or another compatible RTD. Likewise,
`rtd-sensor` should not need SPI, GPIO, ADC, or transmitter knowledge to apply a
sensor model.

That separation provides several practical benefits:

- acquisition drivers can be tested against electrical behavior independently
  of RTD curves;
- one RTD model can consume measurements from many hardware paths;
- new acquisition hardware does not require duplicating temperature science;
- diagnostics stay tied to evidence the acquisition device actually exposes;
  and
- simulation can target either the acquisition boundary or RTD-model behavior
  without pretending they are the same problem.

## What belongs in rtd-acquire?

Electrical acquisition configuration, communication with acquisition hardware,
resistance scaling, acquisition diagnostics, native evidence, and quantified
resistance uncertainty when the acquisition path supports it.

## What belongs in rtd-sensor?

RTD family/model selection, resistance-to-temperature conversion, model
validity ranges, RTD tolerance, calibration/fitting, and model-level uncertainty
propagation.

See the [rtd-sensor documentation](https://gregrr.github.io/rtd-sensor/) for the
model and RTD-interpretation side of this boundary.
