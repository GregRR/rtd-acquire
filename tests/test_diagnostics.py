from __future__ import annotations

import pytest

from rtd_acquire.core import (
    Diagnostic,
    DiagnosticCode,
    DiagnosticSeverity,
    NativeEvidence,
    diagnostic_message,
)


def test_every_diagnostic_code_has_a_canonical_message() -> None:
    for code in DiagnosticCode:
        assert diagnostic_message(code)


def test_diagnostic_message_is_derived_from_code() -> None:
    diagnostic = Diagnostic(
        code=DiagnosticCode.REFERENCE_LOW,
        severity=DiagnosticSeverity.FAULT,
    )

    assert diagnostic.message == (
        "The acquisition reference is below a monitored threshold."
    )


def test_native_evidence_can_preserve_composite_states() -> None:
    diagnostic = Diagnostic(
        code=DiagnosticCode.SENSOR_CIRCUIT_OPEN,
        severity=DiagnosticSeverity.FAULT,
        native_evidence=(
            NativeEvidence(identifier="0x60n0:02", message="Overrange"),
            NativeEvidence(identifier="0x60n0:07", message="Error"),
        ),
    )

    assert len(diagnostic.native_evidence) == 2
    assert diagnostic.native_evidence[0].message == "Overrange"
    assert diagnostic.native_evidence[1].message == "Error"


def test_native_evidence_accepts_identifier_only() -> None:
    evidence = NativeEvidence(identifier="D5")

    assert evidence.identifier == "D5"
    assert evidence.message is None


def test_native_evidence_accepts_message_only() -> None:
    evidence = NativeEvidence(message="Sensor Open")

    assert evidence.identifier is None
    assert evidence.message == "Sensor Open"


@pytest.mark.parametrize(
    ("identifier", "message"),
    [
        (None, None),
        ("", None),
        ("   ", None),
        (None, ""),
        (None, "   "),
        (" ", " "),
    ],
)
def test_native_evidence_rejects_empty_entries(
    identifier: str | None,
    message: str | None,
) -> None:
    with pytest.raises(ValueError, match="non-empty identifier or message"):
        NativeEvidence(identifier=identifier, message=message)
