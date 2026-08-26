"""MAX31865 register decoding independent of any SPI implementation.

Implementation basis: Analog Devices (2015), MAX31865 data sheet; see
``docs/REFERENCES.md``.
"""

from __future__ import annotations

from dataclasses import dataclass

from ..core import (
    Diagnostic,
    DiagnosticCode,
    DiagnosticSeverity,
    Measurement,
    NativeEvidence,
)
from ..core.errors import AcquisitionError
from .config import MAX31865Config

_RTD_CODE_SCALE = 32768.0


@dataclass(frozen=True, slots=True)
class _FaultMapping:
    mask: int
    code: DiagnosticCode
    severity: DiagnosticSeverity
    identifier: str
    native_message: str


_FAULT_MAPPINGS = (
    _FaultMapping(
        0x80,
        DiagnosticCode.RESISTANCE_HIGH_THRESHOLD,
        DiagnosticSeverity.WARNING,
        "D7",
        "RTD High Threshold",
    ),
    _FaultMapping(
        0x40,
        DiagnosticCode.RESISTANCE_LOW_THRESHOLD,
        DiagnosticSeverity.WARNING,
        "D6",
        "RTD Low Threshold",
    ),
    _FaultMapping(
        0x20,
        DiagnosticCode.REFERENCE_INPUT_ABOVE_THRESHOLD,
        DiagnosticSeverity.FAULT,
        "D5",
        "REFIN- > 0.85 x VBIAS",
    ),
    _FaultMapping(
        0x10,
        DiagnosticCode.REFERENCE_INPUT_BELOW_THRESHOLD,
        DiagnosticSeverity.FAULT,
        "D4",
        "REFIN- < 0.85 x VBIAS (FORCE- open)",
    ),
    _FaultMapping(
        0x08,
        DiagnosticCode.RTD_INPUT_BELOW_THRESHOLD,
        DiagnosticSeverity.FAULT,
        "D3",
        "RTDIN- < 0.85 x VBIAS (FORCE- open)",
    ),
    _FaultMapping(
        0x04,
        DiagnosticCode.INPUT_VOLTAGE_FAULT,
        DiagnosticSeverity.FAULT,
        "D2",
        "Overvoltage/undervoltage fault",
    ),
)


def measurement_from_registers(
    config: MAX31865Config,
    *,
    rtd_register: int,
    fault_status_register: int,
) -> Measurement:
    """Decode one MAX31865 native register state into a Measurement."""

    _validate_register("rtd_register", rtd_register, maximum=0xFFFF)
    _validate_register("fault_status_register", fault_status_register, maximum=0xFF)

    diagnostics = tuple(
        Diagnostic(
            code=mapping.code,
            severity=mapping.severity,
            native_evidence=(
                NativeEvidence(
                    identifier=mapping.identifier,
                    message=mapping.native_message,
                ),
            ),
        )
        for mapping in _FAULT_MAPPINGS
        if fault_status_register & mapping.mask
    )

    if any(
        diagnostic.severity is DiagnosticSeverity.FAULT for diagnostic in diagnostics
    ):
        return Measurement(resistance_ohms=None, diagnostics=diagnostics)

    adc_code = rtd_register >> 1
    resistance_ohms = adc_code / _RTD_CODE_SCALE * config.reference_resistance_ohms
    return Measurement(
        resistance_ohms=resistance_ohms,
        diagnostics=diagnostics,
    )


def _validate_register(name: str, value: int, *, maximum: int) -> None:
    if isinstance(value, bool) or not isinstance(value, int):
        raise AcquisitionError(f"{name} must be an integer")
    if not 0 <= value <= maximum:
        raise AcquisitionError(f"{name} must be between 0 and {maximum}")
