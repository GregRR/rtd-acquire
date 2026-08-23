"""Deterministic SPI-level MAX31865 emulator for tests and development.

Register semantics are based on Analog Devices (2015), MAX31865 data sheet;
see ``docs/REFERENCES.md``. The emulator intentionally models only the native
transactions required by the rtd-acquire driver rather than analog RTD physics.
"""

from __future__ import annotations

from ..core import AcquisitionError, ConfigurationError
from ..transports import SpiSettings

_MAX_ADC_CODE = 0x7FFF
_DEFINED_FAULT_MASK = 0xFC

_CONFIG_WRITE = 0x80
_RTD_READ = 0x01
_HIGH_THRESHOLD_WRITE = 0x83
_FAULT_STATUS_READ = 0x07

_CONFIG_ONE_SHOT = 0x20
_CONFIG_AUTO_FAULT_CYCLE = 0x04
_CONFIG_CLEAR_FAULTS = 0x02


class MAX31865SpiEmulator:
    """Emulate the MAX31865 SPI behavior exercised by the Python driver.

    ``rtd_code`` is the native 15-bit converter code, before the RTD register's
    fault-indicator bit is appended. ``fault_status_register`` is the native
    D7-D2 fault state that is re-latched when the driver runs an automatic
    fault-detection cycle.

    This is a deterministic register/transport emulator. It does not model
    sensor temperature, circuit noise, settling physics, or the analog causes
    of native fault bits.
    """

    def __init__(
        self,
        *,
        rtd_code: int,
        fault_status_register: int = 0,
    ) -> None:
        self._rtd_code = _validate_rtd_code(rtd_code)
        self._configured_fault_status = _validate_fault_status(fault_status_register)
        self._latched_fault_status = 0
        self._transactions: list[bytes] = []
        self._high_threshold_register = 0xFFFF
        self._low_threshold_register = 0x0000
        self._last_config_write = 0
        self._conversion_ready = False
        self._settings = SpiSettings(
            clock_polarity=0,
            clock_phase=1,
            clock_frequency_hz=1_000_000,
        )

    @property
    def settings(self) -> SpiSettings:
        """Return MAX31865-compatible deterministic SPI settings."""

        return self._settings

    @property
    def transactions(self) -> tuple[bytes, ...]:
        """Return the SPI transactions observed so far."""

        return tuple(self._transactions)

    @property
    def high_threshold_register(self) -> int:
        """Return the most recently written native high-threshold register."""

        return self._high_threshold_register

    @property
    def low_threshold_register(self) -> int:
        """Return the most recently written native low-threshold register."""

        return self._low_threshold_register

    @property
    def last_config_write(self) -> int:
        """Return the most recent configuration byte written by the driver."""

        return self._last_config_write

    def transfer(self, tx: bytes, /) -> bytes:
        """Execute one deterministic MAX31865 SPI transaction."""

        if not isinstance(tx, bytes):
            raise AcquisitionError("MAX31865 emulator transfer must receive bytes")
        if not tx:
            raise AcquisitionError("MAX31865 emulator transfer must not be empty")

        self._transactions.append(tx)
        address = tx[0]
        if address & 0x80:
            self._write(address, tx[1:])
            return bytes(len(tx))
        return self._read(address, len(tx) - 1)

    def _write(self, address: int, data: bytes) -> None:
        if address == _CONFIG_WRITE and len(data) == 1:
            self._write_config(data[0])
            return
        if address == _HIGH_THRESHOLD_WRITE and len(data) == 4:
            self._high_threshold_register = int.from_bytes(data[:2], "big")
            self._low_threshold_register = int.from_bytes(data[2:], "big")
            return
        raise AcquisitionError(
            "MAX31865 emulator received an unsupported register write"
        )

    def _write_config(self, value: int) -> None:
        self._last_config_write = value
        if value & _CONFIG_CLEAR_FAULTS:
            self._latched_fault_status = 0
        if value & _CONFIG_AUTO_FAULT_CYCLE:
            self._latched_fault_status = self._configured_fault_status
        if value & _CONFIG_ONE_SHOT:
            self._conversion_ready = True

    def _read(self, address: int, count: int) -> bytes:
        if address == _RTD_READ and count == 2:
            if not self._conversion_ready:
                raise AcquisitionError(
                    "MAX31865 emulator RTD data read requires a one-shot conversion"
                )
            self._conversion_ready = False
            fault_flag = 1 if self._latched_fault_status else 0
            wire_value = (self._rtd_code << 1) | fault_flag
            return b"\x00" + wire_value.to_bytes(2, "big")
        if address == _FAULT_STATUS_READ and count == 1:
            return bytes((0, self._latched_fault_status))
        raise AcquisitionError(
            "MAX31865 emulator received an unsupported register read"
        )


def _validate_rtd_code(value: int) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise ConfigurationError("rtd_code must be an integer")
    if not 0 <= value <= _MAX_ADC_CODE:
        raise ConfigurationError("rtd_code must be between 0 and 32767")
    return value


def _validate_fault_status(value: int) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise ConfigurationError("fault_status_register must be an integer")
    if not 0 <= value <= 0xFF:
        raise ConfigurationError("fault_status_register must be between 0 and 255")
    if value & ~_DEFINED_FAULT_MASK:
        raise ConfigurationError(
            "fault_status_register may contain only defined D7-D2 fault bits"
        )
    return value
