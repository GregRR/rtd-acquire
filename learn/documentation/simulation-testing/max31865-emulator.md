# MAX31865 SPI emulator

`MAX31865SpiEmulator` exercises the real MAX31865 driver's SPI/register path
without physical hardware.

```python
from rtd_acquire.max31865 import (
    MAX31865,
    MAX31865Config,
    MAX31865SpiEmulator,
)

spi = MAX31865SpiEmulator(rtd_code=8192)
config = MAX31865Config(
    reference_resistance_ohms=430.0,
    wire_count=3,
    filter_frequency_hz=60,
)

measurement = MAX31865(spi, config, sleep=lambda _: None).read()
```

The emulator accepts the native 15-bit converter code and optional native fault
status. It records transactions and threshold-register writes so tests can
inspect driver behavior.

## Scope

The emulator models only the MAX31865 SPI/register behavior needed by the
driver. It intentionally does **not** model:

- temperature;
- RTD curves;
- analog settling;
- circuit noise; or
- the physical cause of a fault bit.

That keeps it deterministic and prevents a test double from masquerading as an
unvalidated electrical model.
