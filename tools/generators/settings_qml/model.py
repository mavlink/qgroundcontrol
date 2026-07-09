"""Dataclasses + JSON loader for SettingsUI page definitions."""

from __future__ import annotations

import json
import re
from dataclasses import dataclass, field
from pathlib import Path  # noqa: TC003

from ..common.controls import (
    BaseControlDef,
    parse_button,
    parse_enable_checkbox,
)
from ..common.validation import clamped_repr, reject_unknown_keys, require_dict, require_list

# Matches C++ FactMetaData::splitTranslatedList: [,，、] (ASCII / fullwidth / enumeration commas).
_TRANSLATED_LIST_RE = re.compile("[,，、]")

# Fact-backed control settings: "settingsGroupAccessor.factName" (nested fact names allowed).
# ASCII-only, non-empty segments: fact_name feeds objectNames, which must stay grep-able.
_SETTING_RE = re.compile(r"[A-Za-z0-9_]+(\.[A-Za-z0-9_]+)+")


@dataclass
class ControlDef(BaseControlDef):
    """A single control referencing a setting."""
    placeholder: str = ""
    value: str = ""

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
    "placeholder", "value", "component",
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
            heading=grp_data.get("heading", ""),
            showWhen=grp_data.get("showWhen", ""),
            enableWhen=grp_data.get("enableWhen", ""),
            headingDescription=grp_data.get("headingDescription", ""),
            component=grp_data.get("component", ""),
            sectionName=grp_data.get("sectionName", ""),
            keywords=parse_keywords(grp_data.get("keywords", [])),
            missing=require_list(grp_data.get("missing", []), "group 'missing'", json_path),
        )
        for ctrl_data in require_list(grp_data.get("controls", []), "group 'controls'", json_path):
            reject_unknown_keys(ctrl_data, _ALLOWED_CONTROL_KEYS, "control", json_path)
            ctrl = ControlDef(
                setting=ctrl_data.get("setting", ""),
                label=ctrl_data.get("label", ""),
                control=ctrl_data.get("control", ""),
                showWhen=ctrl_data.get("showWhen", ""),
                enableWhen=ctrl_data.get("enableWhen", ""),
                placeholder=ctrl_data.get("placeholder", ""),
                value=ctrl_data.get("value", ""),
                component=ctrl_data.get("component", ""),
                enableCheckbox=parse_enable_checkbox(ctrl_data.get("enableCheckbox")),
                button=parse_button(ctrl_data.get("button")),
            )
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
