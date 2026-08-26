"""Linux ``spidev`` adapter for configured SPI device connections.

Implementation basis: Raspberry Pi SPI documentation, Linux spidev userspace
API documentation, and Python spidev 3.8; see ``docs/REFERENCES.md``.
"""

from __future__ import annotations

import importlib
import os
from collections.abc import Callable
from contextlib import suppress
from types import TracebackType
from typing import Protocol, cast

from ..core import AcquisitionError, ConfigurationError
from .spi import SpiBitOrder, SpiSettings


class _SpiDevBackend(Protocol):
    """Subset of Python spidev used by the adapter."""

    max_speed_hz: int
    mode: int
    lsbfirst: bool
    bits_per_word: int
    cshigh: bool
    no_cs: bool

    def open_path(self, path: str) -> None: ...

    def xfer2(self, values: list[int]) -> list[int]: ...

    def close(self) -> None: ...


class _SpidevModule(Protocol):
    SpiDev: Callable[[], _SpiDevBackend]


def _new_spidev_backend() -> _SpiDevBackend:
    try:
        module = importlib.import_module("spidev")
    except ModuleNotFoundError as exc:
        if exc.name == "spidev":
            raise ConfigurationError(
                "Linux SPI integration requires the optional spidev package; "
                "install rtd-acquire[raspberry-pi]"
            ) from exc
        raise
    except ImportError as exc:
        raise AcquisitionError("could not import the spidev backend") from exc

    typed_module = cast(_SpidevModule, module)
    return typed_module.SpiDev()


class LinuxSpidevDevice:
    """Configured Linux SPI device backed by the kernel ``spidev`` API.

    This adapter is intentionally Linux-generic. Raspberry Pi 4 and Raspberry
    Pi 5 both reach it through their kernel SPI controller drivers and the same
    userspace ``/dev/spidev*`` interface, so no SoC-specific register access is
    present here.
    """

    def __init__(
        self,
        device_path: str | os.PathLike[str],
        settings: SpiSettings,
        *,
        _backend: _SpiDevBackend | None = None,
    ) -> None:
        path = os.fspath(device_path)
        if not path:
            raise ConfigurationError("device_path must not be empty")

        self._device_path = path
        self._settings = settings
        self._backend = _backend if _backend is not None else _new_spidev_backend()
        self._closed = False

        try:
            self._backend.open_path(path)
            self._backend.mode = (settings.clock_polarity << 1) | settings.clock_phase
            self._backend.max_speed_hz = settings.clock_frequency_hz
            self._backend.lsbfirst = settings.bit_order is SpiBitOrder.LSB_FIRST
            self._backend.bits_per_word = settings.bits_per_word
            self._backend.cshigh = not settings.chip_select_active_low
            self._backend.no_cs = False
        except Exception as exc:
            self._close_after_failed_open()
            raise AcquisitionError(
                f"could not configure Linux SPI device {path!r}"
            ) from exc

    @property
    def settings(self) -> SpiSettings:
        """Return the effective settings requested for this connection."""

        return self._settings

    @property
    def device_path(self) -> str:
        """Return the opened spidev device path."""

        return self._device_path

    def transfer(self, tx: bytes, /) -> bytes:
        """Perform one full-duplex SPI transaction with transport-owned CS."""

        if self._closed:
            raise AcquisitionError("Linux SPI device is closed")

        try:
            response = self._backend.xfer2(list(tx))
        except Exception as exc:
            raise AcquisitionError(
                f"SPI transfer failed on {self._device_path!r}"
            ) from exc

        if len(response) != len(tx):
            raise AcquisitionError(
                "spidev transfer returned an unexpected number of bytes"
            )
        try:
            return bytes(response)
        except (TypeError, ValueError) as exc:
            raise AcquisitionError(
                "spidev transfer returned invalid byte values"
            ) from exc

    def close(self) -> None:
        """Close the Linux SPI device; repeated calls are harmless."""

        if self._closed:
            return
        try:
            self._backend.close()
        except Exception as exc:
            raise AcquisitionError(
                f"could not close Linux SPI device {self._device_path!r}"
            ) from exc
        self._closed = True

    def __enter__(self) -> LinuxSpidevDevice:
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc_value: BaseException | None,
        traceback: TracebackType | None,
    ) -> None:
        self.close()

    def _close_after_failed_open(self) -> None:
        with suppress(Exception):
            self._backend.close()
        self._closed = True
