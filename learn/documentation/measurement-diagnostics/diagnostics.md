# Diagnostics

**Introduced in:** `rtd-acquire 0.1.0a1`

Different acquisition devices use different terminology for similar conditions.
`rtd-acquire` maps supported observations to stable `DiagnosticCode` values so
application logic does not have to key off vendor-specific strings.

Examples include:

- sensor/open/short/input-fault conditions;
- high and low resistance thresholds;
- overrange and underrange;
- reference and input-voltage faults;
- ADC/PGA electrical conditions;
- CRC, register, memory, and SPI integrity errors; and
- generic configuration or hardware faults when the native evidence does not
  justify a more specific claim.

## Severity

Every diagnostic has a `warning` or `fault` severity.

A warning can coexist with a usable resistance. A fault means the measurement
cannot carry a trustworthy resistance.

## Do not infer more than the hardware reports

Normalization is evidence-driven. For example, physically disconnecting a wire
may be known to the person running a test, but the driver should not report a
specific `sensor_circuit_open` code unless the device's observable native state
supports that interpretation.

The acquisition layer should preserve useful evidence without turning a coarse
vendor status into a more specific physical diagnosis than the hardware
actually established.
