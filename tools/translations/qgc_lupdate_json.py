#!/usr/bin/env python3
"""Extract translatable strings from QGC's JSON metadata files into a Qt .ts file."""

from __future__ import annotations

import json
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

QGC_FILE_TYPE_KEY = "fileType"
TRANSLATE_KEYS_KEY = "translateKeys"
ARRAY_ID_KEYS_KEY = "arrayIDKeys"
DISAMBIGUATION_PREFIX = "#loc.disambiguation#"

FILE_TYPE_DEFAULTS: dict[str, tuple[str, str]] = {
    "MavCmdInfo": ("label,enumStrings,friendlyName,description,category", "rawName,comment"),
    "FactMetaData": ("shortDesc,longDesc,enumStrings,label,keywords", "name"),
    "VehicleConfig": ("title,label,text,heading,keywords", "title"),
    "SettingsUI": ("heading,sectionName,label,text,placeholder,keywords", "heading,sectionName"),
    "SettingsPages": ("name", "name"),
}

COMMA_SEPARATED_FIELDS = {"enumStrings", "keywords"}

def _record_loc(loc_dict: dict[str, list[str]], loc_str: str, hierarchy: str) -> None:
    loc_dict.setdefault(loc_str, []).append(hierarchy)

def parse_json_object_for_translate_keys(
    hierarchy: str,
    json_object: dict,
    translate_keys: list[str],
    array_id_keys: list[str],
    loc_dict: dict[str, list[str]],
) -> None:
    for translate_key in translate_keys:
        if translate_key not in json_object:
            continue
        value = json_object[translate_key]
        if isinstance(value, list):
            for idx, loc_str in enumerate(value):
                if isinstance(loc_str, str) and loc_str:
                    _record_loc(loc_dict, loc_str, f"{hierarchy}.{translate_key}[{idx}]")
        elif isinstance(value, str) and value:
            _record_loc(loc_dict, value, f"{hierarchy}.{translate_key}")

    for key, value in json_object.items():
        child_hierarchy = f"{hierarchy}.{key}"
        if isinstance(value, dict):
            parse_json_object_for_translate_keys(
                child_hierarchy, value, translate_keys, array_id_keys, loc_dict
            )
        elif isinstance(value, list):
            parse_json_array_for_translate_keys(
                child_hierarchy, value, translate_keys, array_id_keys, loc_dict
            )

def parse_json_array_for_translate_keys(
    hierarchy: str,
    json_array: list,
    translate_keys: list[str],
    array_id_keys: list[str],
    loc_dict: dict[str, list[str]],
) -> None:
    for index, json_object in enumerate(json_array):
        if not isinstance(json_object, dict):
            continue
        array_index_str = str(index)
        for array_id_key in array_id_keys:
            if array_id_key in json_object:
                array_index_str = json_object[array_id_key]
                break
        parse_json_object_for_translate_keys(
            f"{hierarchy}[{array_index_str}]",
            json_object,
            translate_keys,
            array_id_keys,
            loc_dict,
        )

def add_loc_keys_based_on_qgc_file_type(json_dict: dict) -> None:
    """Inject translateKeys/arrayIDKeys defaults based on fileType, if not set."""
    file_type = json_dict.get(QGC_FILE_TYPE_KEY)
    if file_type not in FILE_TYPE_DEFAULTS:
        return
    translate_default, array_id_default = FILE_TYPE_DEFAULTS[file_type]
    if TRANSLATE_KEYS_KEY not in json_dict and translate_default:
        json_dict[TRANSLATE_KEYS_KEY] = translate_default
    if ARRAY_ID_KEYS_KEY not in json_dict and array_id_default:
        json_dict[ARRAY_ID_KEYS_KEY] = array_id_default

def parse_json(json_path: Path, loc_dict: dict[str, list[str]]) -> None:
    with open(json_path, "rb") as fh:
        json_dict = json.load(fh)
    if not isinstance(json_dict, dict):
        return
    add_loc_keys_based_on_qgc_file_type(json_dict)
    if TRANSLATE_KEYS_KEY not in json_dict:
        return
    translate_keys = json_dict[TRANSLATE_KEYS_KEY].split(",")
    array_id_keys = json_dict.get(ARRAY_ID_KEYS_KEY, "").split(",")
    parse_json_object_for_translate_keys("", json_dict, translate_keys, array_id_keys, loc_dict)

def walk_directory_tree_for_json_files(
    directory: Path, multi_file_loc_array: list[list]
) -> None:
    for path in directory.iterdir():
        if path.is_file() and path.suffix == ".json":
            single_file_loc_dict: dict[str, list[str]] = {}
            parse_json(path, single_file_loc_dict)
            if single_file_loc_dict:
                for entry in multi_file_loc_array:
                    if entry[0] == path.name:
                        print(f"Error: Duplicate filenames: {path.name} paths: {path} {entry[1]}")
                        sys.exit(1)
                multi_file_loc_array.append([path.name, str(path), single_file_loc_dict])
        elif path.is_dir():
            walk_directory_tree_for_json_files(path, multi_file_loc_array)

def _split_disambiguation(source_str: str, context_name: str) -> tuple[str, str]:
    """Return (disambiguation, source). Raises SystemExit if marker is malformed."""
    if not source_str.startswith(DISAMBIGUATION_PREFIX):
        return "", source_str
    work_str = source_str[len(DISAMBIGUATION_PREFIX):]
    terminator = work_str.find("#")
    if terminator == -1:
        print(f"Bad disambiguation {context_name} '{source_str}'")
        sys.exit(1)
    return work_str[:terminator], work_str[terminator + 1:]

def write_json_ts_file(output_path: Path, multi_file_loc_array: list[list]) -> None:
    ts_root = ET.Element("TS", version="2.1")

    for context_name, file_path, single_file_loc_dict in multi_file_loc_array:
        context = ET.SubElement(ts_root, "context")
        ET.SubElement(context, "name").text = context_name

        for loc_key in single_file_loc_dict:
            disambiguation, source_str = _split_disambiguation(loc_key, context_name)
            message = ET.SubElement(context, "message")
            if disambiguation:
                ET.SubElement(message, "comment").text = disambiguation

            hierarchies = single_file_loc_dict[loc_key]
            ET.SubElement(message, "extracomment").text = ", ".join(hierarchies)

            if any(h.rsplit(".", 1)[-1] in COMMA_SEPARATED_FIELDS for h in hierarchies):
                ET.SubElement(
                    message, "translatorcomment"
                ).text = "Only use english comma ',' to separate strings"

            ET.SubElement(message, "location", filename=file_path)
            ET.SubElement(message, "source").text = source_str
            ET.SubElement(message, "translation", type="unfinished")

    ET.indent(ts_root, space="    ")
    with open(output_path, "w", encoding="utf-8") as fh:
        fh.write('<?xml version="1.0" encoding="utf-8"?>\n')
        fh.write("<!DOCTYPE TS>\n")
        fh.write(ET.tostring(ts_root, encoding="unicode"))
        fh.write("\n")

def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    multi_file_loc_array: list[list] = []
    walk_directory_tree_for_json_files(repo_root / "src", multi_file_loc_array)
    write_json_ts_file(repo_root / "translations" / "qgc-json.ts", multi_file_loc_array)
    return 0

if __name__ == "__main__":
    sys.exit(main())
