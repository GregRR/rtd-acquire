"""MAX31865 threshold-register encoding helpers.

Implementation basis: Analog Devices (2015), MAX31865 data sheet; see
``docs/REFERENCES.md``.
"""

from __future__ import annotations

import math

from ..core.errors import ConfigurationError

_ADC_SCALE = 1 << 15
_MAX_ADC_CODE = _ADC_SCALE - 1


def encode_high_threshold_register(
    resistance_ohms: float | None,
    reference_resistance_ohms: float,
) -> int:
    """Encode a high threshold without rounding into the requested window.

    The MAX31865 threshold registers use the same 15-bit ratiometric domain as
    the RTD result, shifted left by one bit. High thresholds round upward so
    quantization cannot make the native threshold lower than the caller's
    request. A request above the highest representable 15-bit threshold is
    rejected rather than silently clamped downward.
    """

    if resistance_ohms is None:
        return 0xFFFF

    raw_code = math.ceil(resistance_ohms / reference_resistance_ohms * _ADC_SCALE)
    if raw_code > _MAX_ADC_CODE:
        raise ConfigurationError(
            "high_fault_threshold_ohms cannot be represented without "
            "rounding below the requested threshold"
        )
    return raw_code << 1


def encode_low_threshold_register(
    resistance_ohms: float | None,
    reference_resistance_ohms: float,
) -> int:
    """Encode a low threshold without rounding into the requested window."""

    if resistance_ohms is None:
        return 0x0000

    raw_code = math.floor(resistance_ohms / reference_resistance_ohms * _ADC_SCALE)
    return raw_code << 1
