from __future__ import annotations

import pytest

from rtd_acquire import AcquisitionError, DiagnosticCode, DiagnosticSeverity
from rtd_acquire.max31865 import MAX31865Config
from rtd_acquire.max31865._decode import measurement_from_registers


def _config() -> MAX31865Config:
    return MAX31865Config(
        reference_resistance_ohms=430.0,
        wire_count=3,
        filter_frequency_hz=60,
    )


@pytest.mark.parametrize(
    ("fault_status", "code", "severity"),
    [
        (0x80, DiagnosticCode.RESISTANCE_HIGH_THRESHOLD, DiagnosticSeverity.WARNING),
        (0x40, DiagnosticCode.RESISTANCE_LOW_THRESHOLD, DiagnosticSeverity.WARNING),
        (
            0x20,
            DiagnosticCode.REFERENCE_INPUT_ABOVE_THRESHOLD,
            DiagnosticSeverity.FAULT,
        ),
        (
            0x10,
            DiagnosticCode.REFERENCE_INPUT_BELOW_THRESHOLD,
            DiagnosticSeverity.FAULT,
        ),
        (
            0x08,
            DiagnosticCode.RTD_INPUT_BELOW_THRESHOLD,
            DiagnosticSeverity.FAULT,
        ),
        (0x04, DiagnosticCode.INPUT_VOLTAGE_FAULT, DiagnosticSeverity.FAULT),
    ],
)
def test_each_defined_fault_bit_maps_without_inference(
    fault_status: int,
    code: DiagnosticCode,
    severity: DiagnosticSeverity,
) -> None:
    measurement = measurement_from_registers(
        _config(),
        rtd_register=0x4001,
        fault_status_register=fault_status,
    )

    assert len(measurement.diagnostics) == 1
    assert measurement.diagnostics[0].code is code
    assert measurement.diagnostics[0].severity is severity


def test_fault_bits_are_reported_in_register_bit_order() -> None:
    measurement = measurement_from_registers(
        _config(),
        rtd_register=0x4001,
        fault_status_register=0xFC,
    )

    assert [diagnostic.code for diagnostic in measurement.diagnostics] == [
        DiagnosticCode.RESISTANCE_HIGH_THRESHOLD,
        DiagnosticCode.RESISTANCE_LOW_THRESHOLD,
        DiagnosticCode.REFERENCE_INPUT_ABOVE_THRESHOLD,
        DiagnosticCode.REFERENCE_INPUT_BELOW_THRESHOLD,
        DiagnosticCode.RTD_INPUT_BELOW_THRESHOLD,
        DiagnosticCode.INPUT_VOLTAGE_FAULT,
    ]
    assert measurement.resistance_ohms is None


def test_reserved_fault_status_bits_do_not_create_diagnostics() -> None:
    measurement = measurement_from_registers(
        _config(),
        rtd_register=0x4000,
        fault_status_register=0x03,
    )

    assert measurement.diagnostics == ()
    assert measurement.resistance_ohms == pytest.approx(107.5)


def test_reserved_fault_status_bits_are_ignored_with_known_warning() -> None:
    measurement = measurement_from_registers(
        _config(),
        rtd_register=0x4001,
        fault_status_register=0x83,
    )

    assert [diagnostic.code for diagnostic in measurement.diagnostics] == [
        DiagnosticCode.RESISTANCE_HIGH_THRESHOLD
    ]
    assert measurement.resistance_ohms == pytest.approx(107.5)


def test_fault_flag_bit_is_not_part_of_resistance_code() -> None:
    without_flag = measurement_from_registers(
        _config(), rtd_register=0x4000, fault_status_register=0
    )
    with_flag = measurement_from_registers(
        _config(), rtd_register=0x4001, fault_status_register=0
    )

    assert without_flag.resistance_ohms == pytest.approx(107.5)
    assert with_flag.resistance_ohms == pytest.approx(107.5)


@pytest.mark.parametrize(
    ("name", "rtd_register", "fault_status"),
    [
        ("rtd_register", -1, 0),
        ("rtd_register", 0x1_0000, 0),
        ("fault_status_register", 0, -1),
        ("fault_status_register", 0, 0x100),
    ],
)
def test_out_of_range_native_registers_are_rejected(
    name: str,
    rtd_register: int,
    fault_status: int,
) -> None:
    with pytest.raises(AcquisitionError, match=name):
        measurement_from_registers(
            _config(),
            rtd_register=rtd_register,
            fault_status_register=fault_status,
        )


@pytest.mark.parametrize(
    ("name", "rtd_register", "fault_status"),
    [
        ("rtd_register", True, 0),
        ("fault_status_register", 0, False),
    ],
)
def test_boolean_register_values_are_rejected(
    name: str,
    rtd_register: int,
    fault_status: int,
) -> None:
    with pytest.raises(AcquisitionError, match=name):
        measurement_from_registers(
            _config(),
            rtd_register=rtd_register,
            fault_status_register=fault_status,
        )
