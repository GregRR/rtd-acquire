from __future__ import annotations

import importlib
from pathlib import Path
from typing import Literal, NoReturn

import pytest

from rtd_acquire import AcquisitionError, ConfigurationError
from rtd_acquire.transports import LinuxSpidevDevice, SpiBitOrder, SpiSettings


class FakeSpidevBackend:
    def __init__(self, *, responses: list[list[int]] | None = None) -> None:
        self.max_speed_hz = 0
        self.mode = 0
        self.lsbfirst = False
        self.bits_per_word = 0
        self.cshigh = False
        self.no_cs = True
        self.opened_path: str | None = None
        self.transfers: list[list[int]] = []
        self.close_calls = 0
        self._responses = list(responses or [])
        self.transfer_error: Exception | None = None
        self.open_error: Exception | None = None

    def open_path(self, path: str) -> None:
        if self.open_error is not None:
            raise self.open_error
        self.opened_path = path

    def xfer2(self, values: list[int]) -> list[int]:
        if self.transfer_error is not None:
            raise self.transfer_error
        self.transfers.append(list(values))
        if self._responses:
            return self._responses.pop(0)
        return [0] * len(values)

    def close(self) -> None:
        self.close_calls += 1


def _settings(
    *,
    clock_polarity: Literal[0, 1] = 0,
    clock_phase: Literal[0, 1] = 1,
    clock_frequency_hz: int = 1_000_000,
    bit_order: SpiBitOrder = SpiBitOrder.MSB_FIRST,
    bits_per_word: int = 8,
    chip_select_active_low: bool = True,
) -> SpiSettings:
    return SpiSettings(
        clock_polarity=clock_polarity,
        clock_phase=clock_phase,
        clock_frequency_hz=clock_frequency_hz,
        bit_order=bit_order,
        bits_per_word=bits_per_word,
        chip_select_active_low=chip_select_active_low,
    )


def test_linux_spidev_configures_backend_from_spi_settings() -> None:
    backend = FakeSpidevBackend()
    settings = _settings(
        clock_polarity=1,
        bit_order=SpiBitOrder.LSB_FIRST,
        bits_per_word=9,
        chip_select_active_low=False,
    )

    device = LinuxSpidevDevice("/dev/spidev0.1", settings, _backend=backend)

    assert device.device_path == "/dev/spidev0.1"
    assert device.settings is settings
    assert backend.opened_path == "/dev/spidev0.1"
    assert backend.mode == 0b11
    assert backend.max_speed_hz == 1_000_000
    assert backend.lsbfirst is True
    assert backend.bits_per_word == 9
    assert backend.cshigh is True
    assert backend.no_cs is False


def test_linux_spidev_accepts_pathlike_device_path() -> None:
    backend = FakeSpidevBackend()

    device = LinuxSpidevDevice(Path("/dev/spidev0.0"), _settings(), _backend=backend)

    assert device.device_path == "/dev/spidev0.0"


def test_linux_spidev_uses_one_xfer2_call_per_transaction() -> None:
    backend = FakeSpidevBackend(responses=[[0x00, 0x12, 0x34]])
    device = LinuxSpidevDevice("/dev/spidev0.0", _settings(), _backend=backend)

    response = device.transfer(b"\x01\x00\x00")

    assert response == b"\x00\x12\x34"
    assert backend.transfers == [[0x01, 0x00, 0x00]]


def test_linux_spidev_rejects_empty_device_path() -> None:
    with pytest.raises(ConfigurationError, match="device_path must not be empty"):
        LinuxSpidevDevice("", _settings(), _backend=FakeSpidevBackend())


def test_linux_spidev_reports_missing_optional_dependency(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    def missing_spidev(name: str) -> NoReturn:
        assert name == "spidev"
        raise ModuleNotFoundError("No module named 'spidev'", name="spidev")

    monkeypatch.setattr(importlib, "import_module", missing_spidev)

    with pytest.raises(ConfigurationError, match=r"rtd-acquire\[raspberry-pi\]"):
        LinuxSpidevDevice("/dev/spidev0.0", _settings())


def test_linux_spidev_wraps_open_or_configuration_failure() -> None:
    backend = FakeSpidevBackend()
    backend.open_error = OSError("permission denied")

    with pytest.raises(AcquisitionError, match="could not configure"):
        LinuxSpidevDevice("/dev/spidev0.0", _settings(), _backend=backend)

    assert backend.close_calls == 1


def test_linux_spidev_wraps_transfer_failure() -> None:
    backend = FakeSpidevBackend()
    backend.transfer_error = OSError("I/O failure")
    device = LinuxSpidevDevice("/dev/spidev0.0", _settings(), _backend=backend)

    with pytest.raises(AcquisitionError, match="SPI transfer failed"):
        device.transfer(b"\x00")


def test_linux_spidev_rejects_wrong_response_length() -> None:
    backend = FakeSpidevBackend(responses=[[0x00]])
    device = LinuxSpidevDevice("/dev/spidev0.0", _settings(), _backend=backend)

    with pytest.raises(AcquisitionError, match="unexpected number of bytes"):
        device.transfer(b"\x00\x00")


def test_linux_spidev_rejects_invalid_response_bytes() -> None:
    backend = FakeSpidevBackend(responses=[[256]])
    device = LinuxSpidevDevice("/dev/spidev0.0", _settings(), _backend=backend)

    with pytest.raises(AcquisitionError, match="invalid byte values"):
        device.transfer(b"\x00")


def test_linux_spidev_close_is_idempotent_and_blocks_future_transfer() -> None:
    backend = FakeSpidevBackend()
    device = LinuxSpidevDevice("/dev/spidev0.0", _settings(), _backend=backend)

    device.close()
    device.close()

    assert backend.close_calls == 1
    with pytest.raises(AcquisitionError, match="is closed"):
        device.transfer(b"\x00")


def test_linux_spidev_context_manager_closes_backend() -> None:
    backend = FakeSpidevBackend()

    with LinuxSpidevDevice("/dev/spidev0.0", _settings(), _backend=backend) as device:
        assert device.transfer(b"\x00") == b"\x00"

    assert backend.close_calls == 1
