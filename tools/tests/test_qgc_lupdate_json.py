#!/usr/bin/env python3
"""JSON translation extraction contracts."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

import pytest

from ._helpers import load_script_module

if TYPE_CHECKING:
    from pathlib import Path

mod = load_script_module("translations/qgc_lupdate_json.py", "qgc_lupdate_json")


def test_file_type_defaults_preserve_explicit_overrides() -> None:
    fact: dict[str, Any] = {"fileType": "FactMetaData"}
    mod.add_loc_keys_based_on_qgc_file_type(fact)
    assert fact["translateKeys"] == "shortDesc,longDesc,enumStrings,label,keywords"
    assert fact["arrayIDKeys"] == "name"

    mav: dict[str, Any] = {"fileType": "MavCmdInfo", "translateKeys": "custom"}
    mod.add_loc_keys_based_on_qgc_file_type(mav)
    assert mav == {
        "fileType": "MavCmdInfo",
        "translateKeys": "custom",
        "arrayIDKeys": "rawName,comment",
    }

    unknown: dict[str, Any] = {"fileType": "WhoKnows"}
    mod.add_loc_keys_based_on_qgc_file_type(unknown)
    assert unknown == {"fileType": "WhoKnows"}


def test_recursive_extraction_tracks_lists_ids_duplicates_and_empty_values() -> None:
    localized: dict[str, list[str]] = {}
    mod.parse_json_object_for_translate_keys(
        "",
        {
            "label": "Speed",
            "empty": "",
            "keywords": ["foo", "bar"],
            "section": {"label": "Speed"},
            "items": [{"name": "alpha", "label": "A"}, {"name": "beta", "label": "B"}],
        },
        ["label", "empty", "keywords"],
        ["name"],
        localized,
    )
    assert localized == {
        "Speed": [".label", ".section.label"],
        "foo": [".keywords[0]"],
        "bar": [".keywords[1]"],
        "A": [".items[alpha].label"],
        "B": [".items[beta].label"],
    }

    indexed: dict[str, list[str]] = {}
    mod.parse_json_array_for_translate_keys(".items", [{"label": "A"}], ["label"], [], indexed)
    assert indexed == {"A": [".items[0].label"]}


def test_parse_json_uses_defaults_and_skips_unmarked_files(tmp_path: Path) -> None:
    localized: dict[str, list[str]] = {}
    plain = tmp_path / "plain.json"
    plain.write_text('{"foo": "bar"}')
    mod.parse_json(plain, localized)
    assert localized == {}

    mav = tmp_path / "mav.json"
    mav.write_text('{"fileType": "MavCmdInfo", "label": "Takeoff"}')
    mod.parse_json(mav, localized)
    assert localized == {"Takeoff": [".label"]}


def test_directory_walk_rejects_duplicate_json_filenames(tmp_path: Path) -> None:
    for directory, label in (("a", "X"), ("b", "Y")):
        path = tmp_path / directory
        path.mkdir()
        (path / "Dup.json").write_text(f'{{"fileType": "MavCmdInfo", "label": "{label}"}}')
    with pytest.raises(SystemExit):
        mod.walk_directory_tree_for_json_files(tmp_path, [])


def test_disambiguation_parsing() -> None:
    assert mod._split_disambiguation("plain", "ctx") == ("", "plain")
    assert mod._split_disambiguation("#loc.disambiguation#foo context#actual", "ctx") == (
        "foo context",
        "actual",
    )
    with pytest.raises(SystemExit):
        mod._split_disambiguation("#loc.disambiguation#missing-terminator", "ctx")
