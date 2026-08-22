from __future__ import annotations

import math

import pytest

from rtd_acquire import ConfigurationError
from rtd_acquire.max31865 import MAX31865Config


def test_config_accepts_common_pt100_electrical_setup() -> None:
    config = MAX31865Config(
        reference_resistance_ohms=430.0,
        wire_count=3,
        filter_frequency_hz=60,
    )

    assert config.reference_resistance_ohms == pytest.approx(430.0)
    assert config.wire_count == 3
    assert config.filter_frequency_hz == 60
    assert config.low_fault_threshold_ohms is None
    assert config.high_fault_threshold_ohms is None


def test_config_does_not_require_rtd_model_identity() -> None:
    config = MAX31865Config(
        reference_resistance_ohms=4300.0,
        wire_count=4,
        filter_frequency_hz=50,
    )

    assert config.reference_resistance_ohms == pytest.approx(4300.0)


@pytest.mark.parametrize("wire_count", [0, 1, 5, 6])
def test_invalid_wire_count_is_rejected(wire_count: int) -> None:
    with pytest.raises(ConfigurationError, match="wire_count must be 2, 3, or 4"):
        MAX31865Config(
            reference_resistance_ohms=430.0,
            wire_count=wire_count,  # type: ignore[arg-type]
            filter_frequency_hz=60,
        )


@pytest.mark.parametrize("frequency", [0, 49, 55, 61])
def test_invalid_filter_frequency_is_rejected(frequency: int) -> None:
    with pytest.raises(
        ConfigurationError, match="filter_frequency_hz must be 50 or 60"
    ):
        MAX31865Config(
            reference_resistance_ohms=430.0,
            wire_count=3,
            filter_frequency_hz=frequency,  # type: ignore[arg-type]
        )


@pytest.mark.parametrize(
    "reference_resistance",
    [0.0, 349.999, 10_000.001, math.inf, -math.inf, math.nan],
)
def test_invalid_reference_resistance_is_rejected(
    reference_resistance: float,
) -> None:
    with pytest.raises(ConfigurationError, match="between 350 and 10000 ohms"):
        MAX31865Config(
            reference_resistance_ohms=reference_resistance,
            wire_count=3,
            filter_frequency_hz=60,
        )


@pytest.mark.parametrize("reference_resistance", [350.0, 10_000.0])
def test_documented_reference_resistance_bounds_are_accepted(
    reference_resistance: float,
) -> None:
    config = MAX31865Config(
        reference_resistance_ohms=reference_resistance,
        wire_count=3,
        filter_frequency_hz=60,
    )

    assert config.reference_resistance_ohms == pytest.approx(reference_resistance)


def test_fault_thresholds_are_expressed_in_ohms() -> None:
    config = MAX31865Config(
        reference_resistance_ohms=430.0,
        wire_count=3,
        filter_frequency_hz=60,
        low_fault_threshold_ohms=80.0,
        high_fault_threshold_ohms=160.0,
    )

    assert config.low_fault_threshold_ohms == pytest.approx(80.0)
    assert config.high_fault_threshold_ohms == pytest.approx(160.0)


@pytest.mark.parametrize("threshold", [-1.0, math.inf, -math.inf, math.nan])
def test_invalid_low_fault_threshold_is_rejected(threshold: float) -> None:
    with pytest.raises(ConfigurationError, match="finite and non-negative"):
        MAX31865Config(
            reference_resistance_ohms=430.0,
            wire_count=3,
            filter_frequency_hz=60,
            low_fault_threshold_ohms=threshold,
        )


@pytest.mark.parametrize("threshold", [0.0, -1.0, math.inf, -math.inf, math.nan])
def test_invalid_high_fault_threshold_is_rejected(threshold: float) -> None:
    with pytest.raises(ConfigurationError, match="finite and greater than zero"):
        MAX31865Config(
            reference_resistance_ohms=430.0,
            wire_count=3,
            filter_frequency_hz=60,
            high_fault_threshold_ohms=threshold,
        )


def test_low_fault_threshold_must_be_below_reference_resistance() -> None:
    with pytest.raises(
        ConfigurationError, match="less than reference_resistance_ohms"
    ):
        MAX31865Config(
            reference_resistance_ohms=430.0,
            wire_count=3,
            filter_frequency_hz=60,
            low_fault_threshold_ohms=430.0,
        )


def test_high_fault_threshold_must_be_below_reference_resistance() -> None:
    with pytest.raises(
        ConfigurationError, match="less than reference_resistance_ohms"
    ):
        MAX31865Config(
            reference_resistance_ohms=430.0,
            wire_count=3,
            filter_frequency_hz=60,
            high_fault_threshold_ohms=430.0,
        )


def test_low_fault_threshold_must_be_below_high_threshold() -> None:
    with pytest.raises(ConfigurationError, match="must be less than"):
        MAX31865Config(
            reference_resistance_ohms=430.0,
            wire_count=3,
            filter_frequency_hz=60,
            low_fault_threshold_ohms=160.0,
            high_fault_threshold_ohms=160.0,
        )
