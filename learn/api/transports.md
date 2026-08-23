# `rtd_acquire.transports`

## `SpiBitOrder`

- `SpiBitOrder.MSB_FIRST` (`"msb_first"`)
- `SpiBitOrder.LSB_FIRST` (`"lsb_first"`)

## `SpiSettings`

```text
SpiSettings(
    clock_polarity: Literal[0, 1],
    clock_phase: Literal[0, 1],
    clock_frequency_hz: int,
    bit_order: SpiBitOrder = SpiBitOrder.MSB_FIRST,
    bits_per_word: int = 8,
    chip_select_active_low: bool = True,
)
```

Represents the observable settings of one configured SPI connection.

## `SpiDevice`

A structural protocol with:

```python
@property
def settings(self) -> SpiSettings: ...

def transfer(self, tx: bytes, /) -> bytes: ...
```

One `transfer` call represents one contiguous full-duplex transaction. The
transport owns chip-select assertion/deassertion for that transaction.

## `LinuxSpidevDevice`

```text
LinuxSpidevDevice(
    device_path: str | os.PathLike[str],
    settings: SpiSettings,
)
```

The normal public constructor takes the device path and SPI settings. The
adapter requires the optional `spidev` backend at runtime.

Public interface:

- `settings`
- `device_path`
- `transfer(tx)`
- `close()`
- context-manager support

Repeated `close()` calls are harmless. Transfers after close raise
`AcquisitionError`.
