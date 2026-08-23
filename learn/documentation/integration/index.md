# Integration

The public contracts are designed so `rtd-acquire` can sit cleanly between
hardware and higher-level software.

Typical integration patterns include:

- acquire resistance, then pass it to `rtd-sensor`;
- build application logic against the generic `AcquisitionDevice` protocol;
- add another hardware driver while retaining the same `Measurement` contract;
  or
- provide a platform transport such as SPI without embedding device logic in
  the transport itself.

Continue with:

- [rtd-sensor](rtd-sensor.md)
- [Implementing acquisition devices](acquisition-devices.md)
- [Hardware integration](hardware-integration.md)
