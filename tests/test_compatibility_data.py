from __future__ import annotations

import json
from pathlib import Path
from typing import cast

_COMPATIBILITY_ROOT = Path(__file__).parents[1] / "compatibility" / "v1"


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


def test_compatibility_manifest_references_existing_version_one_data() -> None:
    manifest = _load_json(_COMPATIBILITY_ROOT / "manifest.json")

    assert manifest["schema_version"] == 1

    assert set(manifest) == {
        "schema_version",
        "rtd_family_requirements",
        "evidence_model",
        "compatibility_record_sets",
    }

    referenced_paths: set[str] = set()
    for key in ("rtd_family_requirements", "evidence_model"):
        entry = _object(manifest[key])
        assert set(entry) == {"path"}
        path = entry["path"]
        assert isinstance(path, str)
        assert Path(path).name == path
        assert path.endswith(".json")
        assert (_COMPATIBILITY_ROOT / path).is_file()
        referenced_paths.add(path)

    record_sets = _list(manifest["compatibility_record_sets"])
    assert record_sets == []

    actual_paths = {
        path.name
        for path in _COMPATIBILITY_ROOT.glob("*.json")
        if path.name != "manifest.json"
    }
    assert actual_paths == referenced_paths


def test_rtd_family_requirements_are_frozen() -> None:
    document = _load_json(_COMPATIBILITY_ROOT / "rtd_families.json")

    assert document["schema_version"] == 1
    assert _object(document["source"]) == {
        "project": "rtd-sensor",
        "repository_ref": "v0.8.0",
        "version": "0.8.0",
        "contract_status": "stable",
        "contract_version": 1,
        "model_catalog": {
            "path": "conformance/v1/models.json",
            "sha256": (
                "7b24f9c5538d090c75e925677347e3b317d4fc6d74d1bb6c9aa885ddc368b3d3"
            ),
        },
        "characteristic_catalog": {
            "path": "conformance/v1/characteristics.json",
            "sha256": (
                "30b2474c511bb057d8440882378d8056035ee8da77dd1417901b9fcb5ca2919b"
            ),
        },
    }

    families = _list(document["families"])
    observed: dict[str, tuple[float, float, float, float, float, str]] = {}
    characteristic_ids: dict[str, str] = {}

    for family_value in families:
        family = _object(family_value)
        family_id = family["model_id"]
        assert isinstance(family_id, str)
        assert family_id not in observed

        assert isinstance(family["display_name"], str)
        characteristic_id = family["characteristic_id"]
        assert isinstance(characteristic_id, str)
        characteristic_ids[family_id] = characteristic_id

        nominal = family["nominal_resistance_ohms"]
        assert isinstance(nominal, (int, float))
        assert nominal > 0

        temperature_range = _object(family["temperature_range_c"])
        minimum_temperature = temperature_range["minimum"]
        maximum_temperature = temperature_range["maximum"]
        assert isinstance(minimum_temperature, (int, float))
        assert isinstance(maximum_temperature, (int, float))
        assert minimum_temperature < maximum_temperature

        envelope = _object(family["required_resistance_envelope_ohms"])
        minimum_resistance = envelope["minimum"]
        maximum_resistance = envelope["maximum"]
        bounds_kind = envelope["bounds_kind"]
        assert isinstance(minimum_resistance, (int, float))
        assert isinstance(maximum_resistance, (int, float))
        assert 0 < minimum_resistance < maximum_resistance
        assert bounds_kind in {"exact", "rounded"}

        observed[family_id] = (
            float(nominal),
            float(minimum_temperature),
            float(maximum_temperature),
            float(minimum_resistance),
            float(maximum_resistance),
            bounds_kind,
        )

    assert characteristic_ids == {
        "pt100": "iec60751_pt385",
        "pt500": "iec60751_pt385",
        "pt1000": "iec60751_pt385",
        "ni120": "ni6720_north_american",
        "ni1000": "ni6180_din43760",
        "ni1000_tk5000": "ni5000_tk5000",
    }

    assert observed == {
        "pt100": (100.0, -200.0, 850.0, 18.52008, 390.481125, "exact"),
        "pt500": (500.0, -200.0, 850.0, 92.6004, 1952.405625, "exact"),
        "pt1000": (1000.0, -200.0, 850.0, 185.2008, 3904.81125, "exact"),
        "ni120": (120.0, -80.0, 260.0, 66.6, 380.3099, "rounded"),
        "ni1000": (
            1000.0,
            -60.0,
            250.0,
            695.202595,
            2891.5625,
            "exact",
        ),
        "ni1000_tk5000": (
            1000.0,
            -60.0,
            250.0,
            751.79284,
            2517.265625,
            "exact",
        ),
    }


def test_compatibility_data_does_not_embed_temperature_model_coefficients() -> None:
    document = _load_json(_COMPATIBILITY_ROOT / "rtd_families.json")

    allowed_family_fields = {
        "model_id",
        "display_name",
        "characteristic_id",
        "nominal_resistance_ohms",
        "temperature_range_c",
        "required_resistance_envelope_ohms",
    }
    for family_value in _list(document["families"]):
        family = _object(family_value)
        assert set(family) == allowed_family_fields


def test_evidence_model_keeps_claim_dimensions_independent() -> None:
    document = _load_json(_COMPATIBILITY_ROOT / "evidence_model.json")

    assert document["schema_version"] == 1

    dimensions: dict[str, dict[str, object]] = {}
    for value in _list(document["claim_dimensions"]):
        dimension = _object(value)
        dimension_id = dimension["id"]
        assert isinstance(dimension_id, str)
        assert dimension_id not in dimensions
        dimensions[dimension_id] = dimension

    assert set(dimensions) == {
        "manufacturer_support",
        "electrical_compatibility",
        "project_validation",
    }

    expected_states = {
        "manufacturer_support": [
            "documented_supported",
            "documented_unsupported",
            "not_established",
        ],
        "electrical_compatibility": [
            "compatible",
            "incompatible",
            "not_assessed",
        ],
        "project_validation": [
            "validated",
            "not_validated",
        ],
    }
    for dimension_id, state_ids in expected_states.items():
        observed_states: list[str] = []
        for value in _list(dimensions[dimension_id]["states"]):
            state = _object(value)
            assert set(state) == {"id", "meaning"}
            state_id = state["id"]
            assert isinstance(state_id, str)
            assert isinstance(state["meaning"], str)
            assert state["meaning"]
            observed_states.append(state_id)
        assert observed_states == state_ids

    depths: set[str] = set()
    for value in _list(document["validation_depths"]):
        depth = _object(value)
        assert set(depth) == {"id", "label", "meaning"}
        depth_id = depth["id"]
        assert isinstance(depth_id, str)
        assert isinstance(depth["label"], str)
        assert isinstance(depth["meaning"], str)
        assert depth["meaning"]
        assert depth_id not in depths
        depths.add(depth_id)

    assert depths == {"range_validated", "family_hardware_validated"}
