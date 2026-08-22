"""Core resistance-measurement result contract."""

from __future__ import annotations

import math
from dataclasses import dataclass
from enum import StrEnum

from .diagnostics import Diagnostic, DiagnosticSeverity


class MeasurementStatus(StrEnum):
    """Trust status derived from a measurement's diagnostics."""

    OK = "ok"
    WARNING = "warning"
    FAULT = "fault"


@dataclass(frozen=True, slots=True)
class Measurement:
    """A trustworthy RTD resistance estimate and acquisition diagnostics."""

    resistance_ohms: float | None
    diagnostics: tuple[Diagnostic, ...] = ()
    standard_uncertainty_ohms: float | None = None

    def __post_init__(self) -> None:
        diagnostics = tuple(self.diagnostics)
        if not all(isinstance(item, Diagnostic) for item in diagnostics):
            raise TypeError("diagnostics must contain only Diagnostic items")
        object.__setattr__(self, "diagnostics", diagnostics)

        if self.resistance_ohms is not None and (
            not math.isfinite(self.resistance_ohms) or self.resistance_ohms <= 0.0
        ):
            raise ValueError("resistance_ohms must be finite and greater than zero")

        if self.standard_uncertainty_ohms is not None and (
            not math.isfinite(self.standard_uncertainty_ohms)
            or self.standard_uncertainty_ohms < 0.0
        ):
            raise ValueError(
                "standard_uncertainty_ohms must be finite and non-negative"
            )

        has_fault = any(
            diagnostic.severity is DiagnosticSeverity.FAULT
            for diagnostic in diagnostics
        )
        if has_fault:
            if self.resistance_ohms is not None:
                raise ValueError("fault measurements must not contain resistance")
            if self.standard_uncertainty_ohms is not None:
                raise ValueError("fault measurements must not contain uncertainty")
        elif self.resistance_ohms is None:
            raise ValueError(
                "a measurement without resistance requires a fault diagnostic"
            )

    @property
    def status(self) -> MeasurementStatus:
        """Derive trust status from diagnostic severities."""

        if any(
            diagnostic.severity is DiagnosticSeverity.FAULT
            for diagnostic in self.diagnostics
        ):
            return MeasurementStatus.FAULT
        if self.diagnostics:
            return MeasurementStatus.WARNING
        return MeasurementStatus.OK
