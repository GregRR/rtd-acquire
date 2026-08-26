---
title: Portable C API
---

# Portable C API

`rtd-acquire 0.2.0` includes a portable C11 implementation alongside the Python
implementation. The two implementations are independent and converge on shared
observable behavior rather than one wrapping the other.

The C core is designed around:

- portable C11;
- no mandatory heap allocation;
- caller-owned or fixed storage;
- small capability-specific HAL interfaces; and
- platform adapters that keep Arduino/HERO and other platform assumptions out
  of the core.

The public C contracts include the SPI and blocking-delay
HALs, caller-owned core measurement/diagnostic result storage, the portable
MAX31865 driver, and the first concrete Arduino AVR / HERO platform adapter.

Continue with:

- [Core result contracts](core.md)
- [HAL interfaces](hal.md)
- [MAX31865](max31865.md)
- [Arduino AVR / HERO adapter](arduino-avr.md)

!!! note "Distribution"
    The portable C implementation ships in the `0.2.0` source distribution and
    repository. The Python wheel remains Python-only; use the sdist or repository
    when integrating the C sources into an embedded build.
