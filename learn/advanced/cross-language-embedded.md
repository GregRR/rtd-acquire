# Cross-language & embedded

The long-term architecture is:

```text
Python implementation ─┐
                       ├─> shared behavioral contracts / conformance
portable C implementation ─┘
```

The implementations do not need identical source structure. They need to agree
where the project declares shared observable semantics.

## Embedded priorities

The portable C foundation is being introduced in `rtd-acquire 0.2.0`.

The portable core is intended to remain usable on constrained systems and
across MCU families. Platform adapters supply small capabilities such as SPI and
blocking relative delay while device drivers remain independent of
Arduino-specific or vendor-specific headers.

SPI chip-select handling stays inside the SPI adapter, and the delay capability
is only a blocking microsecond delay. The core does not require a generic GPIO
HAL, clock service, scheduler, or heap allocator merely to support MAX31865.

The 0.2 C result contract likewise uses caller-owned diagnostic and
native-evidence arrays. Capacity is selected by the caller instead of being a
single library-wide maximum, allowing constrained targets to reserve only the
storage they need.

The portable MAX31865 driver now composes those capabilities directly: SPI and
blocking delay are injected by the platform adapter, while configuration, timing,
register sequencing, VBIAS cleanup, and native decoding remain platform-neutral
C11 code.

**Arduino AVR / HERO adapter introduced in:** `rtd-acquire 0.2.0`

The first concrete adapter targets the Arduino AVR / UNO-class core used by
HERO. It supplies SPI and delay capabilities without changing the portable
MAX31865 implementation. CI compiles the same adapter/example with the real
Arduino Uno core while host tests use minimal stubs, demonstrating that the
platform boundary can be tested independently from both hardware and the C11
device logic.

## Numeric differences

Python normally evaluates these paths with binary64 floating point. Many MCU
targets will prefer or default to binary32. The project therefore treats
numeric acceptance as an explicit conformance topic rather than assuming
bit-for-bit equality across every target.

Cross-language claims are made feature-by-feature as the C implementation
actually consumes the corresponding shared vectors. Both current MAX31865
vector families execute against Python and C.

**Binary64/binary32 profile introduced in:** `rtd-acquire 0.2.0`

Threshold encoding keeps exact integer register outputs. MAX31865 decoded
resistance uses the frozen `python-binary64-c-binary32` profile: expected zero
is exact and nonzero resistance allows at most `2^-22` relative difference.
Everything else in the measurement contract remains exact. Cross-language
configuration cases must also be stable after binary32 input conversion rather
than relying on sub-binary32 distinctions.
