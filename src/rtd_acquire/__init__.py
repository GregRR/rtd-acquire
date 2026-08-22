"""Hardware-agnostic RTD resistance acquisition."""

from .core import (
    AcquisitionDevice,
    AcquisitionError,
    ConfigurationError,
    Diagnostic,
    DiagnosticCode,
    DiagnosticSeverity,
    Measurement,
    MeasurementStatus,
    NativeEvidence,
    RtdAcquireError,
    diagnostic_message,
)

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
