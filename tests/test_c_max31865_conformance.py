from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path
from typing import cast

import pytest

_ROOT = Path(__file__).resolve().parents[1]
_MEASUREMENT_VECTORS = _ROOT / "conformance" / "v1" / "max31865.json"
_THRESHOLD_VECTORS = (
    _ROOT / "conformance" / "v1" / "max31865_threshold_encoding.json"
)


def _object(value: object) -> dict[str, object]:
    assert isinstance(value, dict)
    return cast(dict[str, object], value)


def _list(value: object) -> list[object]:
    assert isinstance(value, list)
    return cast(list[object], value)


def _token(value: object) -> str:
    if value is None:
        return "none"
    assert isinstance(value, (int, float))
    return repr(value)


def _compile_runner(
    compiler: str,
    output: Path,
    runner_source: str,
) -> Path:
    subprocess.run(
        [
            compiler,
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-pedantic",
            "-I",
            str(_ROOT / "c" / "include"),
            str(_ROOT / "c" / "src" / "core.c"),
            str(_ROOT / "c" / "src" / "max31865.c"),
            str(_ROOT / "c" / "tests" / runner_source),
            "-o",
            str(output),
        ],
        check=True,
        cwd=_ROOT,
    )
    return output


@pytest.fixture(scope="module")
def c_threshold_runner(tmp_path_factory: pytest.TempPathFactory) -> Path:
    compiler = shutil.which("cc")
    if compiler is None:
        pytest.skip("portable C conformance requires a C compiler")

    output = tmp_path_factory.mktemp("c-conformance") / "max31865-thresholds"
    return _compile_runner(
        compiler,
        output,
        "max31865_threshold_vector_runner.c",
    )


@pytest.fixture(scope="module")
def c_measurement_runner(tmp_path_factory: pytest.TempPathFactory) -> Path:
    compiler = shutil.which("cc")
    if compiler is None:
        pytest.skip("portable C conformance requires a C compiler")

    output = tmp_path_factory.mktemp("c-conformance") / "max31865-measurements"
    return _compile_runner(
        compiler,
        output,
        "max31865_measurement_vector_runner.c",
    )


def test_max31865_threshold_vectors_execute_against_c(
    c_threshold_runner: Path,
) -> None:
    document = _object(json.loads(_THRESHOLD_VECTORS.read_text(encoding="utf-8")))
    vectors = _list(document["vectors"])

    for vector_value in vectors:
        vector = _object(vector_value)
        configuration = _object(vector["configuration"])
        expected = _object(vector["expected"])

        result = subprocess.run(
            [
                str(c_threshold_runner),
                _token(configuration["reference_resistance_ohms"]),
                _token(configuration["wire_count"]),
                _token(configuration["filter_frequency_hz"]),
                _token(configuration["low_fault_threshold_ohms"]),
                _token(configuration["high_fault_threshold_ohms"]),
            ],
            check=True,
            capture_output=True,
            text=True,
            cwd=_ROOT,
        ).stdout.strip()

        if expected["outcome"] == "configuration_error":
            assert result == "configuration_error"
            continue

        assert result.startswith("registers ")
        _, high_text, low_text = result.split()
        assert int(high_text) == expected["high_threshold_register"]
        assert int(low_text) == expected["low_threshold_register"]


def test_max31865_measurement_vectors_execute_against_c(
    c_measurement_runner: Path,
) -> None:
    document = _object(json.loads(_MEASUREMENT_VECTORS.read_text(encoding="utf-8")))
    vectors = _list(document["vectors"])

    for vector_value in vectors:
        vector = _object(vector_value)
        configuration = _object(vector["configuration"])
        native_input = _object(vector["native_input"])
        expected = _object(vector["expected"])

        lines = subprocess.run(
            [
                str(c_measurement_runner),
                _token(configuration["reference_resistance_ohms"]),
                _token(native_input["rtd_register"]),
                _token(native_input["fault_status_register"]),
            ],
            check=True,
            capture_output=True,
            text=True,
            cwd=_ROOT,
        ).stdout.splitlines()

        assert lines[0] == f"status {expected['status']}"

        expected_resistance = expected["resistance_ohms"]
        if expected_resistance is None:
            assert lines[1] == "resistance none"
        else:
            assert isinstance(expected_resistance, (int, float))
            assert float(lines[1].split(maxsplit=1)[1]) == float(
                expected_resistance
            )

        assert expected["standard_uncertainty_ohms"] is None
        assert lines[2] == "uncertainty none"

        expected_diagnostics = _list(expected["diagnostics"])
        assert lines[3] == f"diagnostics {len(expected_diagnostics)}"

        line_index = 4
        for diagnostic_value in expected_diagnostics:
            diagnostic = _object(diagnostic_value)
            native_evidence = _list(diagnostic["native_evidence"])
            assert lines[line_index] == (
                f"diagnostic {diagnostic['code']} {diagnostic['severity']} "
                f"{len(native_evidence)}"
            )
            line_index += 1

            for evidence_value in native_evidence:
                evidence = _object(evidence_value)
                identifier = evidence["identifier"]
                message = evidence["message"]
                assert lines[line_index] == (
                    "evidence "
                    f"{'none' if identifier is None else identifier}\t"
                    f"{'none' if message is None else message}"
                )
                line_index += 1

        assert line_index == len(lines)
