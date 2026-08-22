"""MAX31865 acquisition support."""

from .config import MAX31865Config
from .driver import MAX31865, MAX31865Timing

__all__ = ["MAX31865", "MAX31865Config", "MAX31865Timing"]
