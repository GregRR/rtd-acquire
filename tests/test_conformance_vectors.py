from __future__ import annotations

import json
import struct
import sys
from pathlib import Path
from typing import Literal, cast

import pytest

from rtd_acquire import ConfigurationError
from rtd_acquire.core import DiagnosticCode, DiagnosticSeverity, MeasurementStatus
from rtd_acquire.max31865 import MAX31865Config
from rtd_acquire.max31865._decode import measurement_from_registers
from rtd_acquire.max31865._thresholds import (
    encode_high_threshold_register,
    encode_low_threshold_register,
)

_CONFORMANCE_ROOT = Path(__file__).parents[1] / "conformance" / "v1"


def _object(value: object) -> dict[str, object]:
    assert isinstance(value, dict)
    assert all(isinstance(key, str) for key in value)
    return cast(dict[str, object], value)


def _list(value: object) -> list[object]:
    assert isinstance(value, list)
    return cast(list[object], value)


def _load_json(path: Path) -> dict[str, object]:
    value: object = json.loads(path.read_text(encoding="utf-8"))
    return _object(value)


def _binary32(value: float) -> float:
    return cast(float, struct.unpack("!f", struct.pack("!f", value))[0])


def _binary32_configuration(
    configuration: dict[str, object],
) -> dict[str, object]:
    result = dict(configuration)
    for key in (
        "reference_resistance_ohms",
        "low_fault_threshold_ohms",
        "high_fault_threshold_ohms",
    ):
        value = result[key]
        if value is not None:
            assert isinstance(value, (int, float))
            result[key] = _binary32(float(value))
    return result


def _configuration_is_valid(configuration: dict[str, object]) -> bool:
    try:
        _max31865_config(configuration)
    except ConfigurationError:
        return False
    return True


def _max31865_config(configuration: dict[str, object]) -> MAX31865Config:
    return MAX31865Config(
        reference_resistance_ohms=cast(
            float, configuration["reference_resistance_ohms"]
        ),
        wire_count=cast(Literal[2, 3, 4], configuration["wire_count"]),
        filter_frequency_hz=cast(Literal[50, 60], configuration["filter_frequency_hz"]),
        low_fault_threshold_ohms=cast(
            float | None, configuration["low_fault_threshold_ohms"]
        ),
        high_fault_threshold_ohms=cast(
            float | None, configuration["high_fault_threshold_ohms"]
        ),
    )


def test_manifest_lists_existing_version_one_vector_sets() -> None:
    manifest = _load_json(_CONFORMANCE_ROOT / "manifest.json")

    assert manifest["schema_version"] == 1
    vector_sets = _list(manifest["vector_sets"])
    assert vector_sets

    for entry_value in vector_sets:
        entry = _object(entry_value)
        assert isinstance(entry["device"], str)
        assert entry["operation"] in {"measurement_decode", "threshold_encoding"}
        path = entry["path"]
        assert isinstance(path, str)
        assert (_CONFORMANCE_ROOT / path).is_file()

    numeric_profiles = _list(manifest["numeric_profiles"])
    assert len(numeric_profiles) == 1
    profile_entry = _object(numeric_profiles[0])
    assert profile_entry["profile_id"] == "python-binary64-c-binary32"
    profile_path = profile_entry["path"]
    assert isinstance(profile_path, str)
    assert (_CONFORMANCE_ROOT / profile_path).is_file()


def test_binary64_binary32_numeric_profile_is_frozen() -> None:
    profile = _load_json(_CONFORMANCE_ROOT / "numeric_profiles.json")

    assert profile["schema_version"] == 1
    assert profile["profile_id"] == "python-binary64-c-binary32"

    requirements = _object(profile["requirements"])
    assert requirements == {
        "python_float_radix": 2,
        "python_float_mantissa_bits": 53,
        "python_float_max_exponent": 1024,
        "c_float_radix": 2,
        "c_float_mantissa_bits": 24,
        "c_float_max_exponent": 128,
    }
    assert sys.float_info.radix == requirements["python_float_radix"]
    assert sys.float_info.mant_dig == requirements["python_float_mantissa_bits"]
    assert sys.float_info.max_exp == requirements["python_float_max_exponent"]

    measurement_decode = _object(profile["measurement_decode"])
    resistance = _object(measurement_decode["resistance_ohms"])
    assert resistance == {
        "relative_tolerance": 2.384185791015625e-7,
        "absolute_tolerance_ohms": 0.0,
        "expected_zero_requires_exact_zero": True,
    }

    validation = _object(profile["configuration_validation"])
    assert validation == {
        "cross_language_vectors_must_be_binary32_stable": True,
    }


def test_cross_language_vector_configurations_are_binary32_stable() -> None:
    for file_name in ("max31865.json", "max31865_threshold_encoding.json"):
        document = _load_json(_CONFORMANCE_ROOT / file_name)
        for vector_value in _list(document["vectors"]):
            vector = _object(vector_value)
            configuration = _object(vector["configuration"])
            original_valid = _configuration_is_valid(configuration)
            binary32_valid = _configuration_is_valid(
                _binary32_configuration(configuration)
            )
            assert binary32_valid is original_valid, vector["id"]


def test_measurement_vectors_exercise_non_binary32_numeric_profile() -> None:
    document = _load_json(_CONFORMANCE_ROOT / "max31865.json")
    vectors = _list(document["vectors"])
    vector = next(
        _object(value)
        for value in vectors
        if _object(value)["id"] == "max31865-non-binary32-reference-ok"
    )
    expected = _object(vector["expected"])
    resistance = expected["resistance_ohms"]
    assert isinstance(resistance, float)
    configuration = _object(vector["configuration"])
    native_input = _object(vector["native_input"])
    reference = configuration["reference_resistance_ohms"]
    rtd_register = native_input["rtd_register"]
    assert isinstance(reference, float)
    assert isinstance(rtd_register, int)
    assert (rtd_register >> 1) / 32768.0 * reference == resistance
    assert _binary32(resistance) != resistance


def test_max31865_vectors_have_unique_ids_and_valid_contract_values() -> None:
    document = _load_json(_CONFORMANCE_ROOT / "max31865.json")

    assert document["schema_version"] == 1
    assert document["device"] == "max31865"
    assert document["operation"] == "measurement_decode"
    vectors = _list(document["vectors"])
    ids: set[str] = set()

    for vector_value in vectors:
        vector = _object(vector_value)
        vector_id = vector["id"]
        assert isinstance(vector_id, str)
        assert vector_id not in ids
        ids.add(vector_id)

        configuration = _object(vector["configuration"])
        MAX31865Config(
            reference_resistance_ohms=cast(
                float, configuration["reference_resistance_ohms"]
            ),
            wire_count=cast(Literal[2, 3, 4], configuration["wire_count"]),
            filter_frequency_hz=cast(
                Literal[50, 60], configuration["filter_frequency_hz"]
            ),
            low_fault_threshold_ohms=cast(
                float | None, configuration["low_fault_threshold_ohms"]
            ),
            high_fault_threshold_ohms=cast(
                float | None, configuration["high_fault_threshold_ohms"]
            ),
        )

        native_input = _object(vector["native_input"])
        rtd_register = native_input["rtd_register"]
        fault_status = native_input["fault_status_register"]
        assert isinstance(rtd_register, int)
        assert 0 <= rtd_register <= 0xFFFF
        assert isinstance(fault_status, int)
        assert 0 <= fault_status <= 0xFF

        expected = _object(vector["expected"])
        status = expected["status"]
        assert status in {item.value for item in MeasurementStatus}

        resistance = expected["resistance_ohms"]
        assert resistance is None or (
            isinstance(resistance, (int, float)) and resistance >= 0
        )

        uncertainty = expected["standard_uncertainty_ohms"]
        assert uncertainty is None or (
            isinstance(uncertainty, (int, float)) and uncertainty >= 0
        )

        diagnostics = _list(expected["diagnostics"])
        severities: list[str] = []
        for diagnostic_value in diagnostics:
            diagnostic = _object(diagnostic_value)
            assert diagnostic["code"] in {item.value for item in DiagnosticCode}
            severity = diagnostic["severity"]
            assert severity in {item.value for item in DiagnosticSeverity}
            assert isinstance(severity, str)
            severities.append(severity)

            native_evidence = _list(diagnostic["native_evidence"])
            for evidence_value in native_evidence:
                evidence = _object(evidence_value)
                identifier = evidence["identifier"]
                message = evidence["message"]
                assert identifier is None or (
                    isinstance(identifier, str) and identifier.strip()
                )
                assert message is None or (isinstance(message, str) and message.strip())
                assert identifier is not None or message is not None

        if DiagnosticSeverity.FAULT.value in severities:
            assert status == MeasurementStatus.FAULT.value
            assert resistance is None
        elif diagnostics:
            assert status == MeasurementStatus.WARNING.value
            assert resistance is not None
        else:
            assert status == MeasurementStatus.OK.value
            assert resistance is not None


def test_max31865_seed_vectors_cover_ok_warning_and_fault() -> None:
    document = _load_json(_CONFORMANCE_ROOT / "max31865.json")
    vectors = _list(document["vectors"])

    statuses = {
        cast(str, _object(_object(vector)["expected"])["status"]) for vector in vectors
    }

    assert statuses == {"ok", "warning", "fault"}


def test_max31865_quarter_scale_reference_value_is_exact() -> None:
    document = _load_json(_CONFORMANCE_ROOT / "max31865.json")
    vectors = _list(document["vectors"])
    first = _object(vectors[0])
    configuration = _object(first["configuration"])
    native_input = _object(first["native_input"])
    expected = _object(first["expected"])

    reference = cast(float, configuration["reference_resistance_ohms"])
    rtd_register = cast(int, native_input["rtd_register"])
    resistance = (rtd_register >> 1) / 32768.0 * reference

    assert resistance == expected["resistance_ohms"] == 107.5


def test_max31865_vectors_execute_against_python_decoder() -> None:
    document = _load_json(_CONFORMANCE_ROOT / "max31865.json")
    vectors = _list(document["vectors"])

    for vector_value in vectors:
        vector = _object(vector_value)
        configuration = _object(vector["configuration"])
        config = MAX31865Config(
            reference_resistance_ohms=cast(
                float, configuration["reference_resistance_ohms"]
            ),
            wire_count=cast(Literal[2, 3, 4], configuration["wire_count"]),
            filter_frequency_hz=cast(
                Literal[50, 60], configuration["filter_frequency_hz"]
            ),
            low_fault_threshold_ohms=cast(
                float | None, configuration["low_fault_threshold_ohms"]
            ),
            high_fault_threshold_ohms=cast(
                float | None, configuration["high_fault_threshold_ohms"]
            ),
        )
        native_input = _object(vector["native_input"])
        expected = _object(vector["expected"])

        measurement = measurement_from_registers(
            config,
            rtd_register=cast(int, native_input["rtd_register"]),
            fault_status_register=cast(int, native_input["fault_status_register"]),
        )

        assert measurement.status.value == expected["status"]
        expected_resistance = expected["resistance_ohms"]
        if expected_resistance is None:
            assert measurement.resistance_ohms is None
        else:
            assert measurement.resistance_ohms == pytest.approx(
                cast(float, expected_resistance)
            )

        expected_diagnostics = _list(expected["diagnostics"])
        assert len(measurement.diagnostics) == len(expected_diagnostics)
        for actual, expected_value in zip(
            measurement.diagnostics, expected_diagnostics, strict=True
        ):
            expected_diagnostic = _object(expected_value)
            assert actual.code.value == expected_diagnostic["code"]
            assert actual.severity.value == expected_diagnostic["severity"]
            expected_native = _list(expected_diagnostic["native_evidence"])
            assert len(actual.native_evidence) == len(expected_native)
            for actual_evidence, expected_evidence_value in zip(
                actual.native_evidence, expected_native, strict=True
            ):
                expected_evidence = _object(expected_evidence_value)
                assert actual_evidence.identifier == expected_evidence["identifier"]
                assert actual_evidence.message == expected_evidence["message"]


def test_max31865_threshold_vectors_execute_against_python_encoder() -> None:
    document = _load_json(_CONFORMANCE_ROOT / "max31865_threshold_encoding.json")

    assert document["schema_version"] == 1
    assert document["device"] == "max31865"
    assert document["operation"] == "threshold_encoding"
    vectors = _list(document["vectors"])
    ids: set[str] = set()

    for vector_value in vectors:
        vector = _object(vector_value)
        vector_id = vector["id"]
        assert isinstance(vector_id, str)
        assert vector_id not in ids
        ids.add(vector_id)

        configuration = _object(vector["configuration"])
        expected = _object(vector["expected"])
        outcome = expected["outcome"]
        assert outcome in {"registers", "configuration_error"}

        if outcome == "configuration_error":
            with pytest.raises(ConfigurationError):
                _max31865_config(configuration)
            continue

        config = _max31865_config(configuration)
        high = encode_high_threshold_register(
            config.high_fault_threshold_ohms,
            config.reference_resistance_ohms,
        )
        low = encode_low_threshold_register(
            config.low_fault_threshold_ohms,
            config.reference_resistance_ohms,
        )

        assert high == expected["high_threshold_register"]
        assert low == expected["low_threshold_register"]
