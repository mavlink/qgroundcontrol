#!/usr/bin/env python3
"""Tests for tools/translations/qgc_lupdate_json.py."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

import pytest

from ._helpers import load_script_module

if TYPE_CHECKING:
    from pathlib import Path

mod = load_script_module("translations/qgc_lupdate_json.py", "qgc_lupdate_json")


def test_add_loc_keys_injects_factmetadata_defaults() -> None:
    d: dict[str, Any] = {"fileType": "FactMetaData"}
    mod.add_loc_keys_based_on_qgc_file_type(d)
    assert d["translateKeys"] == "shortDesc,longDesc,enumStrings,label,keywords"
    assert d["arrayIDKeys"] == "name"


def test_add_loc_keys_respects_existing_translate_keys() -> None:
    d: dict[str, Any] = {"fileType": "MavCmdInfo", "translateKeys": "custom"}
    mod.add_loc_keys_based_on_qgc_file_type(d)
    assert d["translateKeys"] == "custom"  # preserve explicit override
    assert d["arrayIDKeys"] == "rawName,comment"  # still injected


def test_add_loc_keys_unknown_filetype_noop() -> None:
    d: dict[str, Any] = {"fileType": "WhoKnows"}
    mod.add_loc_keys_based_on_qgc_file_type(d)
    assert "translateKeys" not in d
    assert "arrayIDKeys" not in d


def test_parse_json_object_extracts_top_level_string() -> None:
    loc: dict[str, list[str]] = {}
    mod.parse_json_object_for_translate_keys("", {"label": "Hello"}, ["label"], [], loc)
    assert loc == {"Hello": [".label"]}


def test_parse_json_object_extracts_list_values() -> None:
    loc: dict[str, list[str]] = {}
    mod.parse_json_object_for_translate_keys(
        "", {"keywords": ["foo", "bar"]}, ["keywords"], [], loc
    )
    assert loc == {"foo": [".keywords[0]"], "bar": [".keywords[1]"]}


def test_parse_json_object_skips_empty_strings() -> None:
    loc: dict[str, list[str]] = {}
    mod.parse_json_object_for_translate_keys("", {"label": ""}, ["label"], [], loc)
    assert loc == {}


def test_parse_json_object_recurses_into_nested_dict() -> None:
    loc: dict[str, list[str]] = {}
    mod.parse_json_object_for_translate_keys(
        "", {"section": {"label": "Nested"}}, ["label"], [], loc
    )
    assert loc == {"Nested": [".section.label"]}


def test_parse_json_array_uses_array_id_key_for_hierarchy() -> None:
    loc: dict[str, list[str]] = {}
    mod.parse_json_array_for_translate_keys(
        ".items",
        [{"name": "alpha", "label": "A"}, {"name": "beta", "label": "B"}],
        ["label"],
        ["name"],
        loc,
    )
    assert loc == {"A": [".items[alpha].label"], "B": [".items[beta].label"]}


def test_parse_json_array_falls_back_to_index_when_no_id_key() -> None:
    loc: dict[str, list[str]] = {}
    mod.parse_json_array_for_translate_keys(".items", [{"label": "A"}], ["label"], [], loc)
    assert loc == {"A": [".items[0].label"]}


def test_parse_json_object_aggregates_duplicate_loc_strings() -> None:
    """Same source string appearing twice should record both hierarchies."""
    loc: dict[str, list[str]] = {}
    mod.parse_json_object_for_translate_keys(
        "",
        {"label": "Speed", "section": {"label": "Speed"}},
        ["label"],
        [],
        loc,
    )
    assert loc == {"Speed": [".label", ".section.label"]}


def test_parse_json_skips_when_no_translate_keys(tmp_path: Path) -> None:
    p = tmp_path / "f.json"
    p.write_text('{"foo": "bar"}')
    loc: dict[str, list[str]] = {}
    mod.parse_json(p, loc)
    assert loc == {}


def test_parse_json_uses_filetype_defaults(tmp_path: Path) -> None:
    p = tmp_path / "f.json"
    p.write_text('{"fileType": "MavCmdInfo", "label": "Takeoff"}')
    loc: dict[str, list[str]] = {}
    mod.parse_json(p, loc)
    assert loc == {"Takeoff": [".label"]}


def test_walk_directory_exits_on_duplicate_filenames(tmp_path: Path) -> None:
    (tmp_path / "a").mkdir()
    (tmp_path / "b").mkdir()
    (tmp_path / "a" / "Dup.json").write_text('{"fileType": "MavCmdInfo", "label": "X"}')
    (tmp_path / "b" / "Dup.json").write_text('{"fileType": "MavCmdInfo", "label": "Y"}')
    with pytest.raises(SystemExit):
        mod.walk_directory_tree_for_json_files(tmp_path, [])


def test_split_disambiguation_no_marker() -> None:
    assert mod._split_disambiguation("plain", "ctx") == ("", "plain")


def test_split_disambiguation_extracts_comment() -> None:
    assert mod._split_disambiguation("#loc.disambiguation#foo context#actual", "ctx") == (
        "foo context",
        "actual",
    )


def test_split_disambiguation_malformed_exits() -> None:
    with pytest.raises(SystemExit):
        mod._split_disambiguation("#loc.disambiguation#missing-terminator", "ctx")
