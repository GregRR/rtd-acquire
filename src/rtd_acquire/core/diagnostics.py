"""Normalized acquisition diagnostics and native evidence."""

from __future__ import annotations

from dataclasses import dataclass
from enum import StrEnum
from typing import Final


class DiagnosticSeverity(StrEnum):
    """Severity of an acquisition diagnostic."""

    WARNING = "warning"
    FAULT = "fault"


class DiagnosticCode(StrEnum):
    """Stable normalized diagnostic identities used by rtd-acquire."""

    # Sensor / RTD input circuit
    SENSOR_CIRCUIT_OPEN = "sensor_circuit_open"
    SENSOR_CIRCUIT_SHORT = "sensor_circuit_short"
    SENSOR_INPUT_FAULT = "sensor_input_fault"
    SENSOR_NOT_DETECTED = "sensor_not_detected"
    SENSOR_BURNOUT = "sensor_burnout"
    SENSOR_DRIFT = "sensor_drift"
    LEAD_RESISTANCE_HIGH = "lead_resistance_high"

    # Resistance and generic input range
    RESISTANCE_HIGH_THRESHOLD = "resistance_high_threshold"
    RESISTANCE_LOW_THRESHOLD = "resistance_low_threshold"
    INPUT_OVERRANGE = "input_overrange"
    INPUT_UNDERRANGE = "input_underrange"

    # Reference and MAX31865-style electrical checks
    REFERENCE_LOW = "reference_low"
    REFERENCE_FAULT = "reference_fault"
    REFERENCE_INPUT_ABOVE_THRESHOLD = "reference_input_above_threshold"
    REFERENCE_INPUT_BELOW_THRESHOLD = "reference_input_below_threshold"
    RTD_INPUT_BELOW_THRESHOLD = "rtd_input_below_threshold"
    INPUT_VOLTAGE_FAULT = "input_voltage_fault"

    # PGA / ADC electrical conditions
    PGA_POSITIVE_OUTPUT_NEAR_POSITIVE_RAIL = (
        "pga_positive_output_near_positive_rail"
    )
    PGA_POSITIVE_OUTPUT_NEAR_NEGATIVE_RAIL = (
        "pga_positive_output_near_negative_rail"
    )
    PGA_NEGATIVE_OUTPUT_NEAR_POSITIVE_RAIL = (
        "pga_negative_output_near_positive_rail"
    )
    PGA_NEGATIVE_OUTPUT_NEAR_NEGATIVE_RAIL = (
        "pga_negative_output_near_negative_rail"
    )
    POSITIVE_INPUT_OVERVOLTAGE = "positive_input_overvoltage"
    POSITIVE_INPUT_UNDERVOLTAGE = "positive_input_undervoltage"
    NEGATIVE_INPUT_OVERVOLTAGE = "negative_input_overvoltage"
    NEGATIVE_INPUT_UNDERVOLTAGE = "negative_input_undervoltage"
    ADC_SATURATION = "adc_saturation"
    CONVERSION_ERROR = "conversion_error"
    CALIBRATION_ERROR = "calibration_error"
    ANALOG_SUPPLY_FAULT = "analog_supply_fault"
    DIGITAL_SUPPLY_FAULT = "digital_supply_fault"
    LDO_DECOUPLING_FAULT = "ldo_decoupling_fault"

    # Data integrity / device-internal communication and storage
    DATA_CRC_ERROR = "data_crc_error"
    SPI_CRC_ERROR = "spi_crc_error"
    SPI_CLOCK_COUNT_ERROR = "spi_clock_count_error"
    SPI_READ_ERROR = "spi_read_error"
    SPI_WRITE_ERROR = "spi_write_error"
    SPI_WRITE_IGNORED = "spi_write_ignored"
    REGISTER_INTEGRITY_ERROR = "register_integrity_error"
    ROM_INTEGRITY_ERROR = "rom_integrity_error"
    NONVOLATILE_MEMORY_ERROR = "nonvolatile_memory_error"
    CONFIGURATION_ERROR = "configuration_error"
    HARDWARE_FAULT = "hardware_fault"


_DIAGNOSTIC_MESSAGES: Final[dict[DiagnosticCode, str]] = {
    DiagnosticCode.SENSOR_CIRCUIT_OPEN: (
        "The acquisition device reports an open RTD input circuit."
    ),
    DiagnosticCode.SENSOR_CIRCUIT_SHORT: (
        "The acquisition device reports a shorted RTD input circuit."
    ),
    DiagnosticCode.SENSOR_INPUT_FAULT: (
        "The acquisition device reports an RTD input fault without identifying "
        "it more specifically."
    ),
    DiagnosticCode.SENSOR_NOT_DETECTED: (
        "The acquisition device reports that no sensor is detected."
    ),
    DiagnosticCode.SENSOR_BURNOUT: (
        "The acquisition device reports sensor burnout."
    ),
    DiagnosticCode.SENSOR_DRIFT: (
        "The acquisition device reports excessive sensor drift or disagreement."
    ),
    DiagnosticCode.LEAD_RESISTANCE_HIGH: (
        "The acquisition device reports excessive RTD lead resistance."
    ),
    DiagnosticCode.RESISTANCE_HIGH_THRESHOLD: (
        "The measured resistance met or exceeded the configured high threshold."
    ),
    DiagnosticCode.RESISTANCE_LOW_THRESHOLD: (
        "The measured resistance met or fell below the configured low threshold."
    ),
    DiagnosticCode.INPUT_OVERRANGE: (
        "The acquisition input is above its supported measurement range."
    ),
    DiagnosticCode.INPUT_UNDERRANGE: (
        "The acquisition input is below its supported measurement range."
    ),
    DiagnosticCode.REFERENCE_LOW: (
        "The acquisition reference is below a monitored threshold."
    ),
    DiagnosticCode.REFERENCE_FAULT: (
        "The acquisition device reports a reference fault without identifying it "
        "more specifically."
    ),
    DiagnosticCode.REFERENCE_INPUT_ABOVE_THRESHOLD: (
        "The reference input is above the device's monitored threshold."
    ),
    DiagnosticCode.REFERENCE_INPUT_BELOW_THRESHOLD: (
        "The reference input is below the device's monitored threshold."
    ),
    DiagnosticCode.RTD_INPUT_BELOW_THRESHOLD: (
        "The RTD input is below the device's monitored threshold."
    ),
    DiagnosticCode.INPUT_VOLTAGE_FAULT: (
        "The acquisition device reports an input overvoltage or undervoltage "
        "condition."
    ),
    DiagnosticCode.PGA_POSITIVE_OUTPUT_NEAR_POSITIVE_RAIL: (
        "The positive PGA output is near the positive supply rail."
    ),
    DiagnosticCode.PGA_POSITIVE_OUTPUT_NEAR_NEGATIVE_RAIL: (
        "The positive PGA output is near the negative supply rail."
    ),
    DiagnosticCode.PGA_NEGATIVE_OUTPUT_NEAR_POSITIVE_RAIL: (
        "The negative PGA output is near the positive supply rail."
    ),
    DiagnosticCode.PGA_NEGATIVE_OUTPUT_NEAR_NEGATIVE_RAIL: (
        "The negative PGA output is near the negative supply rail."
    ),
    DiagnosticCode.POSITIVE_INPUT_OVERVOLTAGE: (
        "The positive ADC input is above its allowed voltage range."
    ),
    DiagnosticCode.POSITIVE_INPUT_UNDERVOLTAGE: (
        "The positive ADC input is below its allowed voltage range."
    ),
    DiagnosticCode.NEGATIVE_INPUT_OVERVOLTAGE: (
        "The negative ADC input is above its allowed voltage range."
    ),
    DiagnosticCode.NEGATIVE_INPUT_UNDERVOLTAGE: (
        "The negative ADC input is below its allowed voltage range."
    ),
    DiagnosticCode.ADC_SATURATION: (
        "The acquisition device reports ADC saturation."
    ),
    DiagnosticCode.CONVERSION_ERROR: (
        "The acquisition device reports an invalid ADC conversion."
    ),
    DiagnosticCode.CALIBRATION_ERROR: (
        "The acquisition device reports an acquisition-calibration failure."
    ),
    DiagnosticCode.ANALOG_SUPPLY_FAULT: (
        "The acquisition device reports an analog supply fault."
    ),
    DiagnosticCode.DIGITAL_SUPPLY_FAULT: (
        "The acquisition device reports a digital supply fault."
    ),
    DiagnosticCode.LDO_DECOUPLING_FAULT: (
        "The acquisition device reports an LDO decoupling fault."
    ),
    DiagnosticCode.DATA_CRC_ERROR: (
        "Conversion data failed its CRC integrity check."
    ),
    DiagnosticCode.SPI_CRC_ERROR: (
        "The acquisition device reports an SPI CRC error."
    ),
    DiagnosticCode.SPI_CLOCK_COUNT_ERROR: (
        "The acquisition device reports an invalid SPI clock count."
    ),
    DiagnosticCode.SPI_READ_ERROR: (
        "The acquisition device reports an invalid SPI read operation."
    ),
    DiagnosticCode.SPI_WRITE_ERROR: (
        "The acquisition device reports an invalid SPI write operation."
    ),
    DiagnosticCode.SPI_WRITE_IGNORED: (
        "The acquisition device reports that an SPI write was ignored."
    ),
    DiagnosticCode.REGISTER_INTEGRITY_ERROR: (
        "The acquisition device reports a register-memory integrity error."
    ),
    DiagnosticCode.ROM_INTEGRITY_ERROR: (
        "The acquisition device reports a ROM integrity error."
    ),
    DiagnosticCode.NONVOLATILE_MEMORY_ERROR: (
        "The acquisition device reports a nonvolatile-memory integrity error."
    ),
    DiagnosticCode.CONFIGURATION_ERROR: (
        "The acquisition device reports an invalid or inconsistent configuration."
    ),
    DiagnosticCode.HARDWARE_FAULT: (
        "The acquisition device reports an internal hardware fault that cannot be "
        "identified more specifically."
    ),
}


def diagnostic_message(code: DiagnosticCode) -> str:
    """Return the canonical rtd-acquire message for *code*."""

    return _DIAGNOSTIC_MESSAGES[code]


@dataclass(frozen=True, slots=True)
class NativeEvidence:
    """Device- or protocol-native evidence supporting a normalized diagnostic."""

    identifier: str | None = None
    message: str | None = None

    def __post_init__(self) -> None:
        identifier_present = self.identifier is not None and bool(
            self.identifier.strip()
        )
        message_present = self.message is not None and bool(self.message.strip())
        if not identifier_present and not message_present:
            raise ValueError(
                "native evidence requires a non-empty identifier or message"
            )


@dataclass(frozen=True, slots=True)
class Diagnostic:
    """A normalized acquisition diagnostic with optional native evidence."""

    code: DiagnosticCode
    severity: DiagnosticSeverity
    native_evidence: tuple[NativeEvidence, ...] = ()

    @property
    def message(self) -> str:
        """Return the canonical rtd-acquire wording for this diagnostic."""

        return diagnostic_message(self.code)
