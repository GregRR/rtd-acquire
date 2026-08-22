from __future__ import annotations

import math

import pytest

from rtd_acquire.core import (
    Diagnostic,
    DiagnosticCode,
    DiagnosticSeverity,
    Measurement,
    MeasurementStatus,
)


def test_ok_measurement_has_resistance_and_no_diagnostics() -> None:
    measurement = Measurement(resistance_ohms=109.73)

    assert measurement.status is MeasurementStatus.OK
    assert measurement.resistance_ohms == pytest.approx(109.73)
    assert measurement.diagnostics == ()


def test_zero_resistance_is_representable() -> None:
    measurement = Measurement(resistance_ohms=0.0)

    assert measurement.status is MeasurementStatus.OK
    assert measurement.resistance_ohms == 0.0


def test_warning_measurement_keeps_usable_resistance() -> None:
    diagnostic = Diagnostic(
        code=DiagnosticCode.RESISTANCE_HIGH_THRESHOLD,
        severity=DiagnosticSeverity.WARNING,
    )
    measurement = Measurement(
        resistance_ohms=140.0,
        diagnostics=(diagnostic,),
    )

    assert measurement.status is MeasurementStatus.WARNING
    assert measurement.resistance_ohms == pytest.approx(140.0)


def test_fault_measurement_has_no_resistance() -> None:
    diagnostic = Diagnostic(
        code=DiagnosticCode.SENSOR_INPUT_FAULT,
        severity=DiagnosticSeverity.FAULT,
    )
    measurement = Measurement(
        resistance_ohms=None,
        diagnostics=(diagnostic,),
    )

    assert measurement.status is MeasurementStatus.FAULT
    assert measurement.resistance_ohms is None


def test_fault_status_wins_when_warnings_are_also_present() -> None:
    warning = Diagnostic(
        code=DiagnosticCode.RESISTANCE_HIGH_THRESHOLD,
        severity=DiagnosticSeverity.WARNING,
    )
    fault = Diagnostic(
        code=DiagnosticCode.SENSOR_INPUT_FAULT,
        severity=DiagnosticSeverity.FAULT,
    )
    measurement = Measurement(
        resistance_ohms=None,
        diagnostics=(warning, fault),
    )

    assert measurement.status is MeasurementStatus.FAULT


def test_standard_uncertainty_is_allowed_for_usable_measurement() -> None:
    measurement = Measurement(
        resistance_ohms=109.73,
        standard_uncertainty_ohms=0.018,
    )

    assert measurement.standard_uncertainty_ohms == pytest.approx(0.018)


def test_zero_standard_uncertainty_is_representable() -> None:
    measurement = Measurement(
        resistance_ohms=109.73,
        standard_uncertainty_ohms=0.0,
    )

    assert measurement.standard_uncertainty_ohms == 0.0


@pytest.mark.parametrize("resistance", [-1.0, math.inf, -math.inf, math.nan])
def test_invalid_resistance_is_rejected(resistance: float) -> None:
    with pytest.raises(ValueError, match="finite and non-negative"):
        Measurement(resistance_ohms=resistance)


@pytest.mark.parametrize("uncertainty", [-0.1, math.inf, -math.inf, math.nan])
def test_invalid_uncertainty_is_rejected(uncertainty: float) -> None:
    with pytest.raises(ValueError, match="finite and non-negative"):
        Measurement(
            resistance_ohms=109.73,
            standard_uncertainty_ohms=uncertainty,
        )


def test_missing_resistance_requires_fault_diagnostic() -> None:
    with pytest.raises(ValueError, match="requires a fault diagnostic"):
        Measurement(resistance_ohms=None)


def test_warning_without_resistance_is_rejected() -> None:
    warning = Diagnostic(
        code=DiagnosticCode.RESISTANCE_HIGH_THRESHOLD,
        severity=DiagnosticSeverity.WARNING,
    )

    with pytest.raises(ValueError, match="requires a fault diagnostic"):
        Measurement(resistance_ohms=None, diagnostics=(warning,))


def test_fault_with_resistance_is_rejected() -> None:
    fault = Diagnostic(
        code=DiagnosticCode.SENSOR_INPUT_FAULT,
        severity=DiagnosticSeverity.FAULT,
    )

    with pytest.raises(ValueError, match="must not contain resistance"):
        Measurement(resistance_ohms=109.73, diagnostics=(fault,))


def test_fault_with_uncertainty_is_rejected() -> None:
    fault = Diagnostic(
        code=DiagnosticCode.SENSOR_INPUT_FAULT,
        severity=DiagnosticSeverity.FAULT,
    )

    with pytest.raises(ValueError, match="must not contain uncertainty"):
        Measurement(
            resistance_ohms=None,
            diagnostics=(fault,),
            standard_uncertainty_ohms=0.018,
        )
