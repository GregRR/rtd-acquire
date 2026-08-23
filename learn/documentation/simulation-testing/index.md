# Simulation & testing

`rtd-acquire` provides two deliberately different hardware-free tools.

## Generic simulated acquisition

`SimulatedAcquisitionDevice` operates at the public measurement boundary. It is
best for application tests that need deterministic measurements, warnings,
faults, or acquisition-operation failures.

## MAX31865 SPI emulator

`MAX31865SpiEmulator` operates below the driver at the register/SPI boundary. It
lets the real MAX31865 driver sequence execute without a physical converter.

Neither tool is an RTD physics simulator. They do not infer temperature, model
noise, or simulate an analog sensor circuit.

Continue with:

- [Simulated acquisition](simulation.md)
- [MAX31865 SPI emulator](max31865-emulator.md)
