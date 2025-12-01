#!/usr/bin/env python3
"""
QGroundControl JSON Translation Extractor

Extracts translatable strings from JSON files and generates a Qt .ts file.
This works alongside lupdate which handles C++/QML strings.

Usage:
    python qgc-lupdate-json.py                    # Process src/ from repo root
    python qgc-lupdate-json.py path/to/src        # Process specific directory
    python qgc-lupdate-json.py -o custom.ts src/  # Custom output file
"""

import argparse
import codecs
import json
import os
import sys
from pathlib import Path

# JSON keys that indicate translation configuration
QGC_FILE_TYPE_KEY = "fileType"
TRANSLATE_KEYS_KEY = "translateKeys"
ARRAY_ID_KEYS_KEY = "arrayIDKeys"
DISAMBIGUATION_PREFIX = "#loc.disambiguation#"


def parse_json_object_for_translate_keys(
    json_object_hierarchy, json_object, translate_keys, array_id_keys, loc_string_dict
):
    """Recursively parse a JSON object for translatable strings."""
    for translate_key in translate_keys:
        if translate_key in json_object:
            loc_str = json_object[translate_key]
            current_hierarchy = json_object_hierarchy + "." + translate_key
            if loc_str in loc_string_dict:
                # Duplicate of an existing string
                loc_string_dict[loc_str].append(current_hierarchy)
            else:
                # First time we are seeing this string
                loc_string_dict[loc_str] = [current_hierarchy]

    for key in json_object:
        current_hierarchy = json_object_hierarchy + "." + key
        if isinstance(json_object[key], dict):
            parse_json_object_for_translate_keys(
                current_hierarchy,
                json_object[key],
                translate_keys,
                array_id_keys,
                loc_string_dict,
            )
        elif isinstance(json_object[key], list):
            parse_json_array_for_translate_keys(
                current_hierarchy,
                json_object[key],
                translate_keys,
                array_id_keys,
                loc_string_dict,
            )


def parse_json_array_for_translate_keys(
    json_object_hierarchy, json_array, translate_keys, array_id_keys, loc_string_dict
):
    """Recursively parse a JSON array for translatable strings."""
    for index, json_object in enumerate(json_array):
        array_index_str = str(index)
        for array_id_key in array_id_keys:
            if array_id_key in json_object.keys():
                array_index_str = json_object[array_id_key]
                break
        current_hierarchy = json_object_hierarchy + "[" + array_index_str + "]"
        parse_json_object_for_translate_keys(
            current_hierarchy,
            json_object,
            translate_keys,
            array_id_keys,
            loc_string_dict,
        )


def add_loc_keys_based_on_qgc_file_type(json_path, json_dict):
    """Add translation keys automatically based on QGC file type."""
    if QGC_FILE_TYPE_KEY not in json_dict:
        return

    qgc_file_type = json_dict[QGC_FILE_TYPE_KEY]
    translate_key_value = ""
    array_id_keys_value = ""

    if qgc_file_type == "MavCmdInfo":
        translate_key_value = "label,enumStrings,friendlyName,description,category"
        array_id_keys_value = "rawName,comment"
    elif qgc_file_type == "FactMetaData":
        translate_key_value = "shortDesc,longDesc,enumStrings"
        array_id_keys_value = "name"

    if TRANSLATE_KEYS_KEY not in json_dict and translate_key_value:
        json_dict[TRANSLATE_KEYS_KEY] = translate_key_value
    if ARRAY_ID_KEYS_KEY not in json_dict and array_id_keys_value:
        json_dict[ARRAY_ID_KEYS_KEY] = array_id_keys_value


def parse_json(json_path, loc_string_dict):
    """Parse a single JSON file for translatable strings."""
    try:
        with open(json_path, "rb") as json_file:
            json_dict = json.load(json_file)
    except (json.JSONDecodeError, IOError) as e:
        print(f"Warning: Could not parse {json_path}: {e}", file=sys.stderr)
        return

    if not isinstance(json_dict, dict):
        return

    add_loc_keys_based_on_qgc_file_type(json_path, json_dict)

    if TRANSLATE_KEYS_KEY not in json_dict:
        return

    translate_keys = json_dict[TRANSLATE_KEYS_KEY].split(",")
    array_id_keys = json_dict.get(ARRAY_ID_KEYS_KEY, "").split(",")
    parse_json_object_for_translate_keys(
        "", json_dict, translate_keys, array_id_keys, loc_string_dict
    )


def walk_directory_for_json_files(directory, multi_file_loc_array):
    """Recursively walk directory tree for JSON files with translatable strings."""
    for root, _, files in os.walk(directory):
        for filename in files:
            if not filename.endswith(".json"):
                continue

            path = os.path.join(root, filename)
            single_file_loc_dict = {}
            parse_json(path, single_file_loc_dict)

            if not single_file_loc_dict:
                continue

            # Check for duplicate file names
            for entry in multi_file_loc_array:
                if entry[0] == filename:
                    print(
                        f"Error: Duplicate filenames: {filename} paths: {path} {entry[1]}",
                        file=sys.stderr,
                    )
                    sys.exit(1)

            multi_file_loc_array.append([filename, path, single_file_loc_dict])


def escape_xml_string(xml_str):
    """Escape a string for use in XML."""
    xml_str = xml_str.replace("&", "&amp;")
    xml_str = xml_str.replace("<", "&lt;")
    xml_str = xml_str.replace(">", "&gt;")
    xml_str = xml_str.replace("'", "&apos;")
    xml_str = xml_str.replace('"', "&quot;")
    return xml_str


def write_json_ts_file(multi_file_loc_array, output_path):
    """Write the collected translations to a Qt .ts file."""
    with codecs.open(output_path, "w", "utf-8") as ts_file:
        ts_file.write('<?xml version="1.0" encoding="utf-8"?>\n')
        ts_file.write("<!DOCTYPE TS>\n")
        ts_file.write('<TS version="2.1">\n')

        for entry in multi_file_loc_array:
            ts_file.write("<context>\n")
            ts_file.write(f"    <name>{entry[0]}</name>\n")
            single_file_loc_dict = entry[2]

            for loc_str in single_file_loc_dict.keys():
                disambiguation = ""
                display_str = loc_str

                if loc_str.startswith(DISAMBIGUATION_PREFIX):
                    work_str = loc_str[len(DISAMBIGUATION_PREFIX) :]
                    terminator_index = work_str.find("#")
                    if terminator_index == -1:
                        print(
                            f"Bad disambiguation {entry[0]} '{loc_str}'",
                            file=sys.stderr,
                        )
                        sys.exit(1)
                    disambiguation = work_str[:terminator_index]
                    display_str = work_str[terminator_index + 1 :]

                ts_file.write("    <message>\n")
                if disambiguation:
                    ts_file.write(f"        <comment>{disambiguation}</comment>\n")

                extra_comment = ", ".join(single_file_loc_dict[loc_str]) + ", "
                escaped_str = escape_xml_string(display_str)

                ts_file.write(f"        <extracomment>{extra_comment}</extracomment>\n")
                if extra_comment.endswith(".enumStrings, "):
                    ts_file.write(
                        "        <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>\n"
                    )
                ts_file.write(f'        <location filename="{entry[1]}"/>\n')
                ts_file.write(f"        <source>{escaped_str}</source>\n")
                ts_file.write('        <translation type="unfinished"></translation>\n')
                ts_file.write("    </message>\n")

            ts_file.write("</context>\n")

        ts_file.write("</TS>\n")


def find_repo_root():
    """Find the repository root by looking for COPYING.md."""
    current = Path(__file__).resolve().parent
    while current != current.parent:
        if (current / "COPYING.md").exists():
            return current
        current = current.parent
    return None


def main():
    parser = argparse.ArgumentParser(
        description="Extract translatable strings from JSON files to Qt .ts format.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "source_dir",
        nargs="?",
        default=None,
        help="Source directory to scan (default: src/ from repo root)",
    )
    parser.add_argument(
        "-o",
        "--output",
        default=None,
        help="Output .ts file (default: translations/qgc-json.ts)",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Show files being processed",
    )

    args = parser.parse_args()

    # Find repo root
    repo_root = find_repo_root()
    if not repo_root:
        print("Error: Could not find repository root (COPYING.md)", file=sys.stderr)
        sys.exit(1)

    # Determine source directory
    if args.source_dir:
        source_dir = Path(args.source_dir)
        if not source_dir.is_absolute():
            source_dir = repo_root / source_dir
    else:
        source_dir = repo_root / "src"

    if not source_dir.exists():
        print(f"Error: Source directory not found: {source_dir}", file=sys.stderr)
        sys.exit(1)

    # Determine output file
    if args.output:
        output_path = Path(args.output)
        if not output_path.is_absolute():
            output_path = repo_root / output_path
    else:
        output_path = repo_root / "translations" / "qgc-json.ts"

    # Ensure output directory exists
    output_path.parent.mkdir(parents=True, exist_ok=True)

    if args.verbose:
        print(f"Scanning: {source_dir}")
        print(f"Output: {output_path}")

    # Process files
    multi_file_loc_array = []
    walk_directory_for_json_files(str(source_dir), multi_file_loc_array)

    if args.verbose:
        print(f"Found {len(multi_file_loc_array)} JSON files with translatable strings")

    write_json_ts_file(multi_file_loc_array, str(output_path))
    print(f"Generated {output_path}")


if __name__ == "__main__":
    main()
