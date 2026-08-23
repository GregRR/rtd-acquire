# Portable C implementation

This directory contains the portable C side of the shared `rtd-acquire`
behavioral contract and platform/HAL adapters.

The first frozen C capability is the SPI HAL in
`include/rtd_acquire/spi.h`. It mirrors the semantics of the Python SPI
transport without requiring identical language-level APIs: one transfer call
is one complete SPI transaction, the adapter owns chip select, and the driver
can inspect the effective SPI settings.

The second frozen capability is the blocking delay HAL in
`include/rtd_acquire/delay.h`. It accepts caller-owned context and integral
microsecond durations; successful adapters must not return before the requested
interval has elapsed. Host contract tests exercise both HALs without platform
headers or dynamic allocation.

The MAX31865 C driver, caller-owned measurement/diagnostic structures, and HERO
platform adapter remain 0.2 work.
