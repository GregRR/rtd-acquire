"""Public acquisition-device contract."""

from __future__ import annotations

from typing import Protocol

from .measurement import Measurement


class AcquisitionDevice(Protocol):
    """Something capable of producing RTD resistance measurements."""

    def read(self) -> Measurement:
        """Acquire and return one resistance measurement."""
        ...
