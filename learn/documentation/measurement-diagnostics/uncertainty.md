# Resistance uncertainty

`Measurement.standard_uncertainty_ohms` is the acquisition layer's optional
place to report a **standard uncertainty in the resistance result**.

```python
measurement.standard_uncertainty_ohms
```

A numeric value means the acquisition path has quantified that contribution in
ohms. `None` means it has not supplied a quantified value.

## `None` is not zero

Absence of a quantified uncertainty does not imply perfect measurement. It only
means the acquisition result does not carry a standard-uncertainty estimate.

## Keep uncertainty layers distinct

This field concerns the acquired **resistance**. If an application later uses
`rtd-sensor` to convert resistance to temperature, that package can propagate
resistance uncertainty through the selected RTD model and combine it with other
model-level uncertainty contributions.

That separation makes the provenance of uncertainty clearer: acquisition
hardware establishes resistance uncertainty; RTD modeling establishes how that
uncertainty affects temperature interpretation.
