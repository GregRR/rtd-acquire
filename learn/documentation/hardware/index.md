# Hardware & acquisition

`rtd-acquire` sits between hardware-specific measurement systems and
RTD-model interpretation.

The first implemented hardware path is the Analog Devices MAX31865, reached
through an injected SPI transport. On Raspberry Pi, the first host adapter uses
the standard Linux `spidev` userspace interface.

The project is intentionally broader than one converter. Future acquisition
families can include precision ADC/front ends, industrial resistance inputs,
transmitters, and digital industrial interfaces while preserving the same
high-level measurement boundary.

Continue with:

- [MAX31865](max31865.md)
- [Raspberry Pi / Linux SPI](raspberry-pi.md)
- [The acquisition boundary](acquisition-boundary.md)
