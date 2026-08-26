# Hardware & acquisition

`rtd-acquire` sits between hardware-specific measurement systems and
RTD-model interpretation.

The first implemented converter path is the Analog Devices MAX31865, reached
through an injected SPI transport/HAL. On Raspberry Pi, the Python host adapter
uses the standard Linux `spidev` userspace interface. `rtd-acquire 0.2.0` also
adds the portable C11 MAX31865 path and an Arduino AVR / UNO-class adapter for
the inventr.io HERO.

The project is intentionally broader than one converter. Future acquisition
families can include precision ADC/front ends, industrial resistance inputs,
transmitters, and digital industrial interfaces while preserving the same
high-level measurement boundary.

Continue with:

- [MAX31865](max31865.md)
- [Raspberry Pi / Linux SPI](raspberry-pi.md)
- [Arduino AVR / HERO adapter](../../api/c/arduino-avr.md)
- [The acquisition boundary](acquisition-boundary.md)
