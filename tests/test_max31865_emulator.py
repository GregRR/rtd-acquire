from __future__ import annotations

import pytest

from rtd_acquire import (
    AcquisitionError,
    ConfigurationError,
    DiagnosticCode,
    MeasurementStatus,
)
from rtd_acquire.max31865 import MAX31865, MAX31865Config, MAX31865SpiEmulator


def _config(
    *,
    reference_resistance_ohms: float = 430.0,
    low_fault_threshold_ohms: float | None = None,
    high_fault_threshold_ohms: float | None = None,
) -> MAX31865Config:
    return MAX31865Config(
        reference_resistance_ohms=reference_resistance_ohms,
        wire_count=3,
        filter_frequency_hz=60,
        low_fault_threshold_ohms=low_fault_threshold_ohms,
        high_fault_threshold_ohms=high_fault_threshold_ohms,
    )


def test_emulator_runs_real_driver_for_ok_measurement() -> None:
    emulator = MAX31865SpiEmulator(rtd_code=8192)
    device = MAX31865(emulator, _config(), sleep=lambda _: None)

    measurement = device.read()

    assert measurement.status is MeasurementStatus.OK
    assert measurement.resistance_ohms == pytest.approx(107.5)
    assert emulator.transactions == (
        b"\x83\xff\xff\x00\x00",
        b"\x80\x92",
        b"\x80\x94",
        b"\x80\xb0",
        b"\x01\x00\x00",
        b"\x80\x10",
    )
    assert emulator.last_config_write == 0x10


def test_emulator_relatches_warning_fault_after_driver_clear() -> None:
    emulator = MAX31865SpiEmulator(
        rtd_code=8192,
        fault_status_register=0x80,
    )
    device = MAX31865(emulator, _config(), sleep=lambda _: None)

    measurement = device.read()

    assert measurement.status is MeasurementStatus.WARNING
    assert measurement.resistance_ohms == pytest.approx(107.5)
    assert [diagnostic.code for diagnostic in measurement.diagnostics] == [
        DiagnosticCode.RESISTANCE_HIGH_THRESHOLD
    ]
    assert emulator.transactions[-2] == b"\x07\x00"


def test_emulator_drives_real_driver_fault_path() -> None:
    emulator = MAX31865SpiEmulator(
        rtd_code=8192,
        fault_status_register=0x04,
    )
    device = MAX31865(emulator, _config(), sleep=lambda _: None)

    measurement = device.read()

    assert measurement.status is MeasurementStatus.FAULT
    assert measurement.resistance_ohms is None
    assert [diagnostic.code for diagnostic in measurement.diagnostics] == [
        DiagnosticCode.INPUT_VOLTAGE_FAULT
    ]


def test_emulator_captures_encoded_threshold_registers() -> None:
    emulator = MAX31865SpiEmulator(rtd_code=8192)
    device = MAX31865(
        emulator,
        _config(
            reference_resistance_ohms=400.0,
            low_fault_threshold_ohms=50.0,
            high_fault_threshold_ohms=100.0,
        ),
        sleep=lambda _: None,
    )

    device.read()

    assert emulator.high_threshold_register == 0x4000
    assert emulator.low_threshold_register == 0x2000


def test_emulator_is_deterministic_across_repeated_reads() -> None:
    emulator = MAX31865SpiEmulator(rtd_code=8192, fault_status_register=0x80)
    device = MAX31865(emulator, _config(), sleep=lambda _: None)

    first = device.read()
    second = device.read()

    assert first == second
    assert len(emulator.transactions) == 14


@pytest.mark.parametrize("value", [-1, 32768, True, 1.5])
def test_emulator_rejects_invalid_rtd_code(value: object) -> None:
    with pytest.raises(ConfigurationError, match="rtd_code"):
        MAX31865SpiEmulator(rtd_code=value)  # type: ignore[arg-type]


@pytest.mark.parametrize("value", [-1, 256, 0x01, 0x02, 0x03, True, 1.5])
def test_emulator_rejects_invalid_fault_status(value: object) -> None:
    with pytest.raises(ConfigurationError, match="fault_status_register"):
        MAX31865SpiEmulator(
            rtd_code=0,
            fault_status_register=value,  # type: ignore[arg-type]
        )


def test_emulator_requires_one_shot_before_rtd_data_read() -> None:
    emulator = MAX31865SpiEmulator(rtd_code=8192)

    with pytest.raises(AcquisitionError, match="requires a one-shot conversion"):
        emulator.transfer(b"\x01\x00\x00")


def test_emulator_rejects_empty_transaction() -> None:
    emulator = MAX31865SpiEmulator(rtd_code=0)

    with pytest.raises(AcquisitionError, match="must not be empty"):
        emulator.transfer(b"")


def test_emulator_rejects_unsupported_register_transaction() -> None:
    emulator = MAX31865SpiEmulator(rtd_code=0)

    with pytest.raises(AcquisitionError, match="unsupported register read"):
        emulator.transfer(b"\x02\x00")
