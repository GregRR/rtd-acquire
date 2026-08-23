# Integration with rtd-sensor

`rtd-acquire` and `rtd-sensor` are companion packages with an explicit boundary:

```text
rtd-acquire → resistance → rtd-sensor
```

They do not depend on one another at runtime.

## Pass resistance explicitly

```python
from rtd_sensor import pt100

measurement = device.read()
if measurement.resistance_ohms is not None:
    temperature_c = pt100.resistance_to_celsius(
        measurement.resistance_ohms
    )
```

This keeps the hardware acquisition reusable even when the application changes
RTD model or does not need temperature conversion at all.

## Faults stop before model interpretation

If `resistance_ohms is None`, the acquisition layer has determined that it does
not have a trustworthy resistance to hand off. Handle the acquisition
diagnostics rather than attempting temperature conversion.

## Continue learning

[Visit the RTD Playground](https://gregrr.github.io/rtd-sensor/playground/){ .md-button .md-button--primary }

The Playground is hosted by `rtd-sensor` because its experiments focus on RTD
models and what resistance means as temperature.
