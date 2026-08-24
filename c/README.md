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

The portable MAX31865 C layer is implemented in
`include/rtd_acquire/max31865.h` and `src/max31865.c`. It validates the public
electrical configuration, encodes the static configuration byte and directional
threshold registers, and decodes native RTD/fault registers into the shared
caller-owned result contract. Public MAX31865 operations use a discriminated
result enum so invalid arguments, configuration errors, insufficient storage,
and later SPI/delay failures are not collapsed into one Boolean. Both existing
language-neutral MAX31865 vector families execute against Python and C. The
SPI/delay acquisition sequence, general binary64/binary32 acceptance profile,
and HERO platform adapter remain 0.2 work.
