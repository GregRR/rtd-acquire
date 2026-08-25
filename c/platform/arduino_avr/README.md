# Arduino AVR / HERO adapter

This adapter binds the portable `rtd-acquire` C HALs to the Arduino AVR core
used by UNO R3-class boards, including the inventr.io HERO board.

The adapter itself is C++ because Arduino's `SPIClass`/`SPISettings` API is C++,
while the reusable acquisition core remains portable C11.

It provides:

- an SPI adapter using `SPI.beginTransaction()`, byte transfers, and a
  caller-selected chip-select GPIO;
- effective AVR SPI-clock reporting using the same discrete divider model as
  ArduinoCore-avr; and
- a blocking delay adapter that splits `uint32_t` microsecond waits across
  `delay()` and `delayMicroseconds()` so long MAX31865 waits do not overflow an
  AVR `unsigned int`.

The adapter currently targets the Arduino AVR / UNO-class core rather than all
Arduino architectures. Physical HERO + MAX31865 validation remains a separate
roadmap item.

The 0.2 software scope intentionally does not add a separate C++ convenience
wrapper. Arduino sketches use the portable C MAX31865 API through this adapter;
a wrapper can be reconsidered later if hardware or user feedback identifies a
concrete usability problem that cannot be solved without another public layer.

See `examples/max31865_read/max31865_read.ino` for the minimal acquisition
wiring at the software API level. The repository CI stages the portable C core
and this adapter as a temporary Arduino library and compiles that sketch for
`arduino:avr:uno`.
