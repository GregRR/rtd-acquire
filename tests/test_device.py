from __future__ import annotations

import pytest

from rtd_acquire import (
    AcquisitionDevice,
    AcquisitionError,
    ConfigurationError,
    Measurement,
    RtdAcquireError,
)


class ExampleDevice:
    """Minimal structural implementation used to exercise the public protocol."""

    def __init__(self, measurement: Measurement) -> None:
        self._measurement = measurement

    def read(self) -> Measurement:
        return self._measurement


def read_once(device: AcquisitionDevice) -> Measurement:
    """Exercise structural typing without requiring protocol inheritance."""

    return device.read()


def test_acquisition_device_uses_structural_typing() -> None:
    expected = Measurement(resistance_ohms=109.73)
    device = ExampleDevice(expected)

    assert read_once(device) is expected


@pytest.mark.parametrize("error_type", [ConfigurationError, AcquisitionError])
def test_public_errors_share_package_base(error_type: type[Exception]) -> None:
    assert issubclass(error_type, RtdAcquireError)


def test_configuration_error_is_distinct_from_acquisition_error() -> None:
    assert not issubclass(ConfigurationError, AcquisitionError)
