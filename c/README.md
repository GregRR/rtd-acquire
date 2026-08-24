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

The portable result contract in `include/rtd_acquire/core.h` adds caller-owned
measurement, diagnostic, and native-evidence storage. Callers choose the array
capacities they can afford; the core does not impose one global diagnostic
maximum. `src/core.c` derives measurement status and validates the shared
Python/C result invariants.

The first MAX31865 C layer is implemented in `include/rtd_acquire/max31865.h`
and `src/max31865.c`. It validates the public electrical configuration and
encodes the static configuration byte and directional threshold registers.
The existing language-neutral threshold vectors execute against both Python
and C. Native register decoding, the SPI/delay acquisition sequence, and the
HERO platform adapter remain 0.2 work.
