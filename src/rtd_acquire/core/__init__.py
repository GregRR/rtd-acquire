"""Core public contracts for rtd-acquire."""

from .device import AcquisitionDevice
from .diagnostics import (
    Diagnostic,
    DiagnosticCode,
    DiagnosticSeverity,
    NativeEvidence,
    diagnostic_message,
)
from .errors import AcquisitionError, ConfigurationError, RtdAcquireError
from .measurement import Measurement, MeasurementStatus

__all__ = [
    "AcquisitionDevice",
    "AcquisitionError",
    "ConfigurationError",
    "Diagnostic",
    "DiagnosticCode",
    "DiagnosticSeverity",
    "Measurement",
    "MeasurementStatus",
    "NativeEvidence",
    "RtdAcquireError",
    "diagnostic_message",
]
