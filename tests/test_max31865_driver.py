from __future__ import annotations

from collections.abc import Callable

import pytest

from rtd_acquire import AcquisitionError, ConfigurationError, MeasurementStatus
from rtd_acquire.max31865 import MAX31865, MAX31865Config, MAX31865Timing
from rtd_acquire.transports import SpiBitOrder, SpiSettings


class ScriptedSpi:
    def __init__(
        self,
        responses: list[bytes] | None = None,
        *,
        settings: SpiSettings | None = None,
    ) -> None:
        self._responses = list(responses or [])
        self.transactions: list[bytes] = []
        self._settings = settings or SpiSettings(
            clock_polarity=0,
            clock_phase=1,
            clock_frequency_hz=1_000_000,
        )

    @property
    def settings(self) -> SpiSettings:
        return self._settings

    def transfer(self, tx: bytes, /) -> bytes:
        self.transactions.append(tx)
        if self._responses:
            return self._responses.pop(0)
        return bytes(len(tx))


def _config(
    *,
    wire_count: int = 3,
    filter_frequency_hz: int = 60,
    low_fault_threshold_ohms: float | None = None,
    high_fault_threshold_ohms: float | None = None,
    reference_resistance_ohms: float = 430.0,
) -> MAX31865Config:
    return MAX31865Config(
        reference_resistance_ohms=reference_resistance_ohms,
        wire_count=wire_count,  # type: ignore[arg-type]
        filter_frequency_hz=filter_frequency_hz,  # type: ignore[arg-type]
        low_fault_threshold_ohms=low_fault_threshold_ohms,
        high_fault_threshold_ohms=high_fault_threshold_ohms,
    )


def _recording_sleep(delays: list[float]) -> Callable[[float], None]:
    def sleep(seconds: float) -> None:
        delays.append(seconds)

    return sleep


def test_one_shot_read_performs_documented_register_sequence() -> None:
    spi = ScriptedSpi(
        responses=[
            bytes(5),
            bytes(2),
            bytes(2),
            bytes(2),
            b"\x00\x40\x00",
            bytes(2),
        ]
    )
    delays: list[float] = []
    device = MAX31865(spi, _config(), sleep=_recording_sleep(delays))

    measurement = device.read()

    assert measurement.status is MeasurementStatus.OK
    assert measurement.resistance_ohms == pytest.approx(107.5)
    assert spi.transactions == [
        b"\x83\xff\xff\x00\x00",
        b"\x80\x92",
        b"\x80\x94",
        b"\x80\xb0",
        b"\x01\x00\x00",
        b"\x80\x10",
    ]
    assert delays == pytest.approx([0.0115, 0.0006, 0.006, 0.055])


def test_fault_flag_causes_fault_status_register_read() -> None:
    spi = ScriptedSpi(
        responses=[
            bytes(5),
            bytes(2),
            bytes(2),
            bytes(2),
            b"\x00\x40\x01",
            b"\x00\x80",
            bytes(2),
        ]
    )
    device = MAX31865(spi, _config(), sleep=lambda _: None)

    measurement = device.read()

    assert measurement.status is MeasurementStatus.WARNING
    assert measurement.resistance_ohms == pytest.approx(107.5)
    assert spi.transactions[-2] == b"\x07\x00"


def test_50_hz_configuration_uses_filter_bit_and_longer_conversion_wait() -> None:
    spi = ScriptedSpi(
        responses=[bytes(5), bytes(2), bytes(2), bytes(2), b"\x00\x40\x00", bytes(2)]
    )
    delays: list[float] = []
    device = MAX31865(
        spi,
        _config(filter_frequency_hz=50),
        sleep=_recording_sleep(delays),
    )

    device.read()

    assert spi.transactions[1] == b"\x80\x93"
    assert spi.transactions[2] == b"\x80\x95"
    assert spi.transactions[3] == b"\x80\xb1"
    assert spi.transactions[-1] == b"\x80\x11"
    assert delays[-1] == pytest.approx(0.066)


@pytest.mark.parametrize(
    ("wire_count", "expected_base"),
    [(2, 0x00), (3, 0x10), (4, 0x00)],
)
def test_wire_count_controls_only_documented_three_wire_bit(
    wire_count: int,
    expected_base: int,
) -> None:
    spi = ScriptedSpi(
        responses=[bytes(5), bytes(2), bytes(2), bytes(2), b"\x00\x40\x00", bytes(2)]
    )
    device = MAX31865(spi, _config(wire_count=wire_count), sleep=lambda _: None)

    device.read()

    assert spi.transactions[1] == bytes((0x80, expected_base | 0x82))
    assert spi.transactions[-1] == bytes((0x80, expected_base))


def test_configured_thresholds_are_encoded_in_device_ratio_format() -> None:
    spi = ScriptedSpi(
        responses=[bytes(5), bytes(2), bytes(2), bytes(2), b"\x00\x40\x00", bytes(2)]
    )
    device = MAX31865(
        spi,
        _config(
            reference_resistance_ohms=400.0,
            low_fault_threshold_ohms=50.0,
            high_fault_threshold_ohms=100.0,
        ),
        sleep=lambda _: None,
    )

    device.read()

    assert spi.transactions[0] == b"\x83\x40\x00\x20\x00"


def test_threshold_quantization_does_not_trigger_inside_requested_window() -> None:
    spi = ScriptedSpi(
        responses=[bytes(5), bytes(2), bytes(2), bytes(2), b"\x00\x40\x00", bytes(2)]
    )
    device = MAX31865(
        spi,
        _config(
            reference_resistance_ohms=400.0,
            low_fault_threshold_ohms=50.001,
            high_fault_threshold_ohms=100.001,
        ),
        sleep=lambda _: None,
    )

    device.read()

    threshold_write = spi.transactions[0]
    high = int.from_bytes(threshold_write[1:3], "big") >> 1
    low = int.from_bytes(threshold_write[3:5], "big") >> 1
    assert high == 8193
    assert low == 4096


def test_timing_can_model_larger_external_input_filter() -> None:
    timing = MAX31865Timing(input_filter_time_constant_seconds=0.002)

    assert timing.bias_settle_seconds == pytest.approx(0.022)
    assert timing.post_fault_settle_seconds == pytest.approx(0.011)


@pytest.mark.parametrize("time_constant", [-0.001, float("inf"), float("nan")])
def test_invalid_input_filter_time_constant_is_rejected(time_constant: float) -> None:
    with pytest.raises(ConfigurationError, match="finite and non-negative"):
        MAX31865Timing(input_filter_time_constant_seconds=time_constant)


@pytest.mark.parametrize(
    ("settings", "message"),
    [
        (
            SpiSettings(0, 0, 1_000_000),
            "clock_phase=1",
        ),
        (
            SpiSettings(0, 1, 5_000_001),
            "<= 5000000",
        ),
        (
            SpiSettings(0, 1, 1_000_000, bit_order=SpiBitOrder.LSB_FIRST),
            "MSB-first",
        ),
        (
            SpiSettings(0, 1, 1_000_000, bits_per_word=16),
            "8-bit",
        ),
        (
            SpiSettings(0, 1, 1_000_000, chip_select_active_low=False),
            "active-low",
        ),
    ],
)
def test_incompatible_spi_settings_are_rejected(
    settings: SpiSettings,
    message: str,
) -> None:
    with pytest.raises(ConfigurationError, match=message):
        MAX31865(ScriptedSpi(settings=settings), _config())


def test_spi_response_length_mismatch_is_acquisition_error() -> None:
    spi = ScriptedSpi(responses=[b"\x00"])
    device = MAX31865(spi, _config(), sleep=lambda _: None)

    with pytest.raises(AcquisitionError, match="unexpected number of bytes"):
        device.read()


def test_non_bytes_spi_response_is_acquisition_error() -> None:
    class InvalidSpi(ScriptedSpi):
        def transfer(self, tx: bytes, /) -> bytes:
            self.transactions.append(tx)
            return bytearray(len(tx))  # type: ignore[return-value]

    device = MAX31865(InvalidSpi(), _config(), sleep=lambda _: None)

    with pytest.raises(AcquisitionError, match="must return bytes"):
        device.read()
