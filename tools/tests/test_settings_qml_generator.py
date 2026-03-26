"""Tests for the settings QML page generator."""

import json
from pathlib import Path

import pytest

from generators.settings_qml.page_generator import (
    ControlDef,
    GroupDef,
    PageDef,
    generate_page_qml,
    generate_pages_model_qml,
    load_page_def,
)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _make_settings_dir(tmp_path: Path, facts: dict[str, list[dict]]) -> Path:
    """Create a temporary Settings directory with SettingsGroup.json files.

    Args:
        facts: mapping of stem (e.g. "App") to list of fact dicts.
    """
    settings_dir = tmp_path / "Settings"
    settings_dir.mkdir()
    for stem, fact_list in facts.items():
        data = {
            "version": 1,
            "fileType": "FactMetaData",
            "QGC.MetaData.Facts": fact_list,
        }
        (settings_dir / f"{stem}.SettingsGroup.json").write_text(
            json.dumps(data), encoding="utf-8"
        )
    return settings_dir


def _make_page_json(tmp_path: Path, page_data: dict) -> Path:
    """Write a page UI definition JSON to a temp file."""
    p = tmp_path / "Test.SettingsUI.json"
    p.write_text(json.dumps(page_data, indent=2), encoding="utf-8")
    return p


# ---------------------------------------------------------------------------
# Tests: load_page_def
# ---------------------------------------------------------------------------

class TestLoadPageDef:
    def test_loads_groups(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [
                {"heading": "General", "controls": [{"setting": "appSettings.x"}]},
                {"heading": "Advanced", "controls": [{"setting": "appSettings.y"}]},
            ],
        }
        page = load_page_def(_make_page_json(tmp_path, data))
        assert len(page.groups) == 2
        assert page.groups[0].heading == "General"
        assert page.groups[1].heading == "Advanced"

    def test_loads_controls(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [
                {
                    "heading": "G",
                    "controls": [
                        {"setting": "appSettings.x", "label": "My Label", "control": "combobox"},
                        {"setting": "appSettings.y"},
                    ],
                }
            ],
        }
        page = load_page_def(_make_page_json(tmp_path, data))
        assert len(page.groups[0].controls) == 2
        assert page.groups[0].controls[0].label == "My Label"
        assert page.groups[0].controls[0].control == "combobox"
        assert page.groups[0].controls[1].setting == "appSettings.y"

    def test_loads_bindings(self, tmp_path: Path):
        data = {
            "version": 1,
            "bindings": {"_mgr": "QGroundControl.settingsManager"},
            "groups": [],
        }
        page = load_page_def(_make_page_json(tmp_path, data))
        assert page.bindings == {"_mgr": "QGroundControl.settingsManager"}

    def test_loads_component_group(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [
                {"component": "MyCustomComponent", "sectionName": "Custom", "keywords": "a,b"},
            ],
        }
        page = load_page_def(_make_page_json(tmp_path, data))
        assert page.groups[0].component == "MyCustomComponent"
        assert page.groups[0].sectionName == "Custom"
        assert page.groups[0].keywords == "a,b"

    def test_loads_showWhen_enableWhen(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [
                {
                    "heading": "G",
                    "showWhen": "someCondition",
                    "enableWhen": "anotherCondition",
                    "controls": [
                        {"setting": "appSettings.x", "showWhen": "ctrl_cond", "enableWhen": "ctrl_en"},
                    ],
                }
            ],
        }
        page = load_page_def(_make_page_json(tmp_path, data))
        assert page.groups[0].showWhen == "someCondition"
        assert page.groups[0].enableWhen == "anotherCondition"
        assert page.groups[0].controls[0].showWhen == "ctrl_cond"
        assert page.groups[0].controls[0].enableWhen == "ctrl_en"

    def test_empty_groups(self, tmp_path: Path):
        data = {"version": 1, "groups": []}
        page = load_page_def(_make_page_json(tmp_path, data))
        assert page.groups == []


# ---------------------------------------------------------------------------
# Tests: ControlDef properties
# ---------------------------------------------------------------------------

class TestControlDef:
    def test_settings_group(self):
        ctrl = ControlDef(setting="appSettings.myFact")
        assert ctrl.settings_group == "appSettings"

    def test_fact_name(self):
        ctrl = ControlDef(setting="appSettings.myFact")
        assert ctrl.fact_name == "myFact"

    def test_nested_fact_name(self):
        ctrl = ControlDef(setting="appSettings.nested.fact")
        assert ctrl.fact_name == "nested.fact"


# ---------------------------------------------------------------------------
# Tests: GroupDef.display_name
# ---------------------------------------------------------------------------

class TestGroupDef:
    def test_display_name_from_heading(self):
        grp = GroupDef(heading="My Heading")
        assert grp.display_name == "My Heading"

    def test_display_name_from_section_name(self):
        grp = GroupDef(heading="Ignored", sectionName="Override")
        assert grp.display_name == "Override"


# ---------------------------------------------------------------------------
# Tests: generate_page_qml
# ---------------------------------------------------------------------------

class TestGeneratePageQml:
    @pytest.fixture
    def settings_dir(self, tmp_path: Path) -> Path:
        return _make_settings_dir(tmp_path, {
            "App": [
                {"name": "enableFeature", "type": "bool", "shortDesc": "Enable", "label": "Enable Feature"},
                {"name": "maxAlt", "type": "double", "shortDesc": "Max alt", "label": "Maximum Altitude"},
                {"name": "colorScheme", "type": "uint32", "shortDesc": "Color",
                 "enumStrings": "Light,Dark", "enumValues": "0,1", "label": "Color Scheme"},
                {"name": "savePath", "type": "string", "shortDesc": "Save path", "label": "Save Path"},
            ],
        })

    def test_has_imports(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(heading="G", controls=[ControlDef(setting="appSettings.enableFeature")]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "import QtQuick" in qml
        assert "import QGroundControl.FactControls" in qml
        assert "import QGroundControl.Controls" in qml

    def test_root_element(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(heading="G", controls=[ControlDef(setting="appSettings.enableFeature")]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "SettingsPage {" in qml
        assert qml.rstrip().endswith("}")

    def test_bool_generates_checkbox(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(controls=[ControlDef(setting="appSettings.enableFeature")]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "FactCheckBoxSlider {" in qml
        assert "QGroundControl.settingsManager.appSettings.enableFeature" in qml

    def test_enum_generates_combobox(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(controls=[ControlDef(setting="appSettings.colorScheme")]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "LabelledFactComboBox {" in qml
        assert "indexModel: false" in qml

    def test_numeric_generates_textfield(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(controls=[ControlDef(setting="appSettings.maxAlt")]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "LabelledFactTextField {" in qml

    def test_explicit_control_override(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(controls=[ControlDef(setting="appSettings.maxAlt", control="combobox")]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "LabelledFactComboBox {" in qml

    def test_heading(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(heading="My Section", controls=[ControlDef(setting="appSettings.enableFeature")]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert 'heading: qsTr("My Section")' in qml

    def test_no_heading_when_empty(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(controls=[ControlDef(setting="appSettings.enableFeature")]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "heading:" not in qml

    def test_component_group(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(component="MyCustomWidget"),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "MyCustomWidget {" in qml
        assert "Layout.fillWidth: true" in qml

    def test_showWhen_on_group(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(heading="G", showWhen="someFlag", controls=[
                ControlDef(setting="appSettings.enableFeature"),
            ]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "(someFlag)" in qml

    def test_enableWhen_on_group(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(heading="G", enableWhen="otherFlag", controls=[
                ControlDef(setting="appSettings.enableFeature"),
            ]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "enabled: otherFlag" in qml

    def test_showWhen_on_control(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(controls=[
                ControlDef(setting="appSettings.enableFeature", showWhen="x === 1"),
            ]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "(x === 1)" in qml
        assert "fact.userVisible" in qml

    def test_enableWhen_on_control(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(controls=[
                ControlDef(setting="appSettings.enableFeature", enableWhen="enabled_expr"),
            ]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "enabled: enabled_expr" in qml

    def test_explicit_label(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(controls=[
                ControlDef(setting="appSettings.maxAlt", label="Custom Label"),
            ]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert 'qsTr("Custom Label")' in qml

    def test_browse_control(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(controls=[ControlDef(setting="appSettings.savePath", control="browse")]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "LabelledFactBrowse {" in qml

    def test_slider_control(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(controls=[ControlDef(setting="appSettings.maxAlt", control="slider")]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "FactTextFieldSlider {" in qml

    def test_scaler_control(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(controls=[ControlDef(setting="appSettings.maxAlt", control="scaler")]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "LabelledFactIncrementer {" in qml

    def test_bindings_emitted(self, settings_dir: Path):
        page = PageDef(
            bindings={"_mgr": "QGroundControl.settingsManager"},
            groups=[GroupDef(controls=[ControlDef(setting="appSettings.enableFeature")])],
        )
        qml = generate_page_qml(page, settings_dir)
        assert "property var _mgr: QGroundControl.settingsManager" in qml

    def test_string_field_width_added(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(controls=[ControlDef(setting="appSettings.savePath")]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "_stringFieldWidth" in qml

    def test_layout_fill_width(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(heading="G", controls=[
                ControlDef(setting="appSettings.enableFeature"),
                ControlDef(setting="appSettings.maxAlt"),
            ]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert qml.count("Layout.fillWidth: true") >= 3  # group + 2 controls

    def test_section_filter_visibility(self, settings_dir: Path):
        page = PageDef(groups=[
            GroupDef(heading="A", controls=[ControlDef(setting="appSettings.enableFeature")]),
            GroupDef(heading="B", controls=[ControlDef(setting="appSettings.maxAlt")]),
        ])
        qml = generate_page_qml(page, settings_dir)
        assert "sectionFilter === 0" in qml
        assert "sectionFilter === 1" in qml


# ---------------------------------------------------------------------------
# Tests: generate_pages_model_qml
# ---------------------------------------------------------------------------

class TestGeneratePagesModelQml:
    @pytest.fixture
    def pages_setup(self, tmp_path: Path) -> Path:
        """Create a SettingsPages.json and minimal page definitions."""
        pages_dir = tmp_path / "pages"
        pages_dir.mkdir()

        # Page definition
        page_def = {
            "version": 1,
            "groups": [
                {"heading": "Section A", "controls": [{"setting": "appSettings.x"}]},
                {"heading": "Section B", "controls": [{"setting": "appSettings.y"}]},
            ],
        }
        (pages_dir / "Test.SettingsUI.json").write_text(
            json.dumps(page_def), encoding="utf-8"
        )

        # Settings metadata
        settings_dir = pages_dir.parent.parent.parent / "Settings"
        settings_dir.mkdir(parents=True, exist_ok=True)
        meta = {
            "version": 1,
            "fileType": "FactMetaData",
            "QGC.MetaData.Facts": [
                {"name": "x", "type": "bool", "shortDesc": "X", "label": "X"},
                {"name": "y", "type": "bool", "shortDesc": "Y", "label": "Y"},
            ],
        }
        (settings_dir / "App.SettingsGroup.json").write_text(
            json.dumps(meta), encoding="utf-8"
        )

        # Pages JSON
        pages_json = {
            "version": 1,
            "pages": [
                {
                    "name": "Test Page",
                    "qml": "TestPage.qml",
                    "icon": "qrc:/test.svg",
                    "pageDefinition": "Test.SettingsUI.json",
                },
                {"divider": True},
            ],
        }
        pages_path = pages_dir / "SettingsPages.json"
        pages_path.write_text(json.dumps(pages_json), encoding="utf-8")
        return pages_path

    def test_generates_list_model(self, pages_setup: Path):
        qml = generate_pages_model_qml(pages_setup)
        assert "ListModel {" in qml
        assert qml.rstrip().endswith("}")

    def test_page_entry(self, pages_setup: Path):
        qml = generate_pages_model_qml(pages_setup)
        assert 'name: qsTr("Test Page")' in qml
        assert "qrc:/qml/QGroundControl/AppSettings/TestPage.qml" in qml
        assert "qrc:/test.svg" in qml

    def test_divider_entry(self, pages_setup: Path):
        qml = generate_pages_model_qml(pages_setup)
        assert '"Divider"' in qml

    def test_sections_extracted(self, pages_setup: Path):
        qml = generate_pages_model_qml(pages_setup)
        assert "Section A" in qml
        assert "Section B" in qml

    def test_search_terms_present(self, pages_setup: Path):
        qml = generate_pages_model_qml(pages_setup)
        assert "searchTerms" in qml

    def test_page_visible_default(self, pages_setup: Path):
        qml = generate_pages_model_qml(pages_setup)
        assert "return true" in qml

    def test_page_visible_expression(self, tmp_path: Path):
        pages_dir = tmp_path / "pages2"
        pages_dir.mkdir()
        pages_json = {
            "version": 1,
            "pages": [
                {
                    "name": "Cond",
                    "qml": "Cond.qml",
                    "icon": "qrc:/c.svg",
                    "visible": "QGroundControl.someFlag",
                },
            ],
        }
        pages_path = pages_dir / "SettingsPages.json"
        pages_path.write_text(json.dumps(pages_json), encoding="utf-8")
        qml = generate_pages_model_qml(pages_path)
        assert "QGroundControl.someFlag" in qml


# ---------------------------------------------------------------------------
# Tests: Real page definitions (integration)
# ---------------------------------------------------------------------------

class TestRealPageDefinitions:
    """Test against real QGC page definition files if available."""

    @pytest.fixture
    def repo_root(self) -> Path:
        root = Path(__file__).resolve().parent.parent.parent
        if (root / "src" / "Settings").is_dir():
            return root
        pytest.skip("Not running from QGC repo root")

    def test_all_page_defs_load(self, repo_root: Path):
        pages_dir = repo_root / "src" / "UI" / "AppSettings" / "pages"
        json_files = list(pages_dir.glob("*.SettingsUI.json"))
        assert len(json_files) > 0, "No page definition files found"
        for json_file in json_files:
            page = load_page_def(json_file)
            assert isinstance(page, PageDef), f"Failed to load {json_file.name}"

    def test_all_page_defs_generate(self, repo_root: Path):
        pages_dir = repo_root / "src" / "UI" / "AppSettings" / "pages"
        settings_dir = repo_root / "src" / "Settings"
        json_files = list(pages_dir.glob("*.SettingsUI.json"))
        for json_file in json_files:
            page = load_page_def(json_file)
            qml = generate_page_qml(page, settings_dir)
            assert "SettingsPage {" in qml, f"Generation failed for {json_file.name}"

    def test_viewer3d_page(self, repo_root: Path):
        pages_dir = repo_root / "src" / "UI" / "AppSettings" / "pages"
        settings_dir = repo_root / "src" / "Settings"
        page = load_page_def(pages_dir / "Viewer3D.SettingsUI.json")
        qml = generate_page_qml(page, settings_dir)
        assert 'heading: qsTr("General")' in qml
        assert 'heading: qsTr("Data")' in qml
        assert "viewer3DSettings.enabled" in qml

    def test_pages_model_generates(self, repo_root: Path):
        pages_path = repo_root / "src" / "UI" / "AppSettings" / "pages" / "SettingsPages.json"
        qml = generate_pages_model_qml(pages_path)
        assert "ListModel {" in qml
        assert "General" in qml
