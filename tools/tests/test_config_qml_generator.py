"""Tests for the vehicle config QML page generator's JSON schema validation."""

import json
from pathlib import Path

import pytest
from generators.config_qml.model import load_page_def

from ._helpers import REPO_ROOT


def _make_page_json(tmp_path: Path, page_data: dict) -> Path:
    p = tmp_path / "Test.VehicleConfig.json"
    p.write_text(json.dumps(page_data, indent=2), encoding="utf-8")
    return p


def _minimal_page(**extra) -> dict:
    data = {
        "fileType": "VehicleConfig",
        "version": 1,
        "sections": [
            {"title": "General", "controls": [{"param": "PARAM_ONE", "control": "textfield"}]},
        ],
    }
    data.update(extra)
    return data


class TestUnknownKeyRejection:
    def test_unknown_root_key_rejected(self, tmp_path: Path):
        data = _minimal_page(bogusRootKey=True)
        with pytest.raises(ValueError, match="bogusRootKey"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_unknown_section_key_rejected(self, tmp_path: Path):
        data = _minimal_page()
        data["sections"][0]["titel"] = "typo"
        with pytest.raises(ValueError, match="titel"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_unknown_control_key_rejected(self, tmp_path: Path):
        data = _minimal_page()
        data["sections"][0]["controls"][0]["sliderMinn"] = 5
        with pytest.raises(ValueError, match="sliderMinn"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_unknown_repeat_key_rejected(self, tmp_path: Path):
        data = _minimal_page()
        data["sections"][0]["repeat"] = {"paramPrefix": "BATT", "startIdx": 1}
        with pytest.raises(ValueError, match="startIdx"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_comment_keys_accepted(self, tmp_path: Path):
        data = _minimal_page(comment="root note")
        data["sections"][0]["comment"] = "section note"
        data["sections"][0]["controls"][0]["comment"] = "control note"
        page = load_page_def(_make_page_json(tmp_path, data))
        assert len(page.sections[0].controls) == 1

    def test_non_object_control_rejected(self, tmp_path: Path):
        # A string where a control object belongs must not be treated as per-character keys
        data = _minimal_page()
        data["sections"][0]["controls"] = ["PARAM_ONE"]
        with pytest.raises(ValueError, match="must be a JSON object"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_non_array_sections_rejected(self, tmp_path: Path):
        data = _minimal_page()
        data["sections"] = 42
        with pytest.raises(ValueError, match="must be a JSON array"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_non_object_params_rejected(self, tmp_path: Path):
        data = _minimal_page()
        data["params"] = ["PARAM_ONE"]
        with pytest.raises(ValueError, match="must be a JSON object"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_bad_param_entry_names_the_param(self, tmp_path: Path):
        # The error must identify WHICH param entry is wrong-shaped
        data = _minimal_page()
        data["params"] = {"batteryCount": 42}
        with pytest.raises(ValueError, match="batteryCount"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_param_entry_missing_name_rejected(self, tmp_path: Path):
        # An object param without 'name' must not raise a bare KeyError
        data = _minimal_page()
        data["params"] = {"batteryCount": {"required": True}}
        with pytest.raises(ValueError, match="batteryCount"):
            load_page_def(_make_page_json(tmp_path, data))

    @pytest.mark.parametrize(
        ("key", "bad_value"),
        [
            ("dialogButton", "oops"),
            ("actionButton", 42),
            ("toggleCheckbox", [1]),
            ("options", ["not-an-object"]),
            ("linkedParams", ["not-an-object"]),
        ],
    )
    def test_non_object_nested_field_rejected(self, tmp_path: Path, key: str, bad_value):
        # Wrong-shaped nested values must fail with a schema error, not an AttributeError
        data = _minimal_page()
        data["sections"][0]["controls"][0][key] = bad_value
        with pytest.raises(ValueError, match=key):
            load_page_def(_make_page_json(tmp_path, data))


class TestRealPageDefinitions:
    """Audit: every VehicleConfig.json in the repo must load under strict validation."""

    @pytest.mark.parametrize(
        "json_path",
        sorted((REPO_ROOT / "src").rglob("*.VehicleConfig.json")),
        ids=lambda p: p.name,
    )
    def test_real_page_loads(self, json_path: Path):
        page = load_page_def(json_path)
        assert page.sections
