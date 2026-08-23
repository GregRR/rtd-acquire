"""Repository-level checks for published Python examples."""

from __future__ import annotations

from pathlib import Path

_EXAMPLES_DIR = Path(__file__).resolve().parents[1] / "examples"


def test_python_examples_compile() -> None:
    """Keep examples syntactically valid without installing optional peers."""

    paths = sorted(_EXAMPLES_DIR.glob("*.py"))
    assert paths
    for path in paths:
        compile(path.read_text(encoding="utf-8"), str(path), "exec")
