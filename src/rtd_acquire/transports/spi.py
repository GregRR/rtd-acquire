"""Minimal SPI transport contract for acquisition drivers."""

from __future__ import annotations

from dataclasses import dataclass
from enum import StrEnum
from typing import Literal, Protocol

from ..core.errors import ConfigurationError


class SpiBitOrder(StrEnum):
    """Bit order used within each SPI word."""

    MSB_FIRST = "msb_first"
    LSB_FIRST = "lsb_first"


@dataclass(frozen=True, slots=True)
class SpiSettings:
    """Observable settings of one configured SPI device connection."""

    clock_polarity: Literal[0, 1]
    clock_phase: Literal[0, 1]
    clock_frequency_hz: int
    bit_order: SpiBitOrder = SpiBitOrder.MSB_FIRST
    bits_per_word: int = 8
    chip_select_active_low: bool = True

    def __post_init__(self) -> None:
        if self.clock_polarity not in (0, 1):
            raise ConfigurationError("clock_polarity must be 0 or 1")
        if self.clock_phase not in (0, 1):
            raise ConfigurationError("clock_phase must be 0 or 1")
        if self.clock_frequency_hz <= 0:
            raise ConfigurationError("clock_frequency_hz must be greater than zero")
        if self.bits_per_word <= 0:
            raise ConfigurationError("bits_per_word must be greater than zero")


class SpiDevice(Protocol):
    """A configured SPI peripheral connection with transaction-owned CS.

    One ``transfer`` call is one contiguous full-duplex SPI transaction. The
    implementation owns chip-select assertion/deassertion and keeps chip select
    active for the entire transfer. A successful call returns exactly one
    received byte for each transmitted byte.
    """

    @property
    def settings(self) -> SpiSettings:
        """Return the settings currently used for this SPI connection."""
        ...

    def transfer(self, tx: bytes, /) -> bytes:
        """Perform one full-duplex transaction and return received bytes."""
        ...
