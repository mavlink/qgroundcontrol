"""
Settings QML Generator — UI-definition driven.

Reads a page-definition JSON file that describes the settings UI layout
and generates QML. The JSON references settings by their SettingsManager
accessor path (e.g. "appSettings.qLocaleLanguage") instead of being
derived directly from SettingsGroup metadata.

Also generates the SettingsPagesModel.qml from SettingsPages.json.
"""

from __future__ import annotations

import json
import re
from dataclasses import dataclass, field
from pathlib import Path

# Matches C++ FactMetaData::splitTranslatedList() regex: [,，、]
# Handles ASCII comma, fullwidth comma (U+FF0C), and enumeration comma (U+3001)
# which translators sometimes substitute for standard commas.
_TRANSLATED_LIST_RE = re.compile('[,\uff0c\u3001]')


# --------------------------------------------------------------------------- #
# Data model
# --------------------------------------------------------------------------- #

@dataclass
class ControlDef:
    """A single control referencing a setting."""
    setting: str        # e.g. "appSettings.qLocaleLanguage"
    label: str = ""     # explicit label override; empty = use fact.label
    control: str = ""   # explicit control type: "checkbox", "combobox", "textfield", "slider", "browse"; empty = auto-detect
    showWhen: str = ""  # extra visibility expression (ANDed with fact.userVisible)
    enableWhen: str = ""  # enabled expression
    placeholder: str = ""  # placeholder text for text fields
    enableCheckbox: dict = field(default_factory=dict)  # slider: {"checked": expr, "onClicked": expr}
    button: dict = field(default_factory=dict)           # adjacent button: {"text": str, "onClicked": expr, "enabled": expr}

    @property
    def settings_group(self) -> str:
        """The settings group accessor, e.g. 'appSettings'."""
        return self.setting.split(".")[0]

    @property
    def fact_name(self) -> str:
        """The fact name, e.g. 'qLocaleLanguage'."""
        return self.setting.split(".", 1)[1]


@dataclass
class GroupDef:
    """A group of controls with an optional heading."""
    heading: str = ""
    showWhen: str = ""             # optional visibility expression
    enableWhen: str = ""           # optional enabled expression
    headingDescription: str = ""   # optional dynamic heading description
    component: str = ""            # custom QML component (replaces generated controls)
    sectionName: str = ""          # display name for tree nav (falls back to heading)
    keywords: str = ""                                    # curated search keywords (comma-separated)
    controls: list[ControlDef] = field(default_factory=list)
    missing: list[str] = field(default_factory=list)  # descriptions of complex UI not yet generated

    @property
    def display_name(self) -> str:
        """Section display name for tree navigation."""
        return self.sectionName or self.heading


@dataclass
class PageDef:
    """A complete settings page definition."""
    bindings: dict[str, str] = field(default_factory=dict)  # name -> QML expression
    groups: list[GroupDef] = field(default_factory=list)


# --------------------------------------------------------------------------- #
# JSON loading
# --------------------------------------------------------------------------- #

def load_page_def(json_path: Path) -> PageDef:
    """Load a page definition from a JSON file."""
    with open(json_path, encoding="utf-8") as f:
        data = json.load(f)

    page = PageDef(bindings=data.get("bindings", {}))
    for grp_data in data.get("groups", []):
        grp = GroupDef(
            heading=grp_data.get("heading", ""),
            showWhen=grp_data.get("showWhen", ""),
            enableWhen=grp_data.get("enableWhen", ""),
            headingDescription=grp_data.get("headingDescription", ""),
            component=grp_data.get("component", ""),
            sectionName=grp_data.get("sectionName", ""),
            keywords=grp_data.get("keywords", ""),
            missing=grp_data.get("missing", []),
        )
        for ctrl_data in grp_data.get("controls", []):
            grp.controls.append(ControlDef(
                setting=ctrl_data["setting"],
                label=ctrl_data.get("label", ""),
                control=ctrl_data.get("control", ""),
                showWhen=ctrl_data.get("showWhen", ""),
                enableWhen=ctrl_data.get("enableWhen", ""),
                placeholder=ctrl_data.get("placeholder", ""),
                enableCheckbox=ctrl_data.get("enableCheckbox", {}),
                button=ctrl_data.get("button", {}),
            ))
        page.groups.append(grp)
    return page


# --------------------------------------------------------------------------- #
# Settings metadata loading (for type info needed by control selection)
# --------------------------------------------------------------------------- #

_metadata_cache: dict[str, dict] = {}


def _load_settings_metadata(settings_dir: Path) -> dict[str, dict]:
    """Load all SettingsGroup.json files and build a lookup.

    Returns: { "factName": { full fact metadata dict } }
    keyed by "settingsGroup.factName" e.g. "appSettings.qLocaleLanguage"
    """
    if _metadata_cache:
        return _metadata_cache

    # Map settings group JSON stems to their accessor names
    stem_to_accessor = {
        "ADSBVehicleManager": "adsbVehicleManagerSettings",
        "APMMavlinkStreamRate": "apmMavlinkStreamRateSettings",
        "App": "appSettings",
        "AutoConnect": "autoConnectSettings",
        "BatteryIndicator": "batteryIndicatorSettings",
        "FirmwareUpgrade": "firmwareUpgradeSettings",
        "FlightMap": "flightMapSettings",
        "FlightMode": "flightModeSettings",
        "FlyView": "flyViewSettings",
        "GimbalController": "gimbalControllerSettings",
        "Joystick": "joystickSettings",
        "JoystickManager": "joystickManagerSettings",
        "Maps": "mapsSettings",
        "Mavlink": "mavlinkSettings",
        "MavlinkActions": "mavlinkActionsSettings",
        "NTRIP": "ntripSettings",
        "OfflineMaps": "offlineMapsSettings",
        "PlanView": "planViewSettings",
        "RTK": "rtkSettings",
        "RemoteID": "remoteIDSettings",
        "Units": "unitsSettings",
        "Video": "videoSettings",
        "Viewer3D": "viewer3DSettings",
    }

    for json_path in settings_dir.glob("*.SettingsGroup.json"):
        stem = json_path.name.replace(".SettingsGroup.json", "")
        accessor = stem_to_accessor.get(stem)
        if not accessor:
            continue

        with open(json_path, encoding="utf-8") as f:
            data = json.load(f)

        for fact in data.get("QGC.MetaData.Facts", []):
            key = f"{accessor}.{fact['name']}"
            _metadata_cache[key] = fact

    return _metadata_cache


def _get_fact_type(setting: str, settings_dir: Path) -> str:
    """Get the type of a fact from its metadata."""
    meta = _load_settings_metadata(settings_dir)
    fact = meta.get(setting, {})
    return fact.get("type", "string").lower()


def _has_enum_strings(setting: str, settings_dir: Path) -> bool:
    """Check if a fact has enum strings defined."""
    meta = _load_settings_metadata(settings_dir)
    fact = meta.get(setting, {})
    return bool(fact.get("enumStrings", ""))


def _split_translated_list(csv: str) -> list[str]:
    """Split a comma-separated string the same way C++ splitTranslatedList does."""
    return [s.strip() for s in _TRANSLATED_LIST_RE.split(csv) if s.strip()]


def _get_fact_search_terms(setting: str, settings_dir: Path) -> list[str]:
    """Get search terms for a fact from its keywords."""
    meta = _load_settings_metadata(settings_dir)
    fact = meta.get(setting, {})
    kw_str = fact.get("keywords", "")
    if not kw_str:
        return []
    return [kw.lower() for kw in _split_translated_list(kw_str)]


# --------------------------------------------------------------------------- #
# QML generation
# --------------------------------------------------------------------------- #

def _qml_control(ctrl: ControlDef, settings_dir: Path) -> str:
    """Generate QML for a single control."""
    indent = "        "
    fact_ref = f"QGroundControl.settingsManager.{ctrl.setting}"

    def _vis_line() -> str:
        if ctrl.showWhen:
            return f"{indent}    visible: ({ctrl.showWhen}) && fact.userVisible"
        return f"{indent}    visible: fact.userVisible"

    def _enabled_line() -> str:
        if ctrl.enableWhen:
            return f"{indent}    enabled: {ctrl.enableWhen}\n"
        return ""

    # Determine control type: explicit override or auto-detect from metadata
    if ctrl.control == "slider":
        label_line = f'    label: qsTr("{ctrl.label}")' if ctrl.label else "    label: fact.label"
        inner_indent = indent
        has_button = bool(ctrl.button)
        if has_button:
            inner_indent = indent + "    "
        lines = []
        if has_button:
            lines.append(f"{indent}RowLayout {{")
            lines.append(f"{indent}    Layout.fillWidth: true")
            lines.append(f"{indent}    spacing: ScreenTools.defaultFontPixelWidth")
            if ctrl.showWhen:
                lines.append(f"{indent}    visible: ({ctrl.showWhen}) && {fact_ref}.userVisible")
            else:
                lines.append(f"{indent}    visible: {fact_ref}.userVisible")
        lines.append(f"{inner_indent}FactTextFieldSlider {{")
        lines.append(f"{inner_indent}    Layout.fillWidth: true")
        lines.append(f"{inner_indent}{label_line}")
        lines.append(f"{inner_indent}    fact: {fact_ref}")
        if not has_button:
            if ctrl.showWhen:
                lines.append(f"{inner_indent}    visible: ({ctrl.showWhen}) && fact.userVisible")
            else:
                lines.append(f"{inner_indent}    visible: fact.userVisible")
        if ctrl.enableCheckbox:
            lines.append(f"{inner_indent}    showEnableCheckbox: true")
            if ctrl.enableCheckbox.get("checked"):
                lines.append(f"{inner_indent}    enableCheckBoxChecked: {ctrl.enableCheckbox['checked']}")
            if ctrl.enableCheckbox.get("onClicked"):
                lines.append(f"{inner_indent}    onEnableCheckboxClicked: {ctrl.enableCheckbox['onClicked']}")
        if ctrl.enableWhen:
            lines.append(f"{inner_indent}    enabled: {ctrl.enableWhen}")
        lines.append(f"{inner_indent}}}")
        if has_button:
            btn = ctrl.button
            lines.append(f'{inner_indent}QGCButton {{')
            lines.append(f'{inner_indent}    text: qsTr("{btn["text"]}")')
            lines.append(f'{inner_indent}    onClicked: {btn["onClicked"]}')
            if btn.get("enabled"):
                lines.append(f'{inner_indent}    enabled: {btn["enabled"]}')
            lines.append(f'{inner_indent}}}')
            lines.append(f"{indent}}}")
        return "\n".join(lines)
    elif ctrl.control == "browse":
        label_line = f'    label: qsTr("{ctrl.label}")' if ctrl.label else "    label: fact.label"
        enabled = _enabled_line()
        return (
            f"{indent}LabelledFactBrowse {{\n"
            f"{indent}    Layout.fillWidth: true\n"
            f"{indent}{label_line}\n"
            f"{indent}    fact: {fact_ref}\n"
            f"{_vis_line()}\n"
            f"{enabled}"
            f"{indent}}}"
        )
    elif ctrl.control == "scaler":
        label_line = f'    label: qsTr("{ctrl.label}")' if ctrl.label else "    label: fact.label"
        enabled = _enabled_line()
        return (
            f"{indent}LabelledFactIncrementer {{\n"
            f"{indent}    Layout.fillWidth: true\n"
            f"{indent}{label_line}\n"
            f"{indent}    fact: {fact_ref}\n"
            f"{_vis_line()}\n"
            f"{enabled}"
            f"{indent}}}"
        )
    elif ctrl.control == "checkbox":
        use_checkbox = True
        use_combobox = False
    elif ctrl.control == "combobox":
        use_checkbox = False
        use_combobox = True
    elif ctrl.control == "textfield":
        use_checkbox = False
        use_combobox = False
    else:
        fact_type = _get_fact_type(ctrl.setting, settings_dir)
        has_enums = _has_enum_strings(ctrl.setting, settings_dir)
        use_checkbox = (fact_type == "bool")
        use_combobox = (not use_checkbox and has_enums)

    enabled = _enabled_line()

    if use_checkbox:
        label_line = f'    text: qsTr("{ctrl.label}")' if ctrl.label else "    text: fact.label"
        return (
            f"{indent}FactCheckBoxSlider {{\n"
            f"{indent}    Layout.fillWidth: true\n"
            f"{indent}{label_line}\n"
            f"{indent}    fact: {fact_ref}\n"
            f"{_vis_line()}\n"
            f"{enabled}"
            f"{indent}}}"
        )
    elif use_combobox:
        label_line = f'    label: qsTr("{ctrl.label}")' if ctrl.label else "    label: fact.label"
        return (
            f"{indent}LabelledFactComboBox {{\n"
            f"{indent}    Layout.fillWidth: true\n"
            f"{indent}{label_line}\n"
            f"{indent}    fact: {fact_ref}\n"
            f"{indent}    indexModel: false\n"
            f"{_vis_line()}\n"
            f"{enabled}"
            f"{indent}}}"
        )
    else:
        label_line = f'    label: qsTr("{ctrl.label}")' if ctrl.label else "    label: fact.label"
        fact_type = _get_fact_type(ctrl.setting, settings_dir)
        lines = [
            f"{indent}LabelledFactTextField {{",
            f"{indent}    Layout.fillWidth: true",
            f"{indent}{label_line}",
            f"{indent}    fact: {fact_ref}",
            _vis_line(),
        ]
        if ctrl.enableWhen:
            lines.append(f"{indent}    enabled: {ctrl.enableWhen}")
        if fact_type == "string":
            lines.append(f"{indent}    textFieldPreferredWidth: _stringFieldWidth")
        if ctrl.placeholder:
            lines.append(f'{indent}    textField.placeholderText: qsTr("{ctrl.placeholder}")')
        lines.append(f"{indent}}}")
        return "\n".join(lines)


def _qml_missing_placeholder(description: str) -> str:
    """Generate QML for a missing-feature placeholder label."""
    indent = "        "
    return (
        f"{indent}QGCLabel {{\n"
        f"{indent}    Layout.fillWidth: true\n"
        f'{indent}    text: qsTr("TODO: {description}")\n'
        f"{indent}    font.italic: true\n"
        f"{indent}    opacity: 0.6\n"
        f"{indent}}}"
    )


def _needs_string_field_width(page: PageDef, settings_dir: Path) -> bool:
    """Check if any control in the page is a string type needing wider fields."""
    for grp in page.groups:
        for ctrl in grp.controls:
            fact_type = _get_fact_type(ctrl.setting, settings_dir)
            has_enums = _has_enum_strings(ctrl.setting, settings_dir)
            if fact_type == "string" and not has_enums:
                return True
    return False


def generate_page_qml(page: PageDef, settings_dir: Path) -> str:
    """Generate a complete QML settings page from a page definition."""
    has_string_fields = _needs_string_field_width(page, settings_dir)

    lines: list[str] = []

    # Imports
    lines.append("import QtQuick")
    lines.append("import QtQuick.Controls")
    lines.append("import QtQuick.Layouts")
    lines.append("")
    lines.append("import QGroundControl")
    lines.append("import QGroundControl.FactControls")
    lines.append("import QGroundControl.Controls")
    lines.append("")

    # Root element
    lines.append("SettingsPage {")
    if has_string_fields:
        lines.append("    property real _stringFieldWidth: ScreenTools.defaultFontPixelWidth * 30")

    # Emit page-level bindings as QML properties
    for name, expr in page.bindings.items():
        # Infer QML type from expression
        if expr.startswith(("QGroundControl.", "_settingsManager.")):
            qml_type = "var"
        elif any(op in expr for op in ["&&", "||", "===", "!=="]):
            qml_type = "bool"
        elif expr.startswith(("qsTr(", '"')):
            qml_type = "string"
        else:
            qml_type = "var"
        lines.append(f"    property {qml_type} {name}: {expr}")

    lines.append("")

    # Groups
    for grp_idx, grp in enumerate(page.groups):
        section_vis = f"(sectionFilter === -1 || sectionFilter === {grp_idx})"

        # Custom component: emit it directly instead of generating controls
        if grp.component:
            lines.append(f"    {grp.component} {{")
            lines.append("        Layout.fillWidth: true")
            if grp.showWhen:
                lines.append(f"        visible: {section_vis} && ({grp.showWhen})")
            else:
                lines.append(f"        visible: {section_vis}")
            lines.append("    }")
            lines.append("")
            continue

        lines.append("    SettingsGroupLayout {")
        lines.append("        Layout.fillWidth: true")
        if grp.heading:
            lines.append(f'        heading: qsTr("{grp.heading}")')
        if grp.headingDescription:
            lines.append(f"        headingDescription: {grp.headingDescription}")

        # Build visibility expression: sectionFilter AND explicit showWhen AND auto-hide when all controls are hidden
        vis_parts = [section_vis]
        if grp.showWhen:
            vis_parts.append(f"({grp.showWhen})")
        if grp.controls:
            fact_refs = [f"QGroundControl.settingsManager.{c.setting}" for c in grp.controls]
            auto_vis = " || ".join(f"{ref}.userVisible" for ref in fact_refs)
            vis_parts.append(f"({auto_vis})")
        lines.append(f"        visible: {' && '.join(vis_parts)}")

        if grp.enableWhen:
            lines.append(f"        enabled: {grp.enableWhen}")

        lines.append("")

        for ctrl in grp.controls:
            lines.append(_qml_control(ctrl, settings_dir))
            lines.append("")

        for desc in grp.missing:
            lines.append(_qml_missing_placeholder(desc))
            lines.append("")

        # Remove trailing blank line inside the group
        if lines and lines[-1] == "":
            lines.pop()
        lines.append("    }")
        lines.append("")

    # Remove trailing blank line
    if lines and lines[-1] == "":
        lines.pop()
    lines.append("}")
    lines.append("")

    return "\n".join(lines)


def generate_pages_model_qml(pages_json_path: Path) -> str:
    """Generate SettingsPagesModel.qml from SettingsPages.json.

    Each page entry includes a ``sections`` property — a JSON-encoded array
    of section display names extracted from the page's SettingsUI.json.
    The sidebar tree uses this to build a two-level hierarchy.
    """
    with open(pages_json_path, encoding="utf-8") as f:
        data = json.load(f)

    pages_dir = pages_json_path.parent

    lines: list[str] = []
    lines.append("import QtQml.Models")
    lines.append("")
    lines.append("import QGroundControl")
    lines.append("import QGroundControl.Controls")
    lines.append("")
    lines.append("ListModel {")

    for entry in data.get("pages", []):
        if entry.get("divider"):
            lines.append("")
            lines.append("    ListElement {")
            lines.append('        name: "Divider"')
            lines.append("    }")
            continue

        name = entry["name"]
        icon = entry["icon"]
        visible = entry.get("visible", "")

        # Support either full "url" or short "qml" (prefixed with AppSettings base)
        url = entry.get("url")
        if not url:
            qml = entry["qml"]
            url = f"qrc:/qml/QGroundControl/AppSettings/{qml}"

        # Extract section names and search terms from the page definition
        sections: list[str] = []
        # searchTerms: list of {section: index, terms: "section heading keywords..."}
        search_terms: list[dict] = []
        page_def_name = entry.get("pageDefinition")
        if page_def_name:
            page_def_path = pages_dir / page_def_name
            if page_def_path.exists():
                settings_dir = pages_dir.parent.parent.parent / "Settings"
                page_def = load_page_def(page_def_path)
                for grp_idx, grp in enumerate(page_def.groups):
                    section_name = grp.display_name
                    sections.append(section_name)
                    # Build search terms from page name + section heading
                    terms_parts = [name.lower(), section_name.lower()]
                    if grp.controls:
                        # Collect keywords from individual fact metadata
                        for ctrl in grp.controls:
                            terms_parts.extend(
                                _get_fact_search_terms(ctrl.setting, settings_dir)
                            )
                    else:
                        # Component-only group: use group-level keywords
                        if grp.keywords:
                            terms_parts.extend(
                                kw.lower() for kw in _split_translated_list(grp.keywords)
                            )
                    # Deduplicate while preserving order
                    seen: set[str] = set()
                    unique_terms: list[str] = []
                    for t in terms_parts:
                        if t not in seen:
                            seen.add(t)
                            unique_terms.append(t)
                    search_terms.append({
                        "section": grp_idx,
                        "terms": " ".join(unique_terms),
                    })

        # Encode as JSON strings for the ListElement
        sections_json = json.dumps(sections)
        search_json = json.dumps(search_terms).replace("'", "\\'")

        lines.append("")
        lines.append("    ListElement {")
        lines.append(f'        name: qsTr("{name}")')
        lines.append(f'        url: "{url}"')
        lines.append(f'        iconUrl: "{icon}"')
        lines.append(f"        sections: '{sections_json}'")
        lines.append(f"        searchTerms: '{search_json}'")
        if visible:
            lines.append(f"        pageVisible: function() {{ return {visible} }}")
        else:
            lines.append("        pageVisible: function() { return true }")
        lines.append("    }")

    lines.append("}")
    lines.append("")

    return "\n".join(lines)
