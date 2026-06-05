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

# Matches C++ FactMetaData::splitTranslatedList: [,，、] (ASCII / fullwidth / enumeration commas).
_TRANSLATED_LIST_RE = re.compile("[,，、]")


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


def load_page_def(json_path: Path) -> PageDef:
    """Load a page definition from a JSON file."""
    with open(json_path, encoding="utf-8") as f:
        data = json.load(f)

    page = PageDef(imports=data.get("imports", []), bindings=data.get("bindings", {}))
    for grp_data in data.get("groups", []):
        grp = GroupDef(
            heading=grp_data.get("heading", ""),
            showWhen=grp_data.get("showWhen", ""),
            enableWhen=grp_data.get("enableWhen", ""),
            headingDescription=grp_data.get("headingDescription", ""),
            component=grp_data.get("component", ""),
            sectionName=grp_data.get("sectionName", ""),
            keywords=parse_keywords(grp_data.get("keywords", [])),
            missing=grp_data.get("missing", []),
        )
        for ctrl_data in grp_data.get("controls", []):
            grp.controls.append(ControlDef(
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
            ))
        page.groups.append(grp)
    return page
