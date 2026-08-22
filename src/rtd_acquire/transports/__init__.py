"""Transport contracts used by hardware-specific acquisition drivers."""

from .spi import SpiBitOrder, SpiDevice, SpiSettings

__all__ = ["SpiBitOrder", "SpiDevice", "SpiSettings"]
