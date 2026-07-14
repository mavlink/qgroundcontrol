"""Contract tests for the settings QML generator."""

import json
from pathlib import Path

import pytest
from generators.common.controls import ButtonDef
from generators.settings_qml.page_generator import (
    ControlDef,
    GroupDef,
    PageDef,
    generate_page_qml,
    generate_pages_model_qml,
    load_page_def,
)

from ._helpers import REPO_ROOT


def _write_json(path: Path, data: object) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data), encoding="utf-8")
    return path


def _settings_dir(tmp_path: Path) -> Path:
    return _write_json(
        tmp_path / "Settings" / "App.SettingsGroup.json",
        {
            "version": 1,
            "fileType": "FactMetaData",
            "QGC.MetaData.Facts": [
                {"name": "enabled", "type": "bool", "label": "Enabled"},
                {"name": "mode", "type": "uint32", "label": "Mode", "enumStrings": "A,B"},
                {"name": "limit", "type": "double", "label": "Limit"},
                {"name": "path", "type": "string", "label": "Path"},
            ],
        },
    ).parent


def test_load_page_definition_preserves_structure(tmp_path: Path):
    path = _write_json(
        tmp_path / "Test.SettingsUI.json",
        {
            "version": 1,
            "comment": "schema comments are allowed",
            "imports": ["My.Module"],
            "bindings": {"_manager": "QGroundControl.settingsManager"},
            "groups": [
                {
                    "heading": "General",
                    "showWhen": "visibleFlag",
                    "enableWhen": "enabledFlag",
                    "controls": [
                        {
                            "setting": "appSettings.limit",
                            "label": "Altitude",
                            "control": "slider",
                            "showWhen": "controlVisible",
                        }
                    ],
                },
                {"component": "CustomGroup", "sectionName": "Custom", "keywords": "one,two"},
            ],
        },
    )

    page = load_page_def(path)

    assert page.imports == ["My.Module"]
    assert page.bindings == {"_manager": "QGroundControl.settingsManager"}
    assert page.groups[0].display_name == "General"
    assert page.groups[0].controls[0] == ControlDef(
        setting="appSettings.limit",
        label="Altitude",
        control="slider",
        showWhen="controlVisible",
    )
    assert page.groups[1].display_name == "Custom"
    assert page.groups[1].keywords == ["one", "two"]


def test_load_page_definition_rejects_invalid_schema(tmp_path: Path):
    for data, message in (
        ([{"groups": []}], "must be a JSON object"),
        ({"groups": "bad"}, "must be a JSON array"),
        ({"groups": [{"controls": ["bad"]}]}, "must be a JSON object"),
        ({"unknown": True, "groups": []}, "unknown"),
        ({"groups": [{"controls": [{"setting": "missingDot"}]}]}, "settingsGroupAccessor.factName"),
        (
            {"groups": [{"controls": [{"setting": "appSettings.value", "button": "bad"}]}]},
            "button",
        ),
    ):
        path = _write_json(tmp_path / "Invalid.SettingsUI.json", data)
        with pytest.raises(ValueError, match=message):
            load_page_def(path)


def test_generate_page_covers_fact_and_custom_controls(tmp_path: Path):
    page = PageDef(
        bindings={"_manager": "QGroundControl.settingsManager"},
        groups=[
            GroupDef(
                heading="Main Settings",
                showWhen="groupVisible",
                enableWhen="groupEnabled",
                controls=[
                    ControlDef(setting="appSettings.enabled"),
                    ControlDef(setting="appSettings.mode"),
                    ControlDef(setting="appSettings.limit", control="slider", enableWhen="canEdit"),
                    ControlDef(setting="appSettings.path", control="browse", label="Save Path"),
                    ControlDef(
                        control="info",
                        label="Bytes sent",
                        value="sink.bytesSentDisplay",
                        showWhen="sink.enabled",
                        button=ButtonDef(text="Reset", onClicked="sink.reset()"),
                    ),
                    ControlDef(control="component", component="InlineWidget", enableWhen="canEdit"),
                ],
            ),
            GroupDef(component="CustomGroup", showWhen="customVisible"),
        ],
    )

    qml = generate_page_qml(page, _settings_dir(tmp_path), page_name="Fly View")

    for fragment in (
        "SettingsPage {",
        'objectName: "settingsPage_FlyView"',
        'objectName: "settingsGroup_MainSettings"',
        "property var _manager: QGroundControl.settingsManager",
        "FactCheckBoxSlider {",
        "LabelledFactComboBox {",
        "FactTextFieldSlider {",
        "LabelledFactBrowse {",
        "LabelledLabel {",
        "InlineWidget {",
        "CustomGroup {",
        'text: qsTr("Reset")',
        "onClicked: sink.reset()",
        "visible: sink.enabled",
        "enabled: canEdit",
        "(groupVisible)",
        "enabled: groupEnabled",
    ):
        assert fragment in qml


def test_generate_page_rejects_ambiguous_object_names(tmp_path: Path):
    page = PageDef(
        groups=[
            GroupDef(heading="EU Vehicle", controls=[ControlDef(setting="appSettings.path")]),
            GroupDef(heading="EU-Vehicle", controls=[ControlDef(setting="appSettings.enabled")]),
        ]
    )
    with pytest.raises(ValueError, match="settingsGroup_EUVehicle"):
        generate_page_qml(page, _settings_dir(tmp_path))


def test_generate_pages_model_covers_pages_sections_and_visibility(tmp_path: Path):
    pages_dir = tmp_path / "src" / "AppSettings" / "pages"
    _write_json(
        pages_dir / "Test.SettingsUI.json",
        {
            "groups": [
                {"heading": "Section A", "controls": [{"setting": "appSettings.enabled"}]},
                {"heading": "Section B", "controls": [{"setting": "appSettings.limit"}]},
            ]
        },
    )
    pages_path = _write_json(
        pages_dir / "SettingsPages.json",
        {
            "pages": [
                {
                    "name": "Test Page",
                    "qml": "TestPage.qml",
                    "icon": "qrc:/test.svg",
                    "pageDefinition": "Test.SettingsUI.json",
                    "visible": "QGroundControl.someFlag",
                },
                {"divider": True},
            ]
        },
    )

    qml = generate_pages_model_qml(pages_path)

    for fragment in (
        "ListModel {",
        'nameKey: "Test Page"',
        "qrc:/qml/QGroundControl/AppSettings/TestPage.qml",
        "Section A",
        "Section B",
        "searchTerms",
        "QGroundControl.someFlag",
        '"Divider"',
    ):
        assert fragment in qml


def test_generate_pages_model_rejects_invalid_schema(tmp_path: Path):
    for data in (
        {"unknown": True, "pages": []},
        {"pages": "bad"},
        {"pages": [{"name": "P", "qml": "P.qml", "vissible": True}]},
    ):
        path = _write_json(tmp_path / "SettingsPages.json", data)
        with pytest.raises(ValueError):
            generate_pages_model_qml(path)


def test_all_repository_settings_definitions_generate():
    pages_dir = REPO_ROOT / "src" / "AppSettings" / "pages"
    settings_dir = REPO_ROOT / "src" / "Settings"
    definitions = sorted(pages_dir.glob("*.SettingsUI.json"))
    assert definitions

    for path in definitions:
        assert "SettingsPage {" in generate_page_qml(load_page_def(path), settings_dir)

    assert "ListModel {" in generate_pages_model_qml(pages_dir / "SettingsPages.json")
