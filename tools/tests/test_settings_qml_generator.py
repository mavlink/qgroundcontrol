"""Tests for the settings QML page generator."""

import json
import re
import sys
from pathlib import Path

import pytest
from generators.common.validation import require_qml_safe_string
from generators.settings_qml import generate_pages as settings_generator
from generators.settings_qml.page_generator import (
    ControlDef,
    GroupDef,
    PageDef,
    generate_page_qml,
    generate_pages_model_qml,
    get_fact_type,
    load_page_def,
    load_settings_metadata,
)

from ._helpers import REPO_ROOT


def _make_settings_dir(tmp_path: Path, facts: dict[str, list[dict]]) -> Path:
    """Create a Settings dir with one SettingsGroup.json per stem in `facts`."""
    settings_dir = tmp_path / "Settings"
    settings_dir.mkdir()
    for stem, fact_list in facts.items():
        data = {
            "version": 1,
            "fileType": "FactMetaData",
            "QGC.MetaData.Facts": fact_list,
        }
        (settings_dir / f"{stem}.SettingsGroup.json").write_text(json.dumps(data), encoding="utf-8")
    return settings_dir


def _make_page_json(tmp_path: Path, page_data: dict) -> Path:
    """Write a page UI definition JSON to a temp file."""
    p = tmp_path / "Test.SettingsUI.json"
    p.write_text(json.dumps(page_data, indent=2), encoding="utf-8")
    return p


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

    def test_unknown_root_key_rejected(self, tmp_path: Path):
        data = {
            "version": 1,
            "bogusRootKey": True,
            "groups": [{"heading": "G", "controls": [{"setting": "appSettings.x"}]}],
        }
        with pytest.raises(ValueError, match="bogusRootKey"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_non_object_root_rejected(self, tmp_path: Path):
        # A JSON array root must produce a clear shape error, not a confusing traceback
        p = tmp_path / "Test.SettingsUI.json"
        p.write_text(json.dumps([{"heading": "G"}]), encoding="utf-8")
        with pytest.raises(ValueError, match="must be a JSON object"):
            load_page_def(p)

    def test_non_object_control_rejected(self, tmp_path: Path):
        # A string where a control object belongs must not be treated as per-character keys
        data = {
            "version": 1,
            "groups": [{"heading": "G", "controls": ["appSettings.x"]}],
        }
        with pytest.raises(ValueError, match="must be a JSON object"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_non_object_error_repr_truncated(self):
        # A huge offending value must not balloon the error message
        from generators.common.validation import reject_unknown_keys

        with pytest.raises(ValueError) as excinfo:
            reject_unknown_keys(["x" * 50] * 100, frozenset(), "control", "Test.json")
        assert len(str(excinfo.value)) < 400
        assert str(excinfo.value).count("...") >= 1

    def test_non_array_groups_rejected(self, tmp_path: Path):
        # groups: 42 must give a clear shape error, not a bare TypeError traceback
        data = {"version": 1, "groups": 42}
        with pytest.raises(ValueError, match="must be a JSON array"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_string_controls_rejected(self, tmp_path: Path):
        # controls: "x" must not be iterated per-character
        data = {"version": 1, "groups": [{"heading": "G", "controls": "appSettings.x"}]}
        with pytest.raises(ValueError, match="must be a JSON array"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_non_object_bindings_rejected(self, tmp_path: Path):
        # bindings: [] would only blow up later in emit with an AttributeError
        data = {"version": 1, "bindings": [1, 2], "groups": []}
        with pytest.raises(ValueError, match="must be a JSON object"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_missing_setting_rejected(self, tmp_path: Path):
        # A fact-backed control without a setting would crash emit with a bare IndexError
        data = {"version": 1, "groups": [{"heading": "G", "controls": [{"label": "Oops"}]}]}
        with pytest.raises(ValueError, match=r"settingsGroupAccessor\.factName"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_dotless_setting_rejected(self, tmp_path: Path):
        # Forgetting the settings group prefix must be a clear authoring error
        data = {
            "version": 1,
            "groups": [{"heading": "G", "controls": [{"setting": "operatorIDEU"}]}],
        }
        with pytest.raises(ValueError, match="operatorIDEU"):
            load_page_def(_make_page_json(tmp_path, data))

    @pytest.mark.parametrize(
        "bad_setting", ["appSettings..x", "appSettings.x.", ".x", "appSettings.höhe"]
    )
    def test_malformed_setting_segments_rejected(self, tmp_path: Path, bad_setting: str):
        # Empty path segments or non-ASCII would emit broken fact refs / objectNames
        data = {
            "version": 1,
            "groups": [{"heading": "G", "controls": [{"setting": bad_setting}]}],
        }
        with pytest.raises(ValueError, match=r"settingsGroupAccessor\.factName"):
            load_page_def(_make_page_json(tmp_path, data))

    @pytest.mark.parametrize("key", ["enableCheckbox", "button"])
    def test_non_object_nested_field_rejected(self, tmp_path: Path, key: str):
        # A truthy non-object (e.g. a string) must not reach .get() with an AttributeError
        data = {
            "version": 1,
            "groups": [{"heading": "G", "controls": [{"setting": "appSettings.x", key: "oops"}]}],
        }
        with pytest.raises(ValueError, match=key):
            load_page_def(_make_page_json(tmp_path, data))

    @pytest.mark.parametrize("bad_value", [[], "", 0, False])
    def test_falsy_non_object_nested_field_rejected(self, tmp_path: Path, bad_value):
        # Falsy wrong-shaped values must not be silently treated as "absent"
        data = {
            "version": 1,
            "groups": [
                {
                    "heading": "G",
                    "controls": [{"setting": "appSettings.x", "enableCheckbox": bad_value}],
                }
            ],
        }
        with pytest.raises(ValueError, match="enableCheckbox"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_unknown_group_key_rejected(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [
                {"heading": "G", "showWen": "typo", "controls": [{"setting": "appSettings.x"}]}
            ],
        }
        with pytest.raises(ValueError, match="showWen"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_unknown_control_key_rejected(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [
                {"heading": "G", "controls": [{"setting": "appSettings.x", "enabelWhen": "typo"}]}
            ],
        }
        with pytest.raises(ValueError, match="enabelWhen"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_comment_keys_accepted(self, tmp_path: Path):
        data = {
            "version": 1,
            "comment": "root note",
            "groups": [
                {
                    "heading": "G",
                    "comment": "group note",
                    "controls": [{"setting": "appSettings.x", "comment": "control note"}],
                }
            ],
        }
        page = load_page_def(_make_page_json(tmp_path, data))
        assert len(page.groups[0].controls) == 1

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
        assert page.groups[0].keywords == ["a", "b"]

    def test_loads_showWhen_enableWhen(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [
                {
                    "heading": "G",
                    "showWhen": "someCondition",
                    "enableWhen": "anotherCondition",
                    "controls": [
                        {
                            "setting": "appSettings.x",
                            "showWhen": "ctrl_cond",
                            "enableWhen": "ctrl_en",
                        },
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


class TestGroupDef:
    def test_display_name_from_heading(self):
        grp = GroupDef(heading="My Heading")
        assert grp.display_name == "My Heading"

    def test_display_name_from_section_name(self):
        grp = GroupDef(heading="Ignored", sectionName="Override")
        assert grp.display_name == "Override"


class TestGeneratePageQml:
    @pytest.fixture
    def settings_dir(self, tmp_path: Path) -> Path:
        return _make_settings_dir(
            tmp_path,
            {
                "App": [
                    {
                        "name": "enableFeature",
                        "type": "bool",
                        "shortDesc": "Enable",
                        "label": "Enable Feature",
                    },
                    {
                        "name": "maxAlt",
                        "type": "double",
                        "shortDesc": "Max alt",
                        "label": "Maximum Altitude",
                    },
                    {
                        "name": "colorScheme",
                        "type": "uint32",
                        "shortDesc": "Color",
                        "enumStrings": "Light,Dark",
                        "enumValues": "0,1",
                        "label": "Color Scheme",
                    },
                    {
                        "name": "savePath",
                        "type": "string",
                        "shortDesc": "Save path",
                        "label": "Save Path",
                    },
                ],
            },
        )

    def test_has_imports(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(heading="G", controls=[ControlDef(setting="appSettings.enableFeature")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "import QtQuick" in qml
        assert "import QGroundControl.FactControls" in qml
        assert "import QGroundControl.Controls" in qml

    def test_root_element(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(heading="G", controls=[ControlDef(setting="appSettings.enableFeature")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "SettingsPage {" in qml
        assert qml.rstrip().endswith("}")

    def test_page_name_emits_object_name(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(heading="G", controls=[ControlDef(setting="appSettings.enableFeature")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir, page_name="Fly View")
        assert 'objectName: "settingsPage_FlyView"' in qml

    def test_page_name_empty_no_object_name(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(heading="G", controls=[ControlDef(setting="appSettings.enableFeature")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir, page_name="")
        assert 'objectName: "settingsPage_' not in qml

    def test_bool_generates_checkbox(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(controls=[ControlDef(setting="appSettings.enableFeature")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "FactCheckBoxSlider {" in qml
        assert "QGroundControl.settingsManager.appSettings.enableFeature" in qml

    def test_enum_generates_combobox(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(controls=[ControlDef(setting="appSettings.colorScheme")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "LabelledFactComboBox {" in qml
        assert "indexModel: false" in qml

    def test_numeric_generates_textfield(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(controls=[ControlDef(setting="appSettings.maxAlt")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "LabelledFactTextField {" in qml

    def test_explicit_control_override(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(controls=[ControlDef(setting="appSettings.maxAlt", control="combobox")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "LabelledFactComboBox {" in qml

    def test_textfield_has_object_name(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(controls=[ControlDef(setting="appSettings.savePath")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert 'objectName: "settingsTextField_savePath"' in qml

    def test_group_has_object_name(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    heading="EU Vehicle Info", controls=[ControlDef(setting="appSettings.savePath")]
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert 'objectName: "settingsGroup_EUVehicleInfo"' in qml

    def test_group_object_name_sanitized(self, settings_dir: Path):
        # Quotes, backslashes and other non-identifier characters in a heading must not
        # be able to break out of (or corrupt) the generated QML string literal
        page = PageDef(
            groups=[
                GroupDef(
                    heading='Say "Hi\\" & <Bye>!',
                    controls=[ControlDef(setting="appSettings.savePath")],
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert 'objectName: "settingsGroup_SayHiBye"' in qml

    def test_duplicate_group_object_name_rejected(self, settings_dir: Path):
        # The sanitizer is lossy: distinct headings can collapse to the same objectName,
        # which would make UI test lookups silently match the wrong group. Fail loudly.
        page = PageDef(
            groups=[
                GroupDef(
                    heading="EU Vehicle Info", controls=[ControlDef(setting="appSettings.savePath")]
                ),
                GroupDef(
                    heading="EU-Vehicle Info",
                    controls=[ControlDef(setting="appSettings.enableFeature")],
                ),
            ]
        )
        with pytest.raises(ValueError, match="settingsGroup_EUVehicleInfo"):
            generate_page_qml(page, settings_dir)

    def test_page_name_sanitizing_to_empty_rejected(self, settings_dir: Path):
        # A page name with no identifier characters would silently drop the page's
        # objectName, breaking UI test lookups. Fail loudly, same as headings.
        page = PageDef(
            groups=[
                GroupDef(heading="G", controls=[ControlDef(setting="appSettings.savePath")]),
            ]
        )
        with pytest.raises(ValueError, match="sanitizes to an empty objectName"):
            generate_page_qml(page, settings_dir, page_name="中文!")

    def test_heading_sanitizing_to_empty_rejected(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(heading="***", controls=[ControlDef(setting="appSettings.savePath")]),
            ]
        )
        with pytest.raises(ValueError, match=r"\*\*\*"):
            generate_page_qml(page, settings_dir)

    def test_page_object_name_sanitized(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(heading="G", controls=[ControlDef(setting="appSettings.enableFeature")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir, page_name='Fly "View"')
        assert 'objectName: "settingsPage_FlyView"' in qml

    def test_checkbox_has_object_name(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(controls=[ControlDef(setting="appSettings.enableFeature")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert 'objectName: "settingsCheckBox_enableFeature"' in qml

    def test_no_error_when_no_validation_ui(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(controls=[ControlDef(setting="appSettings.savePath")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "externalError" not in qml

    def test_heading(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    heading="My Section", controls=[ControlDef(setting="appSettings.enableFeature")]
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert 'heading: qsTr("My Section")' in qml

    def test_no_heading_when_empty(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(controls=[ControlDef(setting="appSettings.enableFeature")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "heading:" not in qml

    def test_component_group(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(component="MyCustomWidget"),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "MyCustomWidget {" in qml
        assert "Layout.fillWidth: true" in qml
        # Component should be wrapped in a ColumnLayout so it doesn't
        # override the component's own visible: binding.
        assert "ColumnLayout {" in qml
        assert "spacing: 0" in qml

    def test_component_group_with_showWhen(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(component="MyCustomWidget", showWhen="someFlag"),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "ColumnLayout {" in qml
        assert "(someFlag)" in qml
        assert "MyCustomWidget {" in qml

    def test_component_control(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    heading="G",
                    controls=[
                        ControlDef(setting="appSettings.enableFeature"),
                        ControlDef(setting="", control="component", component="MyInlineWidget"),
                    ],
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "MyInlineWidget {" in qml
        assert "Layout.fillWidth: true" in qml
        # Should NOT be wrapped in a ColumnLayout (it's inside SettingsGroupLayout)
        lines = [line.strip() for line in qml.splitlines()]
        idx = lines.index("MyInlineWidget {")
        assert "ColumnLayout {" not in lines[idx - 1]

    def test_component_control_with_showWhen(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    heading="G",
                    controls=[
                        ControlDef(
                            setting="",
                            control="component",
                            component="MyWidget",
                            showWhen="featureEnabled",
                        ),
                    ],
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "MyWidget {" in qml
        assert "visible: featureEnabled" in qml

    def test_component_control_with_enableWhen(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    heading="G",
                    controls=[
                        ControlDef(
                            setting="",
                            control="component",
                            component="MyWidget",
                            enableWhen="isReady",
                        ),
                    ],
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "MyWidget {" in qml
        assert "enabled: isReady" in qml

    def test_showWhen_on_group(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    heading="G",
                    showWhen="someFlag",
                    controls=[
                        ControlDef(setting="appSettings.enableFeature"),
                    ],
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "(someFlag)" in qml

    def test_enableWhen_on_group(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    heading="G",
                    enableWhen="otherFlag",
                    controls=[
                        ControlDef(setting="appSettings.enableFeature"),
                    ],
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "enabled: otherFlag" in qml

    def test_showWhen_on_control(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    controls=[
                        ControlDef(setting="appSettings.enableFeature", showWhen="x === 1"),
                    ]
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "(x === 1)" in qml
        assert "appSettings.enableFeature.userVisible" in qml

    def test_enableWhen_on_control(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    controls=[
                        ControlDef(setting="appSettings.enableFeature", enableWhen="enabled_expr"),
                    ]
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "enabled: enabled_expr" in qml

    def test_explicit_label(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    controls=[
                        ControlDef(setting="appSettings.maxAlt", label="Custom Label"),
                    ]
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert 'qsTr("Custom Label")' in qml

    def test_browse_control(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(controls=[ControlDef(setting="appSettings.savePath", control="browse")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "LabelledFactBrowse {" in qml

    def test_browse_control_properties(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    controls=[
                        ControlDef(
                            setting="appSettings.savePath",
                            control="browse",
                            properties={
                                "selectFolder": "false",
                                "nameFilters": '[ qsTr("OSM (*.osm)") ]',
                            },
                        )
                    ]
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "selectFolder: false" in qml
        assert 'nameFilters: [ qsTr("OSM (*.osm)") ]' in qml

    def test_properties_rejected_on_non_browse_control(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [
                {
                    "heading": "G",
                    "controls": [
                        {
                            "setting": "appSettings.x",
                            "control": "textfield",
                            "properties": {"a": "b"},
                        }
                    ],
                }
            ],
        }
        with pytest.raises(ValueError, match="properties"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_properties_bad_name_rejected(self, tmp_path: Path):
        # Property names are emitted verbatim into QML; invalid ones must fail at load time
        data = {
            "version": 1,
            "groups": [
                {
                    "heading": "G",
                    "controls": [
                        {
                            "setting": "appSettings.x",
                            "control": "browse",
                            "properties": {"name filters": "[]"},
                        }
                    ],
                }
            ],
        }
        with pytest.raises(ValueError, match="valid QML property name"):
            load_page_def(_make_page_json(tmp_path, data))

    @pytest.mark.parametrize("reserved", ["id", "objectName", "label", "fact", "enabled"])
    def test_properties_reserved_name_rejected(self, tmp_path: Path, reserved: str):
        # These are already emitted by the control template; a JSON override would
        # generate a duplicate binding that only fails later at qmllint/build time
        data = {
            "version": 1,
            "groups": [
                {
                    "heading": "G",
                    "controls": [
                        {
                            "setting": "appSettings.x",
                            "control": "browse",
                            "properties": {reserved: "oops"},
                        }
                    ],
                }
            ],
        }
        with pytest.raises(ValueError, match="reserved"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_properties_primitive_values_coerced(self, tmp_path: Path):
        # JSON booleans/numbers must become QML literals, not Python reprs (True/False)
        data = {
            "version": 1,
            "groups": [
                {
                    "heading": "G",
                    "controls": [
                        {
                            "setting": "appSettings.x",
                            "control": "browse",
                            "properties": {"selectFolder": False, "maxCount": 42},
                        }
                    ],
                }
            ],
        }
        page = load_page_def(_make_page_json(tmp_path, data))
        props = page.groups[0].controls[0].properties
        assert props["selectFolder"] == "false"
        assert props["maxCount"] == "42"

    def test_properties_non_primitive_value_rejected(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [
                {
                    "heading": "G",
                    "controls": [
                        {
                            "setting": "appSettings.x",
                            "control": "browse",
                            "properties": {"nameFilters": ["*.osm"]},
                        }
                    ],
                }
            ],
        }
        with pytest.raises(ValueError, match="string, boolean, or number"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_properties_end_to_end_json_to_qml(self, tmp_path: Path, settings_dir: Path):
        # Full pipeline: JSON schema -> load_page_def (coercion) -> generate_page_qml.
        # Guards the seam between loader and emitter that the unit tests above bypass.
        data = {
            "version": 1,
            "groups": [
                {
                    "heading": "G",
                    "controls": [
                        {
                            "setting": "appSettings.savePath",
                            "control": "browse",
                            "properties": {
                                "selectFolder": False,
                                "nameFilters": '[ qsTr("OSM (*.osm)") ]',
                            },
                        }
                    ],
                }
            ],
        }
        page = load_page_def(_make_page_json(tmp_path, data))
        qml = generate_page_qml(page, settings_dir)
        assert "LabelledFactBrowse {" in qml
        assert "selectFolder: false" in qml
        assert 'nameFilters: [ qsTr("OSM (*.osm)") ]' in qml

    def test_slider_control(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(controls=[ControlDef(setting="appSettings.maxAlt", control="slider")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "FactTextFieldSlider {" in qml

    def test_scaler_control(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(controls=[ControlDef(setting="appSettings.maxAlt", control="scaler")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "LabelledFactIncrementer {" in qml

    def test_info_control(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    controls=[
                        ControlDef(
                            setting="",
                            control="info",
                            label="Log files are saved to",
                            value="logSavePath",
                            showWhen="diskLoggingEnabledValue",
                        )
                    ]
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "LabelledLabel {" in qml
        assert 'label: qsTr("Log files are saved to")' in qml
        assert "labelText: logSavePath" in qml
        assert "visible: diskLoggingEnabledValue" in qml

    def test_info_control_no_show_when(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    controls=[
                        ControlDef(
                            setting="",
                            control="info",
                            label="Some info",
                            value="someBinding",
                        )
                    ]
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "LabelledLabel {" in qml
        assert "visible:" not in qml.split("LabelledLabel")[1].split("}")[0]

    def test_info_control_with_button(self, settings_dir: Path):
        from generators.common.controls import ButtonDef
        from generators.settings_qml.page_generator import ControlDef as CD

        page = PageDef(
            groups=[
                GroupDef(
                    controls=[
                        CD(
                            setting="",
                            control="info",
                            label="Bytes sent",
                            value="sink.bytesSentDisplay",
                            showWhen="sink && sink.enabled",
                            button=ButtonDef(text="Reset", onClicked="sink.resetBytesSent()"),
                        )
                    ]
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "RowLayout {" in qml
        assert "LabelledLabel {" in qml
        assert 'label: qsTr("Bytes sent")' in qml
        assert "labelText: sink.bytesSentDisplay" in qml
        assert "QGCButton {" in qml
        assert 'text: qsTr("Reset")' in qml
        assert "onClicked: sink.resetBytesSent()" in qml
        assert "visible: sink && sink.enabled" in qml

    def test_info_control_with_enable_when(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    controls=[
                        ControlDef(
                            setting="",
                            control="info",
                            label="Info",
                            value="someValue",
                            enableWhen="someCondition",
                        )
                    ]
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "LabelledLabel {" in qml
        assert "enabled: someCondition" in qml

    def test_bindings_emitted(self, settings_dir: Path):
        page = PageDef(
            bindings={"_mgr": "QGroundControl.settingsManager"},
            groups=[GroupDef(controls=[ControlDef(setting="appSettings.enableFeature")])],
        )
        qml = generate_page_qml(page, settings_dir)
        assert "property var _mgr: QGroundControl.settingsManager" in qml

    def test_string_field_width_added(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(controls=[ControlDef(setting="appSettings.savePath")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "_stringFieldWidth" in qml

    def test_layout_fill_width(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    heading="G",
                    controls=[
                        ControlDef(setting="appSettings.enableFeature"),
                        ControlDef(setting="appSettings.maxAlt"),
                    ],
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert qml.count("Layout.fillWidth: true") >= 3  # group + 2 controls

    def test_section_filter_visibility(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(heading="A", controls=[ControlDef(setting="appSettings.enableFeature")]),
                GroupDef(heading="B", controls=[ControlDef(setting="appSettings.maxAlt")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "sectionFilter === 0" in qml
        assert "sectionFilter === 1" in qml


class TestGeneratePagesModelQml:
    @pytest.fixture
    def pages_setup(self, tmp_path: Path) -> Path:
        """Create a SettingsPages.json and minimal page definitions."""
        pages_dir = tmp_path / "pages"
        pages_dir.mkdir()

        # Page definition
        page_def = {
            "version": 1,
            "imports": ["Test.Settings"],
            "bindings": {
                "pageVisible": "advancedMode && !unusedBinding",
                "advancedMode": "QGroundControl.corePlugin.showAdvancedUI",
                "unusedBinding": "QGroundControl.settingsManager.appSettings.y.userVisible",
            },
            "groups": [
                {
                    "heading": "Section A",
                    "showWhen": "pageVisible",
                    "controls": [{"setting": "appSettings.x"}],
                },
                {"heading": "Section B", "controls": [{"setting": "appSettings.y"}]},
                {
                    "component": "TestComponent",
                    "sectionName": "Section C",
                    "showWhen": "advancedMode",
                },
            ],
        }
        (pages_dir / "Test.SettingsUI.json").write_text(json.dumps(page_def), encoding="utf-8")

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
        (settings_dir / "App.SettingsGroup.json").write_text(json.dumps(meta), encoding="utf-8")

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
        assert 'name: qsTranslate("SettingsPages.json", "Test Page")' in qml
        assert 'nameKey: "Test Page"' in qml
        assert "qrc:/qml/QGroundControl/AppSettings/TestPage.qml" in qml
        assert "qrc:/test.svg" in qml

    def test_divider_entry(self, pages_setup: Path):
        qml = generate_pages_model_qml(pages_setup)
        assert '"Divider"' in qml

    def test_sections_extracted(self, pages_setup: Path):
        qml = generate_pages_model_qml(pages_setup)
        assert 'name: qsTranslate("Test.SettingsUI.json", "Section A")' in qml
        assert 'name: qsTranslate("Test.SettingsUI.json", "Section B")' in qml

    def test_section_visibility_generated(self, pages_setup: Path):
        qml = generate_pages_model_qml(pages_setup)
        assert "sections: function()" in qml
        assert "property QtObject _page0SectionState: QtObject" in qml
        assert "property var advancedMode: QGroundControl.corePlugin.showAdvancedUI" in qml
        assert "property bool pageVisible: advancedMode && !unusedBinding" in qml
        assert "property var unusedBinding: QGroundControl.settingsManager.appSettings.y.userVisible" in qml
        assert "visible: _page0SectionState._sectionVisibility[0]" in qml
        assert (
            "(pageVisible) && "
            "(QGroundControl.settingsManager.appSettings.x.userVisible)" in qml
        )
        assert "(QGroundControl.settingsManager.appSettings.y.userVisible)" in qml
        assert "visible: _page0SectionState._sectionVisibility[2]" in qml

    def test_search_terms_present(self, pages_setup: Path):
        qml = generate_pages_model_qml(pages_setup)
        assert 'searchTerms: ["test page section a"' in qml
        assert 'qsTranslate("Test.SettingsUI.json", "Section A")' in qml

    def test_page_imports_propagated(self, pages_setup: Path):
        qml = generate_pages_model_qml(pages_setup)
        assert "import Test.Settings" in qml

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

    def test_unknown_root_key_rejected(self, tmp_path: Path):
        pages_path = tmp_path / "SettingsPages.json"
        pages_path.write_text(
            json.dumps(
                {
                    "version": 1,
                    "bogusRootKey": True,
                    "pages": [{"name": "P", "qml": "P.qml", "icon": "qrc:/p.svg"}],
                }
            ),
            encoding="utf-8",
        )
        with pytest.raises(ValueError, match="bogusRootKey"):
            generate_pages_model_qml(pages_path)

    def test_unknown_page_entry_key_rejected(self, tmp_path: Path):
        pages_path = tmp_path / "SettingsPages.json"
        pages_path.write_text(
            json.dumps(
                {
                    "version": 1,
                    "pages": [
                        {"name": "P", "qml": "P.qml", "icon": "qrc:/p.svg", "vissible": "typo"}
                    ],
                }
            ),
            encoding="utf-8",
        )
        with pytest.raises(ValueError, match="vissible"):
            generate_pages_model_qml(pages_path)

    def test_comment_keys_accepted(self, tmp_path: Path):
        pages_path = tmp_path / "SettingsPages.json"
        pages_path.write_text(
            json.dumps(
                {
                    "version": 1,
                    "comment": "root note",
                    "pages": [
                        {"name": "P", "qml": "P.qml", "icon": "qrc:/p.svg", "comment": "entry note"}
                    ],
                }
            ),
            encoding="utf-8",
        )
        qml = generate_pages_model_qml(pages_path)
        assert 'nameKey: "P"' in qml

    def test_non_array_pages_rejected(self, tmp_path: Path):
        # pages: "oops" must not be iterated per-character
        pages_path = tmp_path / "SettingsPages.json"
        pages_path.write_text(json.dumps({"version": 1, "pages": "oops"}), encoding="utf-8")
        with pytest.raises(ValueError, match="must be a JSON array"):
            generate_pages_model_qml(pages_path)


class TestQmlUnsafeStringRejection:
    """Strings embedded in generated QML literals must not contain quote/backslash/newline."""

    @pytest.mark.parametrize("bad_value", [["safe"], 123, None, {"a": 1}])
    def test_non_string_rejected(self, bad_value: object):
        with pytest.raises(ValueError, match="must be a string"):
            require_qml_safe_string(bad_value, "test field", "test.json")

    def test_unsafe_section_name_rejected(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [
                {
                    "heading": "G",
                    "sectionName": 'Bad "Section"',
                    "controls": [{"setting": "appSettings.x"}],
                }
            ],
        }
        with pytest.raises(ValueError, match="group sectionName"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_unsafe_group_keyword_rejected(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [
                {
                    "heading": "G",
                    "keywords": ["map\\layers"],
                    "controls": [{"setting": "appSettings.x"}],
                }
            ],
        }
        with pytest.raises(ValueError, match="group keyword"):
            load_page_def(_make_page_json(tmp_path, data))

    @pytest.mark.parametrize("bad_char", ['"', "\\", "\n"])
    def test_unsafe_group_heading_rejected(self, tmp_path: Path, bad_char: str):
        data = {
            "version": 1,
            "groups": [{"heading": f"Bad{bad_char}Heading", "controls": [{"setting": "appSettings.x"}]}],
        }
        with pytest.raises(ValueError, match="group heading"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_heading_description_expression_accepted(self, tmp_path: Path):
        # headingDescription is a QML expression field, emitted raw — quotes allowed
        data = {
            "version": 1,
            "groups": [
                {
                    "heading": "G",
                    "headingDescription": 'qsTr("Has \\"quotes\\"")',
                    "controls": [{"setting": "appSettings.x"}],
                }
            ],
        }
        page = load_page_def(_make_page_json(tmp_path, data))
        assert page.groups[0].headingDescription

    def test_unsafe_control_label_rejected(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [
                {"heading": "G", "controls": [{"setting": "appSettings.x", "label": 'A "label"'}]}
            ],
        }
        with pytest.raises(ValueError, match="control label"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_unsafe_placeholder_rejected(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [
                {
                    "heading": "G",
                    "controls": [{"setting": "appSettings.x", "placeholder": "a\\b"}],
                }
            ],
        }
        with pytest.raises(ValueError, match="placeholder"):
            load_page_def(_make_page_json(tmp_path, data))

    @pytest.mark.parametrize("field", ["name", "url", "icon"])
    def test_unsafe_pages_model_entry_rejected(self, tmp_path: Path, field: str):
        entry = {"name": "Page", "qml": "P.qml", "icon": "qrc:/i.svg"}
        entry[field] = f'bad"{field}'
        pages_json = {"version": 1, "pages": [entry]}
        p = tmp_path / "SettingsPages.json"
        p.write_text(json.dumps(pages_json), encoding="utf-8")
        with pytest.raises(ValueError, match=f"page {field}"):
            generate_pages_model_qml(p)

    def test_safe_strings_accepted(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [
                {
                    "heading": "Fly View (What's Shown)",
                    "controls": [{"setting": "appSettings.x", "label": "UI Scale (%)"}],
                }
            ],
        }
        page = load_page_def(_make_page_json(tmp_path, data))
        assert page.groups[0].heading == "Fly View (What's Shown)"

class TestRealPageDefinitions:
    """Test against real QGC page definition files if available."""

    @pytest.fixture
    def repo_root(self) -> Path:
        if (REPO_ROOT / "src" / "Settings").is_dir():
            return REPO_ROOT
        pytest.skip("Not running from QGC repo root")

    def test_all_page_defs_load(self, repo_root: Path):
        pages_dir = repo_root / "src" / "AppSettings" / "pages"
        json_files = list(pages_dir.glob("*.SettingsUI.json"))
        assert len(json_files) > 0, "No page definition files found"
        for json_file in json_files:
            page = load_page_def(json_file)
            assert isinstance(page, PageDef), f"Failed to load {json_file.name}"

    def test_all_page_defs_generate(self, repo_root: Path):
        pages_dir = repo_root / "src" / "AppSettings" / "pages"
        settings_dir = repo_root / "src" / "Settings"
        json_files = list(pages_dir.glob("*.SettingsUI.json"))
        for json_file in json_files:
            page = load_page_def(json_file)
            qml = generate_page_qml(page, settings_dir)
            assert "SettingsPage {" in qml, f"Generation failed for {json_file.name}"

    def test_viewer3d_page(self, repo_root: Path):
        pages_dir = repo_root / "src" / "AppSettings" / "pages"
        settings_dir = repo_root / "src" / "Settings"
        page = load_page_def(pages_dir / "Viewer3D.SettingsUI.json")
        qml = generate_page_qml(page, settings_dir)
        assert 'heading: qsTr("General")' in qml
        assert 'heading: qsTr("Data")' in qml
        assert "viewer3DSettings.enabled" in qml
        # osmFilePath browse control: properties from the JSON must survive to QML
        assert "LabelledFactBrowse {" in qml
        assert "selectFolder: false" in qml
        assert "nameFilters:" in qml

    def test_pages_model_generates(self, repo_root: Path):
        pages_path = repo_root / "src" / "AppSettings" / "pages" / "SettingsPages.json"
        qml = generate_pages_model_qml(pages_path)
        assert "ListModel {" in qml
        assert "General" in qml


def test_cli_preserves_unchanged_output_timestamps(tmp_path: Path, monkeypatch) -> None:
    output_dir = tmp_path / "generated"
    pages_dir = tmp_path / "pages"
    pages_dir.mkdir()
    page_definition = {
        "version": 1,
        "groups": [{"heading": "General", "controls": [{"setting": "appSettings.x"}]}],
    }
    _make_page_json(pages_dir, page_definition)
    (pages_dir / "SettingsPages.json").write_text(
        json.dumps(
            {
                "version": 1,
                "pages": [
                    {
                        "name": "Test Page",
                        "qml": "TestPage.qml",
                        "icon": "qrc:/test.svg",
                        "pageDefinition": "Test.SettingsUI.json",
                    }
                ],
            }
        ),
        encoding="utf-8",
    )
    settings_dir = _make_settings_dir(
        tmp_path,
        {"App": [{"name": "x", "type": "bool", "shortDesc": "X", "label": "X"}]},
    )
    monkeypatch.setattr(settings_generator, "PAGES_DIR", pages_dir)
    monkeypatch.setattr(settings_generator, "SETTINGS_DIR", settings_dir)
    monkeypatch.setattr(
        sys,
        "argv",
        ["generate_pages", "--output-dir", str(output_dir)],
    )

    assert settings_generator.main() == 0
    timestamps = {path.name: path.stat().st_mtime_ns for path in output_dir.glob("*.qml")}
    assert settings_generator.main() == 0

    assert timestamps
    assert timestamps == {path.name: path.stat().st_mtime_ns for path in output_dir.glob("*.qml")}


class TestCustomOverlay:
    """Custom-build overlay merging into the stock pages model."""

    @pytest.fixture
    def stock_setup(self, tmp_path: Path) -> tuple[Path, Path]:
        """Stock pages dir with Alpha/Beta pages plus an empty custom overlay dir."""
        pages_dir = tmp_path / "pages"
        pages_dir.mkdir()
        page_def = {
            "version": 1,
            "groups": [{"heading": "Stock Section", "controls": [{"setting": "appSettings.x"}]}],
        }
        (pages_dir / "Alpha.SettingsUI.json").write_text(json.dumps(page_def), encoding="utf-8")
        (pages_dir / "Beta.SettingsUI.json").write_text(json.dumps(page_def), encoding="utf-8")
        pages_json = {
            "version": 1,
            "pages": [
                {
                    "name": "Alpha",
                    "qml": "Alpha.qml",
                    "icon": "qrc:/alpha.svg",
                    "pageDefinition": "Alpha.SettingsUI.json",
                },
                {"divider": True},
                {
                    "name": "Beta",
                    "qml": "Beta.qml",
                    "icon": "qrc:/beta.svg",
                    "pageDefinition": "Beta.SettingsUI.json",
                },
            ],
        }
        pages_path = pages_dir / "SettingsPages.json"
        pages_path.write_text(json.dumps(pages_json), encoding="utf-8")
        custom_dir = tmp_path / "custom_pages"
        custom_dir.mkdir()
        return pages_path, custom_dir

    def _write_overlay(self, custom_dir: Path, pages: list[dict]) -> None:
        overlay = {"version": 1, "pages": pages}
        (custom_dir / "SettingsPages.json").write_text(json.dumps(overlay), encoding="utf-8")

    def _gamma_entry(self, custom_dir: Path, **extra) -> dict:
        page_def = {
            "version": 1,
            "groups": [{"heading": "Custom Section", "controls": [{"setting": "appSettings.x"}]}],
        }
        (custom_dir / "Gamma.SettingsUI.json").write_text(json.dumps(page_def), encoding="utf-8")
        return {
            "name": "Gamma",
            "qml": "Gamma.qml",
            "icon": "qrc:/gamma.svg",
            "pageDefinition": "Gamma.SettingsUI.json",
            **extra,
        }

    def test_no_overlay_file_keeps_stock_pages(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        qml = generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)
        assert 'nameKey: "Alpha"' in qml
        assert 'nameKey: "Beta"' in qml

    def test_append_page(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir)])
        qml = generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)
        assert 'nameKey: "Gamma"' in qml
        assert 'qsTranslate("Gamma.SettingsUI.json", "Custom Section")' in qml
        assert qml.index('nameKey: "Beta"') < qml.index('nameKey: "Gamma"')

    def test_insert_after(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir, insertAfter="Alpha")])
        qml = generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)
        assert qml.index('nameKey: "Alpha"') < qml.index('nameKey: "Gamma"')
        assert qml.index('nameKey: "Gamma"') < qml.index('nameKey: "Beta"')

    def test_insert_before(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir, insertBefore="Alpha")])
        qml = generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)
        assert qml.index('nameKey: "Gamma"') < qml.index('nameKey: "Alpha"')

    def test_multiple_insert_after_same_anchor_preserves_order(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        gamma = self._gamma_entry(custom_dir, insertAfter="Alpha")
        delta = {
            "name": "Delta",
            "qml": "Delta.qml",
            "icon": "qrc:/delta.svg",
            "pageDefinition": "Gamma.SettingsUI.json",
            "insertAfter": "Alpha",
        }
        self._write_overlay(custom_dir, [gamma, delta])
        qml = generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)
        assert (
            qml.index('nameKey: "Alpha"')
            < qml.index('nameKey: "Gamma"')
            < qml.index('nameKey: "Delta"')
            < qml.index('nameKey: "Beta"')
        )

    def test_non_string_qml_rejected(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir, qml=0)])
        with pytest.raises(ValueError, match="'qml'"):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_non_string_page_definition_rejected(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir, pageDefinition=[])])
        with pytest.raises(ValueError, match="'pageDefinition'"):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_backslash_qml_path_rejected(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir, qml="sub\\Gamma.qml")])
        with pytest.raises(ValueError, match="bare file name"):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_remove_page(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [{"remove": "Beta"}])
        qml = generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)
        assert 'nameKey: "Alpha"' in qml
        assert 'nameKey: "Beta"' not in qml

    def test_replace_page_keeps_position(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        page_def = {
            "version": 1,
            "groups": [{"heading": "Replaced Section", "controls": [{"setting": "appSettings.x"}]}],
        }
        (custom_dir / "AlphaCustom.SettingsUI.json").write_text(json.dumps(page_def), encoding="utf-8")
        self._write_overlay(custom_dir, [{
            "name": "Alpha",
            "qml": "Alpha.qml",
            "icon": "qrc:/alpha-custom.svg",
            "pageDefinition": "AlphaCustom.SettingsUI.json",
        }])
        qml = generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)
        assert qml.count('nameKey: "Alpha"') == 1
        assert "qrc:/alpha-custom.svg" in qml
        assert "Replaced Section" in qml
        assert qml.index('nameKey: "Alpha"') < qml.index('nameKey: "Beta"')

    def test_custom_page_def_shadows_stock(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        page_def = {
            "version": 1,
            "groups": [{"heading": "Shadowed Section", "controls": [{"setting": "appSettings.x"}]}],
        }
        (custom_dir / "Alpha.SettingsUI.json").write_text(json.dumps(page_def), encoding="utf-8")
        qml = generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)
        assert "Shadowed Section" in qml
        assert "Stock Section" not in qml.split('nameKey: "Beta"')[0]

    def test_remove_unknown_page_rejected(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [{"remove": "Nonexistent"}])
        with pytest.raises(ValueError, match="Nonexistent"):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_insert_after_unknown_rejected(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir, insertAfter="Nonexistent")])
        with pytest.raises(ValueError, match="Nonexistent"):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_both_position_keys_rejected(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(
            custom_dir, [self._gamma_entry(custom_dir, insertAfter="Alpha", insertBefore="Beta")]
        )
        with pytest.raises(ValueError, match="insertAfter"):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_replace_with_position_key_rejected(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [{
            "name": "Alpha",
            "qml": "Alpha.qml",
            "icon": "qrc:/alpha.svg",
            "pageDefinition": "Alpha.SettingsUI.json",
            "insertAfter": "Beta",
        }])
        with pytest.raises(ValueError, match="replace"):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_remove_with_extra_keys_rejected(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [{"remove": "Beta", "icon": "qrc:/x.svg"}])
        with pytest.raises(ValueError, match="remove"):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_missing_name_rejected(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        entry = self._gamma_entry(custom_dir)
        del entry["name"]
        self._write_overlay(custom_dir, [entry])
        with pytest.raises(ValueError, match="'name'"):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_empty_position_key_rejected(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir, insertAfter="")])
        with pytest.raises(ValueError, match="insertAfter"):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_non_string_position_key_rejected(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir, insertBefore=1)])
        with pytest.raises(ValueError, match="insertBefore"):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_qml_with_path_rejected(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir, qml="./Gamma.qml")])
        with pytest.raises(ValueError, match="bare file name"):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_case_insensitive_duplicate_qml_rejected(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir, qml="alpha.qml")])
        with pytest.raises(ValueError, match=re.escape("alpha.qml")):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_pages_model_qml_name_reserved(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir, qml="settingsPagesModel.qml")])
        with pytest.raises(ValueError, match=re.escape("SettingsPagesModel.qml")):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_duplicate_qml_rejected(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir, qml="Alpha.qml")])
        with pytest.raises(ValueError, match=re.escape("Alpha.qml")):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_duplicate_qml_error_names_overlay_file(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir, qml="Alpha.qml")])
        with pytest.raises(ValueError, match=re.escape(str(custom_dir / "SettingsPages.json"))):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_overlay_unknown_key_rejected(self, stock_setup: tuple[Path, Path]):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir, bogusKey=True)])
        with pytest.raises(ValueError, match="bogusKey"):
            generate_pages_model_qml(pages_path, custom_pages_dir=custom_dir)

    def test_overlay_keys_rejected_in_stock_file(self, stock_setup: tuple[Path, Path]):
        pages_path, _ = stock_setup
        data = json.loads(pages_path.read_text(encoding="utf-8"))
        data["pages"][0]["insertAfter"] = "Beta"
        pages_path.write_text(json.dumps(data), encoding="utf-8")
        with pytest.raises(ValueError, match="insertAfter"):
            generate_pages_model_qml(pages_path)

    def test_cli_list_outputs(self, stock_setup: tuple[Path, Path], tmp_path: Path, monkeypatch, capsys):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir), {"remove": "Beta"}])
        settings_dir = _make_settings_dir(
            tmp_path,
            {"App": [{"name": "x", "type": "bool", "shortDesc": "X", "label": "X"}]},
        )
        monkeypatch.setattr(settings_generator, "PAGES_DIR", pages_path.parent)
        monkeypatch.setattr(settings_generator, "SETTINGS_DIR", settings_dir)
        monkeypatch.setattr(
            sys,
            "argv",
            ["generate_pages", "--list-outputs", "--custom-pages-dir", str(custom_dir)],
        )
        assert settings_generator.main() == 0
        lines = capsys.readouterr().out.strip().splitlines()
        assert lines == ["Alpha.qml", "Gamma.qml", "SettingsPagesModel.qml"]

    def test_cli_missing_page_definition_fatal(
        self, stock_setup: tuple[Path, Path], monkeypatch, capsys
    ):
        pages_path, custom_dir = stock_setup
        entry = self._gamma_entry(custom_dir)
        (custom_dir / "Gamma.SettingsUI.json").unlink()
        self._write_overlay(custom_dir, [entry])
        monkeypatch.setattr(settings_generator, "PAGES_DIR", pages_path.parent)
        monkeypatch.setattr(
            sys,
            "argv",
            ["generate_pages", "--list-outputs", "--custom-pages-dir", str(custom_dir)],
        )
        assert settings_generator.main() != 0
        assert "Gamma.SettingsUI.json" in capsys.readouterr().err

    def test_cli_collision_rejected_without_generated_pages(
        self, stock_setup: tuple[Path, Path], tmp_path: Path, monkeypatch
    ):
        """Accessor collisions must fail configure even when no generated page needs metadata."""
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [{"remove": "Alpha"}, {"remove": "Beta"}])
        settings_dir = _make_settings_dir(
            tmp_path, {"App": [{"name": "x", "type": "bool", "shortDesc": "X", "label": "X"}]}
        )
        (settings_dir / "SettingsManager.h").write_text(
            "Q_PROPERTY(QObject *appSettings READ appSettings CONSTANT)\n", encoding="utf-8"
        )
        custom_settings_root = tmp_path / "custom_settings"
        custom_settings_root.mkdir()
        custom_settings_dir = _make_settings_dir(
            custom_settings_root,
            {"App": [{"name": "z", "type": "bool", "shortDesc": "Z", "label": "Z"}]},
        )
        monkeypatch.setattr(settings_generator, "PAGES_DIR", pages_path.parent)
        monkeypatch.setattr(settings_generator, "SETTINGS_DIR", settings_dir)
        monkeypatch.setattr(
            sys,
            "argv",
            [
                "generate_pages",
                "--list-outputs",
                "--custom-pages-dir", str(custom_dir),
                "--custom-settings-dir", str(custom_settings_dir),
            ],
        )
        with pytest.raises(ValueError, match="appSettings"):
            settings_generator.main()

    def test_cli_generates_custom_page(self, stock_setup: tuple[Path, Path], tmp_path: Path, monkeypatch):
        pages_path, custom_dir = stock_setup
        self._write_overlay(custom_dir, [self._gamma_entry(custom_dir)])
        settings_dir = _make_settings_dir(
            tmp_path,
            {"App": [{"name": "x", "type": "bool", "shortDesc": "X", "label": "X"}]},
        )
        output_dir = tmp_path / "generated"
        monkeypatch.setattr(settings_generator, "PAGES_DIR", pages_path.parent)
        monkeypatch.setattr(settings_generator, "SETTINGS_DIR", settings_dir)
        monkeypatch.setattr(
            sys,
            "argv",
            [
                "generate_pages",
                "--output-dir", str(output_dir),
                "--custom-pages-dir", str(custom_dir),
            ],
        )
        assert settings_generator.main() == 0
        generated = sorted(p.name for p in output_dir.glob("*.qml"))
        assert generated == ["Alpha.qml", "Beta.qml", "Gamma.qml", "SettingsPagesModel.qml"]
        assert "Custom Section" in (output_dir / "Gamma.qml").read_text(encoding="utf-8")


class TestCustomSettingsMetadata:
    """Custom-build SettingsGroup.json metadata directories."""

    def test_custom_accessor_fact_type(self, tmp_path: Path):
        stock_dir = _make_settings_dir(
            tmp_path, {"App": [{"name": "x", "type": "bool", "shortDesc": "X", "label": "X"}]}
        )
        custom_root = tmp_path / "custom"
        custom_root.mkdir()
        custom_dir = _make_settings_dir(
            custom_root, {"Custom": [{"name": "z", "type": "bool", "shortDesc": "Z", "label": "Z"}]}
        )
        assert get_fact_type("customSettings.z", (stock_dir, custom_dir)) == "bool"
        assert get_fact_type("appSettings.x", (stock_dir, custom_dir)) == "bool"

    def test_page_generation_with_custom_accessor(self, tmp_path: Path):
        stock_dir = _make_settings_dir(
            tmp_path, {"App": [{"name": "x", "type": "bool", "shortDesc": "X", "label": "X"}]}
        )
        custom_root = tmp_path / "custom"
        custom_root.mkdir()
        custom_dir = _make_settings_dir(
            custom_root, {"Custom": [{"name": "z", "type": "bool", "shortDesc": "Z", "label": "Z"}]}
        )
        page = PageDef(groups=[
            GroupDef(heading="G", controls=[ControlDef(setting="customSettings.z")]),
        ])
        qml = generate_page_qml(page, (stock_dir, custom_dir))
        assert "FactCheckBoxSlider" in qml
        assert "QGroundControl.settingsManager.customSettings.z" in qml

    def test_missing_accessor_header_warns_for_custom_dir(self, tmp_path: Path, capsys):
        stock_dir = _make_settings_dir(
            tmp_path, {"App": [{"name": "x", "type": "bool", "shortDesc": "X", "label": "X"}]}
        )
        custom_root = tmp_path / "custom"
        custom_root.mkdir()
        custom_dir = _make_settings_dir(
            custom_root, {"Custom": [{"name": "z", "type": "bool", "shortDesc": "Z", "label": "Z"}]}
        )
        load_settings_metadata((stock_dir, custom_dir))
        assert "collision checking is disabled" in capsys.readouterr().err

    def test_custom_accessor_invalid_qml_identifier_rejected(self, tmp_path: Path):
        stock_dir = _make_settings_dir(
            tmp_path, {"App": [{"name": "x", "type": "bool", "shortDesc": "X", "label": "X"}]}
        )
        custom_root = tmp_path / "custom"
        custom_root.mkdir()
        custom_dir = _make_settings_dir(
            custom_root, {"3D": [{"name": "z", "type": "bool", "shortDesc": "Z", "label": "Z"}]}
        )
        with pytest.raises(ValueError, match="3DSettings"):
            load_settings_metadata((stock_dir, custom_dir))

    def test_custom_accessor_collision_rejected(self, tmp_path: Path):
        stock_dir = _make_settings_dir(
            tmp_path, {"App": [{"name": "x", "type": "bool", "shortDesc": "X", "label": "X"}]}
        )
        (stock_dir / "SettingsManager.h").write_text(
            "Q_PROPERTY(QObject *appSettings READ appSettings CONSTANT)\n", encoding="utf-8"
        )
        custom_root = tmp_path / "custom"
        custom_root.mkdir()
        custom_dir = _make_settings_dir(
            custom_root, {"App": [{"name": "z", "type": "bool", "shortDesc": "Z", "label": "Z"}]}
        )
        with pytest.raises(ValueError, match="appSettings"):
            load_settings_metadata((stock_dir, custom_dir))

    def test_duplicate_derived_custom_accessor_rejected(self, tmp_path: Path):
        stock_dir = _make_settings_dir(
            tmp_path, {"App": [{"name": "x", "type": "bool", "shortDesc": "X", "label": "X"}]}
        )
        # Two dirs: ABC/Abc as sibling files would collide on case-insensitive filesystems
        dirs = []
        for sub, stem in (("custom1", "ABC"), ("custom2", "Abc")):
            root = tmp_path / sub
            root.mkdir()
            dirs.append(_make_settings_dir(
                root, {stem: [{"name": "z", "type": "bool", "shortDesc": "Z", "label": "Z"}]}
            ))
        with pytest.raises(ValueError, match="abcSettings"):
            load_settings_metadata((stock_dir, *dirs))
