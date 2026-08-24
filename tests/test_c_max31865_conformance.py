from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path
from typing import cast

import pytest

_ROOT = Path(__file__).resolve().parents[1]
_VECTORS = _ROOT / "conformance" / "v1" / "max31865_threshold_encoding.json"


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


@pytest.fixture(scope="module")
def c_threshold_runner(tmp_path_factory: pytest.TempPathFactory) -> Path:
    compiler = shutil.which("cc")
    if compiler is None:
        pytest.skip("portable C conformance requires a C compiler")

    output = tmp_path_factory.mktemp("c-conformance") / "max31865-thresholds"
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
            str(_ROOT / "c" / "src" / "max31865.c"),
            str(_ROOT / "c" / "tests" / "max31865_threshold_vector_runner.c"),
            "-o",
            str(output),
        ],
        check=True,
        cwd=_ROOT,
    )
    return output


def test_max31865_threshold_vectors_execute_against_c(
    c_threshold_runner: Path,
) -> None:
    document = _object(json.loads(_VECTORS.read_text(encoding="utf-8")))
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
