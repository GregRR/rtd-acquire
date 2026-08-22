from __future__ import annotations

from dataclasses import FrozenInstanceError

import pytest

from rtd_acquire import ConfigurationError
from rtd_acquire.transports import SpiBitOrder, SpiDevice, SpiSettings


def test_spi_settings_accept_common_max31865_connection() -> None:
    settings = SpiSettings(
        clock_polarity=0,
        clock_phase=1,
        clock_frequency_hz=1_000_000,
    )

    assert settings.clock_polarity == 0
    assert settings.clock_phase == 1
    assert settings.clock_frequency_hz == 1_000_000
    assert settings.bit_order is SpiBitOrder.MSB_FIRST
    assert settings.bits_per_word == 8
    assert settings.chip_select_active_low is True


@pytest.mark.parametrize("polarity", [-1, 2])
def test_invalid_clock_polarity_is_rejected(polarity: int) -> None:
    with pytest.raises(ConfigurationError, match="clock_polarity must be 0 or 1"):
        SpiSettings(
            clock_polarity=polarity,  # type: ignore[arg-type]
            clock_phase=1,
            clock_frequency_hz=1_000_000,
        )


@pytest.mark.parametrize("phase", [-1, 2])
def test_invalid_clock_phase_is_rejected(phase: int) -> None:
    with pytest.raises(ConfigurationError, match="clock_phase must be 0 or 1"):
        SpiSettings(
            clock_polarity=0,
            clock_phase=phase,  # type: ignore[arg-type]
            clock_frequency_hz=1_000_000,
        )


@pytest.mark.parametrize("frequency", [0, -1])
def test_invalid_clock_frequency_is_rejected(frequency: int) -> None:
    with pytest.raises(ConfigurationError, match="must be greater than zero"):
        SpiSettings(
            clock_polarity=0,
            clock_phase=1,
            clock_frequency_hz=frequency,
        )


@pytest.mark.parametrize("bits_per_word", [0, -1])
def test_invalid_bits_per_word_is_rejected(bits_per_word: int) -> None:
    with pytest.raises(ConfigurationError, match="must be greater than zero"):
        SpiSettings(
            clock_polarity=0,
            clock_phase=1,
            clock_frequency_hz=1_000_000,
            bits_per_word=bits_per_word,
        )


def test_spi_settings_are_immutable() -> None:
    settings = SpiSettings(
        clock_polarity=0,
        clock_phase=1,
        clock_frequency_hz=1_000_000,
    )

    with pytest.raises(FrozenInstanceError):
        settings.clock_frequency_hz = 2_000_000  # type: ignore[misc]


def test_structural_spi_device_can_be_consumed_through_protocol() -> None:
    class FakeSpi:
        @property
        def settings(self) -> SpiSettings:
            return SpiSettings(
                clock_polarity=1,
                clock_phase=1,
                clock_frequency_hz=2_000_000,
            )

        def transfer(self, tx: bytes, /) -> bytes:
            return bytes(len(tx))

    def use_spi(device: SpiDevice) -> bytes:
        return device.transfer(b"\x00\x00")

    assert use_spi(FakeSpi()) == b"\x00\x00"
