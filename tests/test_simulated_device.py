from __future__ import annotations

import pytest

from rtd_acquire import (
    AcquisitionDevice,
    AcquisitionError,
    ConfigurationError,
    Diagnostic,
    DiagnosticCode,
    DiagnosticSeverity,
    Measurement,
    MeasurementStatus,
)
from rtd_acquire.simulation import (
    SimulatedAcquisitionDevice,
    SimulatedAcquisitionFailure,
)


def _read_once(device: AcquisitionDevice) -> Measurement:
    return device.read()


def test_simulated_device_satisfies_acquisition_device_contract() -> None:
    expected = Measurement(resistance_ohms=109.73)
    device = SimulatedAcquisitionDevice([expected])

    assert _read_once(device) is expected


def test_simulated_device_replays_measurements_in_order() -> None:
    first = Measurement(resistance_ohms=100.0)
    second = Measurement(resistance_ohms=101.0, standard_uncertainty_ohms=0.02)
    device = SimulatedAcquisitionDevice([first, second])

    assert device.read() is first
    assert device.read() is second
    assert device.read_count == 2


def test_simulated_device_preserves_warning_measurement() -> None:
    warning = Diagnostic(
        code=DiagnosticCode.RESISTANCE_HIGH_THRESHOLD,
        severity=DiagnosticSeverity.WARNING,
    )
    expected = Measurement(resistance_ohms=150.0, diagnostics=(warning,))
    device = SimulatedAcquisitionDevice([expected])

    actual = device.read()

    assert actual is expected
    assert actual.status is MeasurementStatus.WARNING


def test_simulated_device_preserves_fault_measurement() -> None:
    fault = Diagnostic(
        code=DiagnosticCode.INPUT_VOLTAGE_FAULT,
        severity=DiagnosticSeverity.FAULT,
    )
    expected = Measurement(resistance_ohms=None, diagnostics=(fault,))
    device = SimulatedAcquisitionDevice([expected])

    actual = device.read()

    assert actual is expected
    assert actual.status is MeasurementStatus.FAULT


def test_simulated_operation_failure_raises_and_advances() -> None:
    expected = Measurement(resistance_ohms=107.5)
    device = SimulatedAcquisitionDevice(
        [SimulatedAcquisitionFailure("simulated SPI timeout"), expected]
    )

    with pytest.raises(AcquisitionError, match="simulated SPI timeout"):
        device.read()

    assert device.read_count == 1
    assert device.read() is expected
    assert device.read_count == 2


def test_simulated_script_exhaustion_is_an_acquisition_error() -> None:
    device = SimulatedAcquisitionDevice([Measurement(resistance_ohms=100.0)])
    device.read()

    with pytest.raises(AcquisitionError, match="script exhausted"):
        device.read()

    assert device.read_count == 1


def test_repeating_simulation_loops_entire_script() -> None:
    first = Measurement(resistance_ohms=100.0)
    second = Measurement(resistance_ohms=101.0)
    device = SimulatedAcquisitionDevice([first, second], repeat=True)

    assert [device.read(), device.read(), device.read()] == [first, second, first]
    assert device.read_count == 3


def test_reset_restarts_script_and_counter() -> None:
    first = Measurement(resistance_ohms=100.0)
    second = Measurement(resistance_ohms=101.0)
    device = SimulatedAcquisitionDevice([first, second])

    assert device.read() is first
    device.reset()

    assert device.read_count == 0
    assert device.read() is first
    assert device.read_count == 1


def test_empty_simulation_script_is_rejected() -> None:
    with pytest.raises(ConfigurationError, match="must not be empty"):
        SimulatedAcquisitionDevice([])


def test_invalid_simulation_step_is_rejected() -> None:
    with pytest.raises(TypeError, match="steps must be Measurement"):
        SimulatedAcquisitionDevice([object()])  # type: ignore[list-item]


def test_repeat_must_be_boolean() -> None:
    with pytest.raises(ConfigurationError, match="repeat must be a bool"):
        SimulatedAcquisitionDevice(
            [Measurement(resistance_ohms=100.0)],
            repeat=1,  # type: ignore[arg-type]
        )


@pytest.mark.parametrize("message", ["", "   ", 123])
def test_simulated_failure_message_must_be_non_empty_string(message: object) -> None:
    with pytest.raises(ConfigurationError, match="non-empty string"):
        SimulatedAcquisitionFailure(message=message)  # type: ignore[arg-type]
