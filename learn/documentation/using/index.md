# Using rtd-acquire

Most application code only needs to understand three ideas:

1. an `AcquisitionDevice` can produce one `Measurement` with `read()`;
2. a `Measurement` contains resistance plus acquisition trust information; and
3. acquisition-operation failures are separate from device-reported warning or
   fault measurements.

For a quick first run, use the generic simulator. When you later substitute a
real acquisition device, the application-facing measurement contract remains
the same.

Continue with:

- [Installation](installation.md)
- [Measurement results](measurement-results.md)
- [Reading devices](reading-devices.md)
- [Errors and status](errors-status.md)
