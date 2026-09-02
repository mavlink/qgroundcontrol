"""Dataclasses + JSON loader for SettingsUI page definitions."""

from __future__ import annotations

import json
import re
from dataclasses import dataclass, field
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from pathlib import Path

from ..common.controls import (
    BaseControlDef,
    parse_button,
    parse_enable_checkbox,
)
from ..common.validation import (
    clamped_repr,
    reject_unknown_keys,
    require_dict,
    require_list,
    require_qml_safe_string,
)

# Matches C++ FactMetaData::splitTranslatedList: [,，、] (ASCII / fullwidth / enumeration commas).
_TRANSLATED_LIST_RE = re.compile("[,，、]")

# Fact-backed control settings: "settingsGroupAccessor.factName" (nested fact names allowed).
# Segments are QML property names (no leading digit); fact_name also feeds objectNames,
# which must stay grep-able.
_SETTING_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*(\.[A-Za-z_][A-Za-z0-9_]*)+")

# QML property names emitted verbatim into generated QML; a bad name must fail here
# with context, not as a qmllint/build error pointing at generated code.
_QML_PROPERTY_NAME_RE = re.compile(r"[a-z_][A-Za-z0-9_]*")

# Names the control template already emits (or that QML treats specially); a JSON
# 'properties' entry using one would generate a duplicate binding.
_RESERVED_PROPERTY_NAMES = frozenset({"id", "objectName", "label", "fact", "enabled"})


def _coerce_property_value(value: object) -> str:
    """Convert a JSON property value to a QML expression string.

    JSON booleans and numbers map to their QML literals so authors can write
    the natural spelling (e.g. ``"selectFolder": false``); strings pass
    through verbatim as QML expressions. Anything else is a schema error.
    """
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, (int, float)):
        return repr(value)
    if isinstance(value, str):
        return value
    raise ValueError(
        f"'properties' value must be a string, boolean, or number, "
        f"got {type(value).__name__}: {clamped_repr(value)}"
    )


@dataclass
class ControlDef(BaseControlDef):
    """A single control referencing a setting."""
    placeholder: str = ""
    value: str = ""
    properties: dict[str, str] = field(default_factory=dict)

    @property
    def settings_group(self) -> str:
        return self.setting.split(".")[0]

    @property
    def fact_name(self) -> str:
        return self.setting.split(".", 1)[1]


@dataclass
class GroupDef:
    """A group of controls with an optional heading."""
    heading: str = ""
    showWhen: str = ""
    enableWhen: str = ""
    headingDescription: str = ""
    component: str = ""
    sectionName: str = ""
    keywords: list[str] = field(default_factory=list)
    controls: list[ControlDef] = field(default_factory=list)
    missing: list[str] = field(default_factory=list)

    @property
    def display_name(self) -> str:
        return self.sectionName or self.heading


@dataclass
class PageDef:
    """A complete settings page definition."""
    imports: list[str] = field(default_factory=list)
    bindings: dict[str, str] = field(default_factory=dict)
    groups: list[GroupDef] = field(default_factory=list)


def split_translated_list(csv: str) -> list[str]:
    """Split a comma-separated string the same way C++ splitTranslatedList does."""
    return [s.strip() for s in _TRANSLATED_LIST_RE.split(csv) if s.strip()]


def parse_keywords(raw: list[str] | str) -> list[str]:
    """Accept keywords as a JSON array or comma-separated string."""
    if isinstance(raw, list):
        return raw
    if isinstance(raw, str) and raw:
        return [kw.strip() for kw in split_translated_list(raw) if kw.strip()]
    return []


_ALLOWED_ROOT_KEYS = frozenset({
    "fileType", "version", "comment", "imports", "bindings", "groups",
})
_ALLOWED_GROUP_KEYS = frozenset({
    "comment", "heading", "showWhen", "enableWhen", "headingDescription",
    "component", "sectionName", "keywords", "missing", "controls",
})
_ALLOWED_CONTROL_KEYS = frozenset({
    "comment", "setting", "label", "control", "showWhen", "enableWhen",
    "placeholder", "value", "component", "properties",
    "enableCheckbox", "button",
})


def load_page_def(json_path: Path) -> PageDef:
    """Load a page definition from a JSON file."""
    with open(json_path, encoding="utf-8") as f:
        data = json.load(f)

    reject_unknown_keys(data, _ALLOWED_ROOT_KEYS, "page", json_path)

    page = PageDef(
        imports=require_list(data.get("imports", []), "'imports'", json_path),
        bindings=require_dict(data.get("bindings", {}), "'bindings'", json_path),
    )
    for grp_data in require_list(data.get("groups", []), "'groups'", json_path):
        reject_unknown_keys(grp_data, _ALLOWED_GROUP_KEYS, "group", json_path)
        grp = GroupDef(
            heading=require_qml_safe_string(
                grp_data.get("heading", ""), "group heading", json_path
            ),
            showWhen=grp_data.get("showWhen", ""),
            enableWhen=grp_data.get("enableWhen", ""),
            # headingDescription is a QML expression (e.g. qsTr(...)), emitted raw
            headingDescription=grp_data.get("headingDescription", ""),
            component=grp_data.get("component", ""),
            sectionName=require_qml_safe_string(
                grp_data.get("sectionName", ""), "group sectionName", json_path
            ),
            keywords=[
                require_qml_safe_string(kw, "group keyword", json_path)
                for kw in parse_keywords(grp_data.get("keywords", []))
            ],
            missing=require_list(grp_data.get("missing", []), "group 'missing'", json_path),
        )
        for ctrl_data in require_list(grp_data.get("controls", []), "group 'controls'", json_path):
            reject_unknown_keys(ctrl_data, _ALLOWED_CONTROL_KEYS, "control", json_path)
            ctrl = ControlDef(
                setting=ctrl_data.get("setting", ""),
                label=require_qml_safe_string(
                    ctrl_data.get("label", ""), "control label", json_path
                ),
                control=ctrl_data.get("control", ""),
                showWhen=ctrl_data.get("showWhen", ""),
                enableWhen=ctrl_data.get("enableWhen", ""),
                placeholder=require_qml_safe_string(
                    ctrl_data.get("placeholder", ""), "control placeholder", json_path
                ),
                value=ctrl_data.get("value", ""),
                component=ctrl_data.get("component", ""),
                properties=require_dict(ctrl_data.get("properties", {}), "control 'properties'", json_path),
                enableCheckbox=parse_enable_checkbox(ctrl_data.get("enableCheckbox")),
                button=parse_button(ctrl_data.get("button")),
            )
            if ctrl.properties and ctrl.control not in ("browse", "scaler"):
                raise ValueError(
                    f"{json_path}: 'properties' is only supported on 'browse'/'scaler' controls, "
                    f"got control {ctrl.control!r} (control: {clamped_repr(ctrl_data)})"
                )
            for prop_name, prop_value in ctrl.properties.items():
                if not _QML_PROPERTY_NAME_RE.fullmatch(prop_name):
                    raise ValueError(
                        f"{json_path}: 'properties' key must be a valid QML property name, "
                        f"got: {prop_name!r} (control: {clamped_repr(ctrl_data)})"
                    )
                if prop_name in _RESERVED_PROPERTY_NAMES:
                    raise ValueError(
                        f"{json_path}: 'properties' key {prop_name!r} is reserved (already emitted "
                        f"by the generator) (control: {clamped_repr(ctrl_data)})"
                    )
                try:
                    ctrl.properties[prop_name] = _coerce_property_value(prop_value)
                except ValueError as exc:
                    raise ValueError(f"{json_path}: {exc} (control: {clamped_repr(ctrl_data)})") from None
            # component/info controls have no fact; every other kind derives its fact
            # reference and objectName from setting, so a bad one must fail here with
            # context, not deep inside the emitter with an IndexError
            if ctrl.control not in ("component", "info") and not _SETTING_RE.fullmatch(ctrl.setting):
                raise ValueError(
                    f"{json_path}: control setting must be 'settingsGroupAccessor.factName', "
                    f"got: {ctrl.setting!r} (control: {clamped_repr(ctrl_data)})"
                )
            grp.controls.append(ctrl)
        page.groups.append(grp)
    return page


_ALLOWED_PAGES_ROOT_KEYS = frozenset({"fileType", "version", "comment", "pages"})
_ALLOWED_PAGE_ENTRY_KEYS = frozenset({
    "comment", "divider", "name", "url", "qml", "icon", "visible", "pageDefinition",
})
_OVERLAY_POSITION_KEYS = frozenset({"insertAfter", "insertBefore"})
_ALLOWED_OVERLAY_ENTRY_KEYS = _ALLOWED_PAGE_ENTRY_KEYS | _OVERLAY_POSITION_KEYS | {"remove"}


def _load_pages_file(pages_json_path: Path, allowed_entry_keys: frozenset[str]) -> list[dict]:
    with open(pages_json_path, encoding="utf-8") as f:
        data = json.load(f)
    reject_unknown_keys(data, _ALLOWED_PAGES_ROOT_KEYS, "pages file", pages_json_path)
    entries = require_list(data.get("pages", []), "'pages'", pages_json_path)
    for entry in entries:
        reject_unknown_keys(entry, allowed_entry_keys, "page entry", pages_json_path)
        for key in ("pageDefinition", "qml"):
            if key not in entry:
                continue
            value = entry[key]
            if not isinstance(value, str) or not value:
                raise ValueError(
                    f"{pages_json_path}: {key!r} must be a non-empty string, "
                    f"got: {value!r}"
                )
            # Reject both separators: '\\' is not a separator on POSIX but is on Windows
            if "/" in value or "\\" in value:
                raise ValueError(
                    f"{pages_json_path}: {key!r} must be a bare file name, "
                    f"got: {value!r}"
                )
    return entries


def _entry_index(entries: list[dict], name: str) -> int:
    for i, entry in enumerate(entries):
        if not entry.get("divider") and entry.get("name") == name:
            return i
    return -1


def _merge_overlay(entries: list[dict], overlay_entries: list[dict], overlay_path: Path) -> list[dict]:
    # Entries already inserted after each anchor, so repeated insertAfter keeps overlay order
    inserted_after: dict[str, list[dict]] = {}
    for raw in overlay_entries:
        if "remove" in raw:
            if set(raw) - {"remove", "comment"}:
                raise ValueError(
                    f"{overlay_path}: a 'remove' entry must not have other keys "
                    f"(entry: {clamped_repr(raw)})"
                )
            index = _entry_index(entries, raw["remove"])
            if index == -1:
                raise ValueError(
                    f"{overlay_path}: 'remove' references unknown page {raw['remove']!r}"
                )
            del entries[index]
            continue

        for key in _OVERLAY_POSITION_KEYS & set(raw):
            if not isinstance(raw[key], str) or not raw[key]:
                raise ValueError(
                    f"{overlay_path}: {key!r} must be a non-empty string "
                    f"(entry: {clamped_repr(raw)})"
                )
        if "insertAfter" in raw and "insertBefore" in raw:
            raise ValueError(
                f"{overlay_path}: 'insertAfter' and 'insertBefore' are mutually exclusive "
                f"(entry: {clamped_repr(raw)})"
            )
        insert_after = raw.get("insertAfter")
        insert_before = raw.get("insertBefore")

        entry = {k: v for k, v in raw.items() if k not in _OVERLAY_POSITION_KEYS}
        if not entry.get("divider"):
            if not entry.get("name"):
                raise ValueError(
                    f"{overlay_path}: page entry must have a 'name' "
                    f"(entry: {clamped_repr(raw)})"
                )
            existing = _entry_index(entries, entry["name"])
            if existing != -1:
                if insert_after or insert_before:
                    raise ValueError(
                        f"{overlay_path}: cannot combine a replace of existing page "
                        f"{entry['name']!r} with 'insertAfter'/'insertBefore'"
                    )
                entries[existing] = entry
                continue

        if insert_after or insert_before:
            anchor = str(insert_after or insert_before)
            index = _entry_index(entries, anchor)
            if index == -1:
                raise ValueError(
                    f"{overlay_path}: 'insertAfter'/'insertBefore' references unknown page "
                    f"{anchor!r}"
                )
            if insert_after:
                tail = inserted_after.setdefault(anchor, [])
                pos = index + 1
                while pos < len(entries) and any(entries[pos] is prior for prior in tail):
                    pos += 1
                entries.insert(pos, entry)
                tail.append(entry)
            else:
                entries.insert(index, entry)
        else:
            entries.append(entry)
    return entries


def load_pages_data(pages_json_path: Path, custom_pages_dir: Path | None = None) -> list[dict]:
    """Load the stock pages list and merge the custom-build overlay (if present).

    The overlay is `<custom_pages_dir>/SettingsPages.json`. Its entries may append,
    position (`insertAfter`/`insertBefore`), replace (same `name`), or `remove` pages.
    """
    entries = _load_pages_file(pages_json_path, _ALLOWED_PAGE_ENTRY_KEYS)
    error_source = pages_json_path
    if custom_pages_dir is not None:
        overlay_path = custom_pages_dir / "SettingsPages.json"
        if overlay_path.is_file():
            overlay_entries = _load_pages_file(overlay_path, _ALLOWED_OVERLAY_ENTRY_KEYS)
            entries = _merge_overlay(list(entries), overlay_entries, overlay_path)
            # Post-merge duplicates are most plausibly introduced by the overlay
            error_source = overlay_path

    seen_qml: dict[str, str] = {}
    for entry in entries:
        qml = entry.get("qml")
        if not qml:
            continue
        # casefold: macOS/Windows filesystems are case-insensitive, so Foo.qml/foo.qml collide
        key = qml.casefold()
        if key == "settingspagesmodel.qml":
            raise ValueError(
                f"{error_source}: page {entry.get('name')!r} uses reserved output "
                f"file name 'SettingsPagesModel.qml'"
            )
        if key in seen_qml:
            raise ValueError(
                f"{error_source}: pages {seen_qml[key]!r} and {entry.get('name')!r} "
                f"both output qml file {qml!r}"
            )
        seen_qml[key] = entry.get("name", "")
    return entries


def resolve_page_def_path(page_def_name: str, pages_dir: Path, custom_pages_dir: Path | None) -> Path:
    """Resolve a pageDefinition file: custom dir shadows the stock pages dir."""
    if custom_pages_dir is not None:
        candidate = custom_pages_dir / page_def_name
        if candidate.is_file():
            return candidate
    return pages_dir / page_def_name
