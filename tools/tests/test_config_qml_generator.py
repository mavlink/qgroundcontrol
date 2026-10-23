"""Tests for the vehicle config QML page generator's JSON schema validation."""

import json
import sys
from pathlib import Path

import pytest
from generators.config_qml.emit import generate_config_page_qml
from generators.config_qml.generate_pages import main as generate_config_pages
from generators.config_qml.model import ControlDef, PageDef, SectionDef, load_page_def

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


class TestQmlUnsafeStringRejection:
    """Strings embedded in generated QML literals must not contain quote/backslash/newline."""

    @pytest.mark.parametrize("bad_char", ['"', "\\", "\n"])
    def test_unsafe_section_title_rejected(self, tmp_path: Path, bad_char: str):
        data = _minimal_page()
        data["sections"][0]["title"] = f"Bad{bad_char}Title"
        with pytest.raises(ValueError, match="section title"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_non_string_repeat_title_rejected(self, tmp_path: Path):
        # Must raise the validator's ValueError, not a TypeError from the {index} check
        data = _minimal_page()
        data["sections"][0]["title"] = 42
        data["sections"][0]["repeat"] = {"paramPrefix": "BATT", "probePostfix": "_MONITOR"}
        with pytest.raises(ValueError, match="section title"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_unsafe_disabled_heading_rejected(self, tmp_path: Path):
        data = _minimal_page()
        data["sections"][0]["repeat"] = {
            "paramPrefix": "BATT",
            "probePostfix": "_MONITOR",
            "enableParam": "MONITOR",
            "disabledParamValue": "0",
            "disabledSection": {"heading": 'Disabled "Batteries"'},
        }
        with pytest.raises(ValueError, match="disabledSection heading"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_unsafe_control_label_rejected(self, tmp_path: Path):
        data = _minimal_page()
        data["sections"][0]["controls"][0]["label"] = 'A "label"'
        with pytest.raises(ValueError, match="control label"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_unsafe_keyword_rejected(self, tmp_path: Path):
        data = _minimal_page()
        data["sections"][0]["keywords"] = ["esc\\motor"]
        with pytest.raises(ValueError, match="section keyword"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_index_placeholder_in_repeat_title_rejected(self, tmp_path: Path):
        # C++ VehicleComponent::sectionIds() cannot expand {index}, so repeat
        # titles using it would generate mismatched section IDs
        data = _minimal_page()
        data["sections"][0]["title"] = "Battery {index}"
        data["sections"][0]["repeat"] = {"paramPrefix": "BATT", "probePostfix": "_MONITOR"}
        with pytest.raises(ValueError, match="index"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_safe_strings_accepted(self, tmp_path: Path):
        data = _minimal_page()
        data["sections"][0]["title"] = "ESC's & Motors (12-inch)"
        page = load_page_def(_make_page_json(tmp_path, data))
        assert page.sections[0].title == "ESC's & Motors (12-inch)"


class TestParamNameValidation:
    @pytest.mark.parametrize("bad_name", ['BAD"PARAM', "BAT[1]", "A B", "x\\y"])
    def test_bad_control_param_rejected(self, tmp_path: Path, bad_name: str):
        data = _minimal_page()
        data["sections"][0]["controls"][0]["param"] = bad_name
        with pytest.raises(ValueError, match="control param"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_bad_params_entry_rejected(self, tmp_path: Path):
        data = _minimal_page(params={"_p": 'BAD"PARAM'})
        with pytest.raises(ValueError, match="param '_p' name"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_bad_params_object_name_rejected(self, tmp_path: Path):
        data = _minimal_page(params={"_p": {"name": "BAD PARAM", "required": True}})
        with pytest.raises(ValueError, match="param '_p' name"):
            load_page_def(_make_page_json(tmp_path, data))

    @pytest.mark.parametrize("key", ["paramPrefix", "probePostfix", "enableParam"])
    def test_bad_repeat_param_fragment_rejected(self, tmp_path: Path, key: str):
        data = _minimal_page()
        data["sections"][0]["repeat"] = {
            "paramPrefix": "BATT",
            "probePostfix": "_MONITOR",
            "enableParam": "_MONITOR",
            "disabledParamValue": "0",
            key: 'X"Y',
        }
        with pytest.raises(ValueError, match=f"repeat {key}"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_bad_linked_param_rejected(self, tmp_path: Path):
        data = _minimal_page()
        data["sections"][0]["controls"][0]["linkedParams"] = {'BAD"PARAM': "value"}
        with pytest.raises(ValueError, match="linkedParams key"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_noremap_prefix_accepted(self, tmp_path: Path):
        data = _minimal_page(params={"_alt": {"name": "noremap.RTL_ALT_M", "existsOnly": True}})
        page = load_page_def(_make_page_json(tmp_path, data))
        assert 'controller.parameterExists(-1, "noremap.RTL_ALT_M")' in generate_config_page_qml(
            page
        )


class TestQmlStringEscaping:
    """Free-text fields that bypass require_qml_safe_string must still emit valid literals."""

    def test_description_quotes_and_backslashes_escaped(self, tmp_path: Path):
        data = _minimal_page()
        data["sections"][0]["controls"][0].update(
            {"control": "factslider", "description": 'Say "hi" to C:\\temp'}
        )
        qml = generate_config_page_qml(load_page_def(_make_page_json(tmp_path, data)))
        assert (
            'headingDescription: qsTranslate("Test.VehicleConfig.json", "Say \\"hi\\" to C:\\\\temp")'
            in qml
        )

    def test_unvalidated_param_is_still_escaped(self):
        # Defense in depth for definitions built without load_page_def
        page = PageDef(
            json_filename="T.json",
            sections=[SectionDef(title="S", controls=[ControlDef(param='BAD"PARAM')])],
        )
        qml = generate_config_page_qml(page)
        assert 'controller.getParameterFact(-1, "BAD\\"PARAM", false)' in qml
        assert '"BAD"PARAM"' not in qml


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


def test_cli_preserves_unchanged_output_timestamps(tmp_path: Path, monkeypatch) -> None:
    output_dir = tmp_path / "generated"
    pages_dir = tmp_path / "pages"
    pages_dir.mkdir()
    _make_page_json(pages_dir, _minimal_page())
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "generate_pages",
            "--pages-dir",
            str(pages_dir),
            "--output-dir",
            str(output_dir),
        ],
    )

    generate_config_pages()
    timestamps = {path.name: path.stat().st_mtime_ns for path in output_dir.glob("*.qml")}
    generate_config_pages()

    assert timestamps
    assert timestamps == {path.name: path.stat().st_mtime_ns for path in output_dir.glob("*.qml")}


def test_cli_list_outputs_names_each_page_without_writing(
    tmp_path: Path, monkeypatch, capsys
) -> None:
    pages_dir = tmp_path / "pages"
    pages_dir.mkdir()
    for name in ("Safety", "Power"):
        (pages_dir / f"{name}.VehicleConfig.json").write_text(
            json.dumps(_minimal_page()), encoding="utf-8"
        )
    monkeypatch.chdir(tmp_path)
    monkeypatch.setattr(
        sys, "argv", ["generate_pages", "--list-outputs", "--pages-dir", str(pages_dir)]
    )

    generate_config_pages()

    assert capsys.readouterr().out.splitlines() == ["PowerComponent.qml", "SafetyComponent.qml"]
    assert sorted(path.name for path in tmp_path.iterdir()) == ["pages"]
