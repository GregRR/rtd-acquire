"""Capture structured MAX31865 measurements for physical validation."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import statistics
from collections import Counter
from collections.abc import Callable
from datetime import UTC, datetime
from pathlib import Path
from typing import Any, Protocol

from rtd_acquire import Measurement, RtdAcquireError
from rtd_acquire.max31865 import MAX31865, MAX31865Config, MAX31865Timing
from rtd_acquire.transports import LinuxSpidevDevice, SpiSettings


class _ReadableDevice(Protocol):
    def read(self) -> Measurement: ...


def _utc_now() -> datetime:
    return datetime.now(UTC)


def _safe_label(value: str) -> str:
    if not value or value in {".", ".."} or Path(value).name != value:
        raise ValueError("label must be a non-empty filename component")
    if not all(character.isalnum() or character in "-_." for character in value):
        raise ValueError("label may contain only letters, numbers, '-', '_', and '.'")
    return value


def measurement_payload(measurement: Measurement) -> dict[str, Any]:
    """Serialize one public Measurement without losing diagnostic evidence."""

    return {
        "resistance_ohms": measurement.resistance_ohms,
        "standard_uncertainty_ohms": measurement.standard_uncertainty_ohms,
        "status": measurement.status.value,
        "diagnostics": [
            {
                "code": diagnostic.code.value,
                "severity": diagnostic.severity.value,
                "message": diagnostic.message,
                "native_evidence": [
                    {
                        "identifier": evidence.identifier,
                        "message": evidence.message,
                    }
                    for evidence in diagnostic.native_evidence
                ],
            }
            for diagnostic in measurement.diagnostics
        ],
    }


def capture_measurements(
    device: _ReadableDevice,
    *,
    count: int,
    now: Callable[[], datetime] = _utc_now,
) -> list[dict[str, Any]]:
    """Capture *count* measurements with sequence and UTC timestamps."""

    if count <= 0:
        raise ValueError("count must be greater than zero")

    rows: list[dict[str, Any]] = []
    for sequence in range(1, count + 1):
        captured_at = now().astimezone(UTC).isoformat()
        try:
            measurement = device.read()
        except RtdAcquireError as exc:
            rows.append(
                {
                    "schema_version": 1,
                    "sequence": sequence,
                    "captured_at_utc": captured_at,
                    "outcome": "acquisition_error",
                    "error": {
                        "type": type(exc).__name__,
                        "message": str(exc),
                    },
                }
            )
        else:
            rows.append(
                {
                    "schema_version": 1,
                    "sequence": sequence,
                    "captured_at_utc": captured_at,
                    "outcome": "measurement",
                    "measurement": measurement_payload(measurement),
                }
            )
    return rows


def summarize_rows(rows: list[dict[str, Any]]) -> dict[str, Any]:
    """Return deterministic summary statistics for captured rows."""

    if not rows:
        raise ValueError("rows must not be empty")

    statuses: Counter[str] = Counter()
    diagnostic_codes: Counter[str] = Counter()
    error_types: Counter[str] = Counter()
    resistances: list[float] = []
    measurement_count = 0

    for row in rows:
        outcome = row.get("outcome")
        if outcome == "acquisition_error":
            error = row.get("error")
            if not isinstance(error, dict):
                raise TypeError("acquisition error payload must be an object")
            error_type = error.get("type")
            if not isinstance(error_type, str):
                raise TypeError("acquisition error type must be a string")
            error_types[error_type] += 1
            continue
        if outcome != "measurement":
            raise ValueError("capture row has an unknown outcome")

        measurement = row.get("measurement")
        if not isinstance(measurement, dict):
            raise TypeError("measurement payload must be an object")
        measurement_count += 1
        status = measurement["status"]
        if not isinstance(status, str):
            raise TypeError("measurement status must be a string")
        statuses[status] += 1

        resistance = measurement["resistance_ohms"]
        if resistance is not None:
            if not isinstance(resistance, (int, float)) or not math.isfinite(
                resistance
            ):
                raise TypeError("resistance_ohms must be finite or null")
            resistances.append(float(resistance))

        diagnostics = measurement["diagnostics"]
        if not isinstance(diagnostics, list):
            raise TypeError("diagnostics must be a list")
        for diagnostic in diagnostics:
            if not isinstance(diagnostic, dict):
                raise TypeError("diagnostic must be an object")
            code = diagnostic["code"]
            if not isinstance(code, str):
                raise TypeError("diagnostic code must be a string")
            diagnostic_codes[code] += 1

    resistance_summary: dict[str, float | int | None] = {
        "count": len(resistances),
        "mean_ohms": statistics.fmean(resistances) if resistances else None,
        "sample_standard_deviation_ohms": (
            statistics.stdev(resistances) if len(resistances) >= 2 else None
        ),
        "minimum_ohms": min(resistances) if resistances else None,
        "maximum_ohms": max(resistances) if resistances else None,
    }
    return {
        "schema_version": 1,
        "capture_count": len(rows),
        "measurement_count": measurement_count,
        "operation_error_count": len(rows) - measurement_count,
        "status_counts": dict(sorted(statuses.items())),
        "diagnostic_code_counts": dict(sorted(diagnostic_codes.items())),
        "error_type_counts": dict(sorted(error_types.items())),
        "resistance": resistance_summary,
    }


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def write_capture(
    *,
    record_dir: Path,
    label: str,
    rows: list[dict[str, Any]],
    configuration: dict[str, Any],
) -> tuple[Path, Path, Path]:
    """Write JSONL capture, summary, and immutable capture manifest."""

    label = _safe_label(label)
    if not (record_dir / "record.md").is_file():
        raise FileNotFoundError(
            "record_dir must contain record.md created by validation.create_record"
        )

    raw_path = record_dir / f"{label}.measurements.jsonl"
    summary_path = record_dir / f"{label}.summary.json"
    manifest_path = record_dir / f"{label}.capture.json"
    for path in (raw_path, summary_path, manifest_path):
        if path.exists():
            raise FileExistsError(f"capture output already exists: {path}")

    raw_text = "".join(json.dumps(row, sort_keys=True) + "\n" for row in rows)
    raw_path.write_text(raw_text, encoding="utf-8")
    summary = summarize_rows(rows)
    summary_path.write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    manifest = {
        "schema_version": 1,
        "configuration": configuration,
        "files": {
            raw_path.name: {"sha256": _sha256(raw_path)},
            summary_path.name: {"sha256": _sha256(summary_path)},
        },
    }
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    return raw_path, summary_path, manifest_path


def _parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Capture structured MAX31865 measurements on Linux spidev."
    )
    parser.add_argument("record_dir", type=Path)
    parser.add_argument("label")
    parser.add_argument("--spi-path", required=True)
    parser.add_argument("--reference-resistance-ohms", type=float, required=True)
    parser.add_argument("--wire-count", type=int, choices=(2, 3, 4), required=True)
    parser.add_argument(
        "--filter-frequency-hz",
        type=int,
        choices=(50, 60),
        required=True,
    )
    parser.add_argument("--count", type=int, default=20)
    parser.add_argument("--clock-frequency-hz", type=int, default=1_000_000)
    parser.add_argument("--low-fault-threshold-ohms", type=float)
    parser.add_argument("--high-fault-threshold-ohms", type=float)
    parser.add_argument(
        "--input-filter-time-constant-seconds",
        type=float,
        default=0.001,
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = _parse_args(argv)
    settings = SpiSettings(
        clock_polarity=0,
        clock_phase=1,
        clock_frequency_hz=args.clock_frequency_hz,
    )
    config = MAX31865Config(
        reference_resistance_ohms=args.reference_resistance_ohms,
        wire_count=args.wire_count,
        filter_frequency_hz=args.filter_frequency_hz,
        low_fault_threshold_ohms=args.low_fault_threshold_ohms,
        high_fault_threshold_ohms=args.high_fault_threshold_ohms,
    )
    timing = MAX31865Timing(
        input_filter_time_constant_seconds=args.input_filter_time_constant_seconds
    )
    configuration = {
        "device": "MAX31865",
        "requested_count": args.count,
        "spi_path": args.spi_path,
        "spi": {
            "clock_polarity": settings.clock_polarity,
            "clock_phase": settings.clock_phase,
            "clock_frequency_hz": settings.clock_frequency_hz,
            "bit_order": settings.bit_order.value,
            "bits_per_word": settings.bits_per_word,
            "chip_select_active_low": settings.chip_select_active_low,
        },
        "max31865": {
            "reference_resistance_ohms": config.reference_resistance_ohms,
            "wire_count": config.wire_count,
            "filter_frequency_hz": config.filter_frequency_hz,
            "low_fault_threshold_ohms": config.low_fault_threshold_ohms,
            "high_fault_threshold_ohms": config.high_fault_threshold_ohms,
            "input_filter_time_constant_seconds": (
                timing.input_filter_time_constant_seconds
            ),
        },
    }

    with LinuxSpidevDevice(args.spi_path, settings) as spi:
        rows = capture_measurements(
            MAX31865(spi, config, timing=timing),
            count=args.count,
        )
    paths = write_capture(
        record_dir=args.record_dir,
        label=args.label,
        rows=rows,
        configuration=configuration,
    )
    for path in paths:
        print(path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
