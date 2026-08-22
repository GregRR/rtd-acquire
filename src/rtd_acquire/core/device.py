"""Public acquisition-device contract."""

from __future__ import annotations

from typing import Protocol

from .measurement import Measurement


class AcquisitionDevice(Protocol):
    """Something capable of producing RTD resistance measurements.

    The base contract does not require ``read()`` to be thread-safe or
    reentrant. Callers must serialize access to a device instance unless that
    implementation explicitly documents a stronger guarantee.
    """

    def read(self) -> Measurement:
        """Acquire and return one resistance measurement."""
        ...
