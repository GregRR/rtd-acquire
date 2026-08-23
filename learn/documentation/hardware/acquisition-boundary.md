# The acquisition boundary

The most important architectural rule in `rtd-acquire` is the boundary between
**measuring resistance** and **interpreting an RTD model**.

```text
hardware evidence → rtd-acquire → resistance → rtd-sensor → temperature
```

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
