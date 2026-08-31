"""Create a local rtd-acquire hardware-validation record directory."""

from __future__ import annotations

import argparse
import importlib.metadata
import json
import platform
import shutil
import subprocess
import sys
import tomllib
from datetime import UTC, datetime
from pathlib import Path
from typing import Any

_TEMPLATE_PATH = Path(__file__).with_name("v1") / "record-template.md"
_DEFAULT_ROOT = Path(".rtd-acquire-local") / "validation"


def _utc_now() -> datetime:
    return datetime.now(UTC)


def _git_commit(repository_root: Path) -> str | None:
    try:
        result = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=repository_root,
            check=True,
            capture_output=True,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError):
        return None
    value = result.stdout.strip()
    return value or None


def _package_version(repository_root: Path) -> str | None:
    try:
        return importlib.metadata.version("rtd-acquire")
    except importlib.metadata.PackageNotFoundError:
        pyproject_path = repository_root / "pyproject.toml"
        try:
            with pyproject_path.open("rb") as file:
                project = tomllib.load(file).get("project")
        except (OSError, tomllib.TOMLDecodeError):
            return None
        if not isinstance(project, dict):
            return None
        version = project.get("version")
        return version if isinstance(version, str) else None


def build_environment_metadata(
    *,
    created_at: datetime,
    repository_root: Path,
) -> dict[str, Any]:
    """Return reproducibility metadata without collecting host identity."""

    return {
        "schema_version": 1,
        "created_at_utc": created_at.astimezone(UTC).isoformat(),
        "rtd_acquire": {
            "git_commit": _git_commit(repository_root),
            "package_version": _package_version(repository_root),
        },
        "runtime": {
            "python_version": platform.python_version(),
            "python_implementation": platform.python_implementation(),
            "sys_platform": sys.platform,
            "platform_system": platform.system(),
            "platform_release": platform.release(),
            "machine": platform.machine(),
        },
    }


def create_record(
    *,
    record_id: str,
    output_root: Path,
    repository_root: Path,
    created_at: datetime | None = None,
) -> Path:
    """Create one local validation record and return its directory."""

    if not record_id or record_id in {".", ".."}:
        raise ValueError("record_id must be a non-empty directory name")
    if Path(record_id).name != record_id:
        raise ValueError("record_id must not contain path separators")

    destination = output_root / record_id
    if destination.exists():
        raise FileExistsError(f"validation record already exists: {destination}")

    destination.mkdir(parents=True)
    try:
        shutil.copyfile(_TEMPLATE_PATH, destination / "record.md")
        metadata = build_environment_metadata(
            created_at=created_at if created_at is not None else _utc_now(),
            repository_root=repository_root,
        )
        (destination / "environment.json").write_text(
            json.dumps(metadata, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
    except Exception:
        shutil.rmtree(destination)
        raise
    return destination


def _parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create a local rtd-acquire hardware-validation record."
    )
    parser.add_argument("record_id", help="local validation record directory name")
    parser.add_argument(
        "--output-root",
        type=Path,
        default=_DEFAULT_ROOT,
        help="record root (default: .rtd-acquire-local/validation)",
    )
    parser.add_argument(
        "--repository-root",
        type=Path,
        default=Path.cwd(),
        help="repository root used for commit capture (default: current directory)",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = _parse_args(argv)
    destination = create_record(
        record_id=args.record_id,
        output_root=args.output_root,
        repository_root=args.repository_root,
    )
    print(destination)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
