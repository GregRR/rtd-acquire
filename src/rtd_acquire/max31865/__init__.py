"""MAX31865 acquisition support."""

from .config import MAX31865Config
from .driver import MAX31865, MAX31865Timing
from .emulator import MAX31865SpiEmulator

__all__ = [
    "MAX31865",
    "MAX31865Config",
    "MAX31865SpiEmulator",
    "MAX31865Timing",
]
