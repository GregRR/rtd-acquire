from __future__ import annotations

import json
from pathlib import Path
from typing import Literal, cast

import pytest

from rtd_acquire.core import DiagnosticCode, DiagnosticSeverity, MeasurementStatus
from rtd_acquire.max31865 import MAX31865Config
from rtd_acquire.max31865._decode import measurement_from_registers

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


def test_manifest_lists_existing_version_one_vector_sets() -> None:
    manifest = _load_json(_CONFORMANCE_ROOT / "manifest.json")

    assert manifest["schema_version"] == 1
    vector_sets = _list(manifest["vector_sets"])
    assert vector_sets

    for entry_value in vector_sets:
        entry = _object(entry_value)
        assert isinstance(entry["device"], str)
        path = entry["path"]
        assert isinstance(path, str)
        assert (_CONFORMANCE_ROOT / path).is_file()


def test_max31865_vectors_have_unique_ids_and_valid_contract_values() -> None:
    document = _load_json(_CONFORMANCE_ROOT / "max31865.json")

    assert document["schema_version"] == 1
    assert document["device"] == "max31865"
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
        cast(str, _object(_object(vector)["expected"])["status"])
        for vector in vectors
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
            fault_status_register=cast(
                int, native_input["fault_status_register"]
            ),
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
