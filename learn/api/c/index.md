---
title: Portable C API
---

# Portable C API

`rtd-acquire` is developing a portable C11 implementation alongside the Python
implementation. The two implementations are independent and converge on shared
observable behavior rather than one wrapping the other.

The C core is designed around:

- portable C11;
- no mandatory heap allocation;
- caller-owned or fixed storage;
- small capability-specific HAL interfaces; and
- platform adapters that keep Arduino/HERO and other platform assumptions out
  of the core.

The first frozen public C contracts are the SPI and blocking-delay HALs. Future
0.2 work will extend this area with caller-owned measurement/diagnostic result
storage and the portable MAX31865 driver.

Continue with [HAL interfaces](hal.md).

!!! note "Development API"
    The portable C API targets `rtd-acquire 0.2.0`, which is currently under
    development. It is not part of the released `0.1.0a1` package.
