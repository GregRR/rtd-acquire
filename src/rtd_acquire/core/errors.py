"""Exceptions for acquisition operations and caller configuration."""

from __future__ import annotations


class RtdAcquireError(Exception):
    """Base exception for rtd-acquire operation and configuration errors."""


class ConfigurationError(RtdAcquireError):
    """Caller-supplied acquisition configuration is invalid or unsupported."""


class AcquisitionError(RtdAcquireError):
    """An acquisition operation could not complete and return a Measurement."""
