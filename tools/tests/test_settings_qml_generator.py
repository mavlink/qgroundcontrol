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
        with pytest.raises(ValueError, match="settingsGroupAccessor.factName"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_dotless_setting_rejected(self, tmp_path: Path):
        # Forgetting the settings group prefix must be a clear authoring error
        data = {
            "version": 1,
            "groups": [{"heading": "G", "controls": [{"setting": "operatorIDEU"}]}],
        }
        with pytest.raises(ValueError, match="operatorIDEU"):
            load_page_def(_make_page_json(tmp_path, data))

    @pytest.mark.parametrize("bad_setting", ["appSettings..x", "appSettings.x.", ".x", "appSettings.höhe"])
    def test_malformed_setting_segments_rejected(self, tmp_path: Path, bad_setting: str):
        # Empty path segments or non-ASCII would emit broken fact refs / objectNames
        data = {
            "version": 1,
            "groups": [{"heading": "G", "controls": [{"setting": bad_setting}]}],
        }
        with pytest.raises(ValueError, match="settingsGroupAccessor.factName"):
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
            "groups": [{"heading": "G", "controls": [{"setting": "appSettings.x", "enableCheckbox": bad_value}]}],
        }
        with pytest.raises(ValueError, match="enableCheckbox"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_unknown_group_key_rejected(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [{"heading": "G", "showWen": "typo", "controls": [{"setting": "appSettings.x"}]}],
        }
        with pytest.raises(ValueError, match="showWen"):
            load_page_def(_make_page_json(tmp_path, data))

    def test_unknown_control_key_rejected(self, tmp_path: Path):
        data = {
            "version": 1,
            "groups": [{"heading": "G", "controls": [{"setting": "appSettings.x", "enabelWhen": "typo"}]}],
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
                GroupDef(heading="EU Vehicle Info", controls=[ControlDef(setting="appSettings.savePath")]),
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
                GroupDef(heading="EU Vehicle Info", controls=[ControlDef(setting="appSettings.savePath")]),
                GroupDef(heading="EU-Vehicle Info", controls=[ControlDef(setting="appSettings.enableFeature")]),
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
            "groups": [
                {"heading": "Section A", "controls": [{"setting": "appSettings.x"}]},
                {"heading": "Section B", "controls": [{"setting": "appSettings.y"}]},
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

    def test_unknown_root_key_rejected(self, tmp_path: Path):
        pages_path = tmp_path / "SettingsPages.json"
        pages_path.write_text(json.dumps({
            "version": 1,
            "bogusRootKey": True,
            "pages": [{"name": "P", "qml": "P.qml", "icon": "qrc:/p.svg"}],
        }), encoding="utf-8")
        with pytest.raises(ValueError, match="bogusRootKey"):
            generate_pages_model_qml(pages_path)

    def test_unknown_page_entry_key_rejected(self, tmp_path: Path):
        pages_path = tmp_path / "SettingsPages.json"
        pages_path.write_text(json.dumps({
            "version": 1,
            "pages": [{"name": "P", "qml": "P.qml", "icon": "qrc:/p.svg", "vissible": "typo"}],
        }), encoding="utf-8")
        with pytest.raises(ValueError, match="vissible"):
            generate_pages_model_qml(pages_path)

    def test_comment_keys_accepted(self, tmp_path: Path):
        pages_path = tmp_path / "SettingsPages.json"
        pages_path.write_text(json.dumps({
            "version": 1,
            "comment": "root note",
            "pages": [{"name": "P", "qml": "P.qml", "icon": "qrc:/p.svg", "comment": "entry note"}],
        }), encoding="utf-8")
        qml = generate_pages_model_qml(pages_path)
        assert 'nameKey: "P"' in qml

    def test_non_array_pages_rejected(self, tmp_path: Path):
        # pages: "oops" must not be iterated per-character
        pages_path = tmp_path / "SettingsPages.json"
        pages_path.write_text(json.dumps({"version": 1, "pages": "oops"}), encoding="utf-8")
        with pytest.raises(ValueError, match="must be a JSON array"):
            generate_pages_model_qml(pages_path)


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

    def test_pages_model_generates(self, repo_root: Path):
        pages_path = repo_root / "src" / "AppSettings" / "pages" / "SettingsPages.json"
        qml = generate_pages_model_qml(pages_path)
        assert "ListModel {" in qml
        assert "General" in qml
