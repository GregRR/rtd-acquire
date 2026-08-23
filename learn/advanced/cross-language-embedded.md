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

The portable core is intended to remain usable on constrained systems and
across MCU families. Platform adapters can supply capabilities such as SPI while
the device driver remains independent of Arduino-specific or vendor-specific
headers.

## Numeric differences

Python normally evaluates these paths with binary64 floating point. Many MCU
targets will prefer or default to binary32. The project therefore treats
numeric acceptance as an explicit conformance topic rather than assuming
bit-for-bit equality across every target.

Cross-language claims should be made feature-by-feature as the C implementation
actually consumes the corresponding shared vectors.
