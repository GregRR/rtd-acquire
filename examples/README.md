# Examples

Examples preserve the package boundary:

`physical RTD -> rtd-acquire -> resistance -> rtd-sensor`

## `rtd_sensor_pt100.py`

This example passes a resistance obtained through the public
`AcquisitionDevice` contract to the separate `rtd-sensor` package for Pt100
temperature interpretation. It uses `SimulatedAcquisitionDevice` so the example
is runnable without hardware; a real device can be substituted without changing
the handoff to `rtd-sensor`.

`rtd-sensor` is intentionally not an `rtd-acquire` runtime dependency. From a
source checkout, it can be supplied only for the example:

```sh
uv run --with rtd-sensor python examples/rtd_sensor_pt100.py
```
