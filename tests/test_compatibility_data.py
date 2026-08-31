from __future__ import annotations

import json
import math
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

    def add_referenced_path(entry_value: object) -> None:
        entry = _object(entry_value)
        assert set(entry) == {"path"}
        path = entry["path"]
        assert isinstance(path, str)
        assert Path(path).name == path
        assert path.endswith(".json")
        assert (_COMPATIBILITY_ROOT / path).is_file()
        assert path not in referenced_paths
        referenced_paths.add(path)

    for key in ("rtd_family_requirements", "evidence_model"):
        add_referenced_path(manifest[key])

    for record_set in _list(manifest["compatibility_record_sets"]):
        add_referenced_path(record_set)

    actual_paths = {
        path.name
        for path in _COMPATIBILITY_ROOT.glob("*.json")
        if path.name != "manifest.json"
    }
    assert actual_paths == referenced_paths


def test_rtd_sensor_source_pin_matches_independently_verified_v0_8_0() -> None:
    document = _load_json(_COMPATIBILITY_ROOT / "rtd_families.json")

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


def test_rtd_family_requirements_are_frozen() -> None:
    document = _load_json(_COMPATIBILITY_ROOT / "rtd_families.json")

    assert document["schema_version"] == 1

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


def test_max31865_record_set_covers_current_families_once() -> None:
    family_document = _load_json(_COMPATIBILITY_ROOT / "rtd_families.json")
    record_document = _load_json(_COMPATIBILITY_ROOT / "max31865.json")

    assert record_document["schema_version"] == 1
    assert set(record_document) == {
        "schema_version",
        "record_set_id",
        "device",
        "family_requirements_path",
        "evidence_model_path",
        "device_limits",
        "sources",
        "records",
    }
    assert record_document["record_set_id"] == "max31865"
    assert record_document["family_requirements_path"] == "rtd_families.json"
    assert record_document["evidence_model_path"] == "evidence_model.json"
    assert _object(record_document["device"]) == {
        "manufacturer": "Analog Devices",
        "model": "MAX31865",
        "interface": "SPI",
    }

    family_ids = {
        cast(str, _object(value)["model_id"])
        for value in _list(family_document["families"])
    }
    record_family_ids = [
        cast(str, _object(_object(value)["rtd_family"])["model_id"])
        for value in _list(record_document["records"])
    ]

    assert len(record_family_ids) == len(set(record_family_ids))
    assert set(record_family_ids) == family_ids


def test_compatibility_records_use_frozen_evidence_states() -> None:
    evidence_document = _load_json(_COMPATIBILITY_ROOT / "evidence_model.json")
    manifest = _load_json(_COMPATIBILITY_ROOT / "manifest.json")

    allowed_states: dict[str, set[str]] = {}
    for dimension_value in _list(evidence_document["claim_dimensions"]):
        dimension = _object(dimension_value)
        dimension_id = dimension["id"]
        assert isinstance(dimension_id, str)
        allowed_states[dimension_id] = {
            cast(str, _object(state)["id"]) for state in _list(dimension["states"])
        }

    allowed_depths = {
        cast(str, _object(depth)["id"])
        for depth in _list(evidence_document["validation_depths"])
    }

    for record_set_value in _list(manifest["compatibility_record_sets"]):
        record_set_path = _object(record_set_value)["path"]
        assert isinstance(record_set_path, str)
        record_document = _load_json(_COMPATIBILITY_ROOT / record_set_path)
        assert set(record_document) == {
            "schema_version",
            "record_set_id",
            "device",
            "family_requirements_path",
            "evidence_model_path",
            "device_limits",
            "sources",
            "records",
        }
        assert record_document["schema_version"] == 1
        assert record_document["family_requirements_path"] == "rtd_families.json"
        assert record_document["evidence_model_path"] == "evidence_model.json"
        source_ids = {
            cast(str, _object(source)["id"])
            for source in _list(record_document["sources"])
        }

        record_ids: set[str] = set()
        for record_value in _list(record_document["records"]):
            record = _object(record_value)
            assert set(record) == {
                "id",
                "rtd_family",
                "configuration",
                "claims",
                "validation_depths",
                "limitations",
            }
            record_id = record["id"]
            assert isinstance(record_id, str)
            assert record_id not in record_ids
            record_ids.add(record_id)
            limitations = _list(record["limitations"])
            assert limitations
            assert all(isinstance(limitation, str) for limitation in limitations)

            claims = _object(record["claims"])
            assert set(claims) == set(allowed_states)

            for dimension_id, states in allowed_states.items():
                claim = _object(claims[dimension_id])
                assert set(claim) == {"state", "evidence_source_ids", "rationale"}
                state = claim["state"]
                assert isinstance(state, str)
                assert state in states
                rationale = claim["rationale"]
                assert isinstance(rationale, str)
                assert rationale
                for source_id in _list(claim["evidence_source_ids"]):
                    assert isinstance(source_id, str)
                    assert source_id in source_ids

            validation_depths = _list(record["validation_depths"])
            assert all(isinstance(depth, str) for depth in validation_depths)
            assert set(cast(list[str], validation_depths)) <= allowed_depths

            project_state = _object(claims["project_validation"])["state"]
            if project_state == "not_validated":
                assert validation_depths == []
            else:
                assert project_state == "validated"
                assert validation_depths


def test_max31865_electrical_compatibility_covers_full_envelopes() -> None:
    family_document = _load_json(_COMPATIBILITY_ROOT / "rtd_families.json")
    record_document = _load_json(_COMPATIBILITY_ROOT / "max31865.json")

    families = {
        cast(str, _object(value)["model_id"]): _object(value)
        for value in _list(family_document["families"])
    }
    limits = _object(record_document["device_limits"])
    reference_limits = _object(limits["reference_resistance_ohms"])
    minimum_reference = reference_limits["minimum"]
    maximum_reference = reference_limits["maximum"]
    adc_scale = limits["adc_scale_codes"]
    highest_threshold_code = limits["highest_threshold_code"]
    assert isinstance(minimum_reference, (int, float))
    assert isinstance(maximum_reference, (int, float))
    assert isinstance(adc_scale, int)
    assert isinstance(highest_threshold_code, int)
    assert adc_scale == 1 << 15
    assert highest_threshold_code == adc_scale - 1

    bias_voltage = _object(limits["bias_voltage_v"])
    bias_current = _object(limits["bias_output_current_ma"])
    cable_resistance = _object(limits["cable_resistance_ohms_per_lead"])
    minimum_bias_voltage = bias_voltage["minimum"]
    maximum_bias_voltage = bias_voltage["maximum"]
    minimum_bias_current = bias_current["minimum"]
    maximum_bias_current = bias_current["maximum"]
    maximum_cable_resistance = cable_resistance["maximum"]
    assert isinstance(minimum_bias_voltage, (int, float))
    assert isinstance(maximum_bias_voltage, (int, float))
    assert isinstance(minimum_bias_current, (int, float))
    assert isinstance(maximum_bias_current, (int, float))
    assert isinstance(maximum_cable_resistance, (int, float))

    expected_reference_resistances = {
        "pt100": 430.0,
        "pt500": 2000.0,
        "pt1000": 4300.0,
        "ni120": 430.0,
        "ni1000": 4300.0,
        "ni1000_tk5000": 4300.0,
    }

    for record_value in _list(record_document["records"]):
        record = _object(record_value)
        family_ref = _object(record["rtd_family"])
        model_id = family_ref["model_id"]
        characteristic_id = family_ref["characteristic_id"]
        assert isinstance(model_id, str)
        assert isinstance(characteristic_id, str)

        family = families[model_id]
        assert family["characteristic_id"] == characteristic_id
        envelope = _object(family["required_resistance_envelope_ohms"])
        minimum_resistance = envelope["minimum"]
        maximum_resistance = envelope["maximum"]
        assert isinstance(minimum_resistance, (int, float))
        assert isinstance(maximum_resistance, (int, float))

        configuration = _object(record["configuration"])
        assert set(configuration) == {"reference_resistance_ohms", "wire_count"}
        reference_resistance = configuration["reference_resistance_ohms"]
        assert isinstance(reference_resistance, (int, float))
        assert float(reference_resistance) == expected_reference_resistances[model_id]
        assert configuration["wire_count"] == 4
        assert minimum_reference <= reference_resistance <= maximum_reference
        assert 0 < minimum_resistance < maximum_resistance < reference_resistance

        high_threshold_code = math.ceil(
            maximum_resistance / reference_resistance * adc_scale
        )
        assert high_threshold_code <= highest_threshold_code

        maximum_current_ma = (
            maximum_bias_voltage / (reference_resistance + minimum_resistance) * 1000.0
        )
        minimum_current_ma = (
            minimum_bias_voltage
            / (
                reference_resistance
                + maximum_resistance
                + 2.0 * maximum_cable_resistance
            )
            * 1000.0
        )
        assert minimum_current_ma >= minimum_bias_current
        assert maximum_current_ma <= maximum_bias_current

        electrical_claim = _object(
            _object(record["claims"])["electrical_compatibility"]
        )
        assert electrical_claim["state"] == "compatible"


def test_max31865_manufacturer_support_is_scoped_to_documented_platinum() -> None:
    record_document = _load_json(_COMPATIBILITY_ROOT / "max31865.json")

    observed: dict[str, str] = {}
    for record_value in _list(record_document["records"]):
        record = _object(record_value)
        model_id = _object(record["rtd_family"])["model_id"]
        state = _object(_object(record["claims"])["manufacturer_support"])["state"]
        assert isinstance(model_id, str)
        assert isinstance(state, str)
        observed[model_id] = state

    assert observed == {
        "pt100": "documented_supported",
        "pt500": "documented_supported",
        "pt1000": "documented_supported",
        "ni120": "not_established",
        "ni1000": "not_established",
        "ni1000_tk5000": "not_established",
    }


def test_max31865_records_do_not_claim_physical_validation() -> None:
    record_document = _load_json(_COMPATIBILITY_ROOT / "max31865.json")

    for record_value in _list(record_document["records"]):
        record = _object(record_value)
        project_claim = _object(_object(record["claims"])["project_validation"])
        assert project_claim["state"] == "not_validated"
        assert _list(project_claim["evidence_source_ids"]) == []
        assert _list(record["validation_depths"]) == []
