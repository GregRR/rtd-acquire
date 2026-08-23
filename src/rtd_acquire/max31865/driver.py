"""Platform-independent MAX31865 acquisition driver.

Implementation basis: Analog Devices (2015), MAX31865 data sheet; see
``docs/REFERENCES.md``.
"""

from __future__ import annotations

import math
import time
from collections.abc import Callable
from contextlib import suppress
from dataclasses import dataclass

from ..core import AcquisitionError, ConfigurationError, Measurement
from ..transports import SpiBitOrder, SpiDevice
from ._decode import measurement_from_registers
from ._thresholds import (
    encode_high_threshold_register,
    encode_low_threshold_register,
)
from .config import MAX31865Config

_MAX_SPI_CLOCK_HZ = 5_000_000
_DEFAULT_INPUT_FILTER_TIME_CONSTANT_SECONDS = 0.001
_FAULT_CYCLE_MAX_SECONDS = 0.0006
_SINGLE_CONVERSION_SECONDS = {50: 0.066, 60: 0.055}

_CONFIG_WRITE = 0x80
_RTD_READ = 0x01
_HIGH_THRESHOLD_WRITE = 0x83
_FAULT_STATUS_READ = 0x07

_CONFIG_BIAS = 0x80
_CONFIG_ONE_SHOT = 0x20
_CONFIG_THREE_WIRE = 0x10
_CONFIG_AUTO_FAULT_CYCLE = 0x04
_CONFIG_CLEAR_FAULTS = 0x02
_CONFIG_FILTER_50HZ = 0x01



@dataclass(frozen=True, slots=True)
class MAX31865Timing:
    """Timing policy for the MAX31865 input network and one-shot reads.

    The default 1 ms input-filter time constant is conservative for the
    datasheet's documented startup characterization. Hardware with a larger
    external RC time constant must provide its actual value.
    """

    input_filter_time_constant_seconds: float = (
        _DEFAULT_INPUT_FILTER_TIME_CONSTANT_SECONDS
    )

    def __post_init__(self) -> None:
        value = self.input_filter_time_constant_seconds
        if not math.isfinite(value) or value < 0.0:
            raise ConfigurationError(
                "input_filter_time_constant_seconds must be finite and non-negative"
            )

    @property
    def bias_settle_seconds(self) -> float:
        """Minimum delay after enabling VBIAS before starting diagnostics."""

        return 10.5 * self.input_filter_time_constant_seconds + 0.001

    @property
    def post_fault_settle_seconds(self) -> float:
        """Minimum datasheet delay after fault detection before ADC restart."""

        return 5.0 * self.input_filter_time_constant_seconds + 0.001


class MAX31865:
    """Acquire RTD resistance from a MAX31865 through an injected SPI device."""

    def __init__(
        self,
        spi: SpiDevice,
        config: MAX31865Config,
        *,
        timing: MAX31865Timing | None = None,
        sleep: Callable[[float], None] = time.sleep,
    ) -> None:
        self._spi = spi
        self._config = config
        self._timing = timing if timing is not None else MAX31865Timing()
        self._sleep = sleep
        self._validate_spi_settings()

    def read(self) -> Measurement:
        """Perform one fault-checked one-shot resistance acquisition."""

        base_config = self._base_config_byte()
        self._write_thresholds()

        try:
            self._write_config(base_config | _CONFIG_BIAS | _CONFIG_CLEAR_FAULTS)
            self._sleep(self._timing.bias_settle_seconds)
            self._write_config(base_config | _CONFIG_BIAS | _CONFIG_AUTO_FAULT_CYCLE)
            self._sleep(_FAULT_CYCLE_MAX_SECONDS)
            self._sleep(self._timing.post_fault_settle_seconds)

            self._write_config(base_config | _CONFIG_BIAS | _CONFIG_ONE_SHOT)
            self._sleep(_SINGLE_CONVERSION_SECONDS[self._config.filter_frequency_hz])

            rtd_bytes = self._read_registers(_RTD_READ, 2)
            rtd_register = int.from_bytes(rtd_bytes, "big")
            fault_status = 0
            if rtd_register & 0x0001:
                fault_status = self._read_registers(_FAULT_STATUS_READ, 1)[0]
        except Exception:
            self._best_effort_bias_off(base_config)
            raise

        self._write_config(base_config)
        return measurement_from_registers(
            self._config,
            rtd_register=rtd_register,
            fault_status_register=fault_status,
        )

    def _validate_spi_settings(self) -> None:
        settings = self._spi.settings
        if settings.clock_phase != 1:
            raise ConfigurationError("MAX31865 requires SPI clock_phase=1")
        if settings.clock_frequency_hz > _MAX_SPI_CLOCK_HZ:
            raise ConfigurationError(
                "MAX31865 SPI clock_frequency_hz must be <= 5000000"
            )
        if settings.bit_order is not SpiBitOrder.MSB_FIRST:
            raise ConfigurationError("MAX31865 requires MSB-first SPI transfers")
        if settings.bits_per_word != 8:
            raise ConfigurationError("MAX31865 requires 8-bit SPI words")
        if not settings.chip_select_active_low:
            raise ConfigurationError("MAX31865 requires active-low chip select")

    def _base_config_byte(self) -> int:
        value = 0
        if self._config.wire_count == 3:
            value |= _CONFIG_THREE_WIRE
        if self._config.filter_frequency_hz == 50:
            value |= _CONFIG_FILTER_50HZ
        return value

    def _write_thresholds(self) -> None:
        high = encode_high_threshold_register(
            self._config.high_fault_threshold_ohms,
            self._config.reference_resistance_ohms,
        )
        low = encode_low_threshold_register(
            self._config.low_fault_threshold_ohms,
            self._config.reference_resistance_ohms,
        )
        self._write_registers(
            _HIGH_THRESHOLD_WRITE,
            high.to_bytes(2, "big") + low.to_bytes(2, "big"),
        )

    def _write_config(self, value: int) -> None:
        self._write_registers(_CONFIG_WRITE, bytes((value,)))

    def _write_registers(self, write_address: int, data: bytes) -> None:
        self._transfer_exact(bytes((write_address,)) + data)

    def _read_registers(self, read_address: int, count: int) -> bytes:
        response = self._transfer_exact(bytes((read_address,)) + bytes(count))
        return response[1:]

    def _transfer_exact(self, tx: bytes) -> bytes:
        response = self._spi.transfer(tx)
        if not isinstance(response, bytes):
            raise AcquisitionError("SPI transfer must return bytes")
        if len(response) != len(tx):
            raise AcquisitionError(
                "SPI transfer returned an unexpected number of bytes"
            )
        return response

    def _best_effort_bias_off(self, base_config: int) -> None:
        with suppress(Exception):
            self._write_config(base_config)
