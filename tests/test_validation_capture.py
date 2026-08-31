from __future__ import annotations

import json
from datetime import UTC, datetime
from pathlib import Path

import pytest

from rtd_acquire import (
    Diagnostic,
    DiagnosticCode,
    DiagnosticSeverity,
    Measurement,
    NativeEvidence,
)
from rtd_acquire.simulation import (
    SimulatedAcquisitionDevice,
    SimulatedAcquisitionFailure,
)
from validation import create_record
from validation.capture_max31865 import (
    capture_measurements,
    measurement_payload,
    summarize_rows,
    write_capture,
)

_FIXED_TIME = datetime(2026, 8, 30, 12, 0, tzinfo=UTC)


def _now() -> datetime:
    return _FIXED_TIME


def test_environment_metadata_avoids_host_identity(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
) -> None:
    monkeypatch.setattr(create_record, "_git_commit", lambda _: "abc123")
    monkeypatch.setattr(create_record, "_package_version", lambda _: "0.3.0")

    metadata = create_record.build_environment_metadata(
        created_at=_FIXED_TIME,
        repository_root=tmp_path,
    )

    assert metadata["created_at_utc"] == "2026-08-30T12:00:00+00:00"
    assert metadata["rtd_acquire"] == {
        "git_commit": "abc123",
        "package_version": "0.3.0",
    }
    runtime = metadata["runtime"]
    assert isinstance(runtime, dict)
    assert set(runtime) == {
        "python_version",
        "python_implementation",
        "sys_platform",
        "platform_system",
        "platform_release",
        "machine",
    }
    assert "hostname" not in json.dumps(metadata).lower()


def test_create_record_copies_template_and_refuses_overwrite(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
) -> None:
    monkeypatch.setattr(create_record, "_git_commit", lambda _: "abc123")
    monkeypatch.setattr(create_record, "_package_version", lambda _: "0.3.0")

    destination = create_record.create_record(
        record_id="pi4-pt100-range",
        output_root=tmp_path,
        repository_root=tmp_path,
        created_at=_FIXED_TIME,
    )

    assert (destination / "record.md").is_file()
    metadata = json.loads((destination / "environment.json").read_text())
    assert metadata["schema_version"] == 1
    assert metadata["rtd_acquire"]["git_commit"] == "abc123"

    with pytest.raises(FileExistsError, match="already exists"):
        create_record.create_record(
            record_id="pi4-pt100-range",
            output_root=tmp_path,
            repository_root=tmp_path,
            created_at=_FIXED_TIME,
        )


@pytest.mark.parametrize("record_id", ["", ".", "..", "nested/name"])
def test_create_record_rejects_unsafe_record_ids(
    record_id: str,
    tmp_path: Path,
) -> None:
    with pytest.raises(ValueError, match="record_id"):
        create_record.create_record(
            record_id=record_id,
            output_root=tmp_path,
            repository_root=tmp_path,
            created_at=_FIXED_TIME,
        )


def test_measurement_payload_preserves_diagnostic_native_evidence() -> None:
    measurement = Measurement(
        resistance_ohms=101.25,
        diagnostics=(
            Diagnostic(
                code=DiagnosticCode.RESISTANCE_HIGH_THRESHOLD,
                severity=DiagnosticSeverity.WARNING,
                native_evidence=(
                    NativeEvidence(identifier="MAX31865_FAULT_HIGH", message="D7=1"),
                ),
            ),
        ),
    )

    payload = measurement_payload(measurement)

    assert payload["resistance_ohms"] == 101.25
    assert payload["status"] == "warning"
    diagnostics = payload["diagnostics"]
    assert isinstance(diagnostics, list)
    assert diagnostics[0]["code"] == "resistance_high_threshold"
    assert diagnostics[0]["native_evidence"] == [
        {"identifier": "MAX31865_FAULT_HIGH", "message": "D7=1"}
    ]


def test_capture_and_summary_include_measurements_and_operation_errors() -> None:
    device = SimulatedAcquisitionDevice(
        [
            Measurement(resistance_ohms=100.0),
            SimulatedAcquisitionFailure("transport failed"),
            Measurement(resistance_ohms=102.0),
        ]
    )

    rows = capture_measurements(device, count=3, now=_now)
    summary = summarize_rows(rows)

    assert [row["outcome"] for row in rows] == [
        "measurement",
        "acquisition_error",
        "measurement",
    ]
    assert rows[1]["error"] == {
        "type": "AcquisitionError",
        "message": "transport failed",
    }
    assert summary["capture_count"] == 3
    assert summary["measurement_count"] == 2
    assert summary["operation_error_count"] == 1
    assert summary["error_type_counts"] == {"AcquisitionError": 1}
    resistance = summary["resistance"]
    assert isinstance(resistance, dict)
    assert resistance["mean_ohms"] == pytest.approx(101.0)
    assert resistance["sample_standard_deviation_ohms"] == pytest.approx(2**0.5)
    assert resistance["minimum_ohms"] == 100.0
    assert resistance["maximum_ohms"] == 102.0


def test_write_capture_writes_hash_manifest_and_refuses_overwrite(
    tmp_path: Path,
) -> None:
    (tmp_path / "record.md").write_text("# record\n", encoding="utf-8")
    rows = capture_measurements(
        SimulatedAcquisitionDevice([Measurement(resistance_ohms=100.0)]),
        count=1,
        now=_now,
    )

    raw_path, summary_path, manifest_path = write_capture(
        record_dir=tmp_path,
        label="pt100-low",
        rows=rows,
        configuration={"device": "MAX31865"},
    )

    assert raw_path.is_file()
    assert summary_path.is_file()
    manifest = json.loads(manifest_path.read_text())
    assert manifest["configuration"] == {"device": "MAX31865"}
    assert set(manifest["files"]) == {raw_path.name, summary_path.name}
    assert all(
        len(file_metadata["sha256"]) == 64
        for file_metadata in manifest["files"].values()
    )

    with pytest.raises(FileExistsError, match="already exists"):
        write_capture(
            record_dir=tmp_path,
            label="pt100-low",
            rows=rows,
            configuration={"device": "MAX31865"},
        )


def test_write_capture_requires_initialized_record(tmp_path: Path) -> None:
    rows = capture_measurements(
        SimulatedAcquisitionDevice([Measurement(resistance_ohms=100.0)]),
        count=1,
        now=_now,
    )

    with pytest.raises(FileNotFoundError, match="record.md"):
        write_capture(
            record_dir=tmp_path,
            label="pt100-low",
            rows=rows,
            configuration={"device": "MAX31865"},
        )
