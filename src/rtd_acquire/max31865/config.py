"""Configuration contract for MAX31865 acquisition.

Implementation basis: Analog Devices (2015), MAX31865 data sheet; see
``docs/REFERENCES.md``.
"""

from __future__ import annotations

import math
from dataclasses import dataclass
from typing import Literal

from ..core.errors import ConfigurationError
from ._thresholds import encode_high_threshold_register

_MIN_REFERENCE_RESISTANCE_OHMS = 350.0
_MAX_REFERENCE_RESISTANCE_OHMS = 10_000.0


@dataclass(frozen=True, slots=True)
class MAX31865Config:
    """Electrical configuration required to acquire RTD resistance."""

    reference_resistance_ohms: float
    wire_count: Literal[2, 3, 4]
    filter_frequency_hz: Literal[50, 60]
    low_fault_threshold_ohms: float | None = None
    high_fault_threshold_ohms: float | None = None

    def __post_init__(self) -> None:
        if not math.isfinite(self.reference_resistance_ohms) or not (
            _MIN_REFERENCE_RESISTANCE_OHMS
            <= self.reference_resistance_ohms
            <= _MAX_REFERENCE_RESISTANCE_OHMS
        ):
            raise ConfigurationError(
                "reference_resistance_ohms must be finite and between "
                "350 and 10000 ohms"
            )

        if self.wire_count not in (2, 3, 4):
            raise ConfigurationError("wire_count must be 2, 3, or 4")

        if self.filter_frequency_hz not in (50, 60):
            raise ConfigurationError("filter_frequency_hz must be 50 or 60")

        self._validate_threshold(
            "low_fault_threshold_ohms",
            self.low_fault_threshold_ohms,
            allow_zero=True,
        )
        self._validate_threshold(
            "high_fault_threshold_ohms",
            self.high_fault_threshold_ohms,
            allow_zero=False,
        )

        encode_high_threshold_register(
            self.high_fault_threshold_ohms,
            self.reference_resistance_ohms,
        )

        if (
            self.low_fault_threshold_ohms is not None
            and self.high_fault_threshold_ohms is not None
            and self.low_fault_threshold_ohms >= self.high_fault_threshold_ohms
        ):
            raise ConfigurationError(
                "low_fault_threshold_ohms must be less than high_fault_threshold_ohms"
            )

    def _validate_threshold(
        self,
        name: str,
        value: float | None,
        *,
        allow_zero: bool,
    ) -> None:
        if value is None:
            return

        minimum_ok = value >= 0.0 if allow_zero else value > 0.0
        if not math.isfinite(value) or not minimum_ok:
            qualifier = "non-negative" if allow_zero else "greater than zero"
            raise ConfigurationError(f"{name} must be finite and {qualifier}")

        if value >= self.reference_resistance_ohms:
            raise ConfigurationError(
                f"{name} must be less than reference_resistance_ohms"
            )
