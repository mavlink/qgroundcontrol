"""Strict schema contracts for vehicle-config QML definitions."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

import pytest
from generators.config_qml.model import load_page_def

from ._helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path


def _write_page(tmp_path: Path, data: dict) -> Path:
    path = tmp_path / "Test.VehicleConfig.json"
    path.write_text(json.dumps(data), encoding="utf-8")
    return path


def _page() -> dict:
    return {
        "fileType": "VehicleConfig",
        "version": 1,
        "sections": [
            {"title": "General", "controls": [{"param": "PARAM_ONE", "control": "textfield"}]}
        ],
    }


def test_schema_rejects_unknown_keys_and_invalid_nested_types(tmp_path: Path) -> None:
    def set_value(data: dict, path: tuple[str | int, ...], value: object) -> None:
        target = data
        for key in path[:-1]:
            target = target[key]
        target[path[-1]] = value

    cases = [
        (("bogusRootKey",), True, "bogusRootKey"),
        (("sections", 0, "titel"), "typo", "titel"),
        (("sections", 0, "controls", 0, "sliderMinn"), 5, "sliderMinn"),
        (("sections", 0, "repeat"), {"paramPrefix": "BATT", "startIdx": 1}, "startIdx"),
        (("sections",), 42, "must be a JSON array"),
        (("sections", 0, "controls"), ["PARAM_ONE"], "must be a JSON object"),
        (("params",), ["PARAM_ONE"], "must be a JSON object"),
        (("params",), {"batteryCount": 42}, "batteryCount"),
        (("params",), {"batteryCount": {"required": True}}, "batteryCount"),
        (("sections", 0, "controls", 0, "dialogButton"), "oops", "dialogButton"),
        (("sections", 0, "controls", 0, "actionButton"), 42, "actionButton"),
        (("sections", 0, "controls", 0, "toggleCheckbox"), [1], "toggleCheckbox"),
        (("sections", 0, "controls", 0, "options"), ["not-an-object"], "options"),
        (("sections", 0, "controls", 0, "linkedParams"), ["not-an-object"], "linkedParams"),
    ]
    for path, value, error in cases:
        data = _page()
        set_value(data, path, value)
        with pytest.raises(ValueError, match=error):
            load_page_def(_write_page(tmp_path, data))


def test_comment_keys_are_accepted(tmp_path: Path) -> None:
    data = _page()
    data["comment"] = "root note"
    data["sections"][0]["comment"] = "section note"
    data["sections"][0]["controls"][0]["comment"] = "control note"
    assert len(load_page_def(_write_page(tmp_path, data)).sections[0].controls) == 1


def test_all_repository_vehicle_config_definitions_load() -> None:
    paths = sorted((REPO_ROOT / "src").rglob("*.VehicleConfig.json"))
    assert paths
    for path in paths:
        assert load_page_def(path).sections, path
