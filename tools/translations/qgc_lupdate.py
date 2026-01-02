#!/usr/bin/env python3
"""
Update Qt and JSON translation files for QGroundControl.

This script:
1. Runs Qt's lupdate tool to extract translatable strings from C++/QML
2. Extracts translatable strings from JSON metadata files

Usage:
    python tools/translations/qgc_lupdate.py           # Run both
    python tools/translations/qgc_lupdate.py --json-only    # JSON only
    python tools/translations/qgc_lupdate.py --lupdate-only # lupdate only
"""

from __future__ import annotations

import argparse
import codecs
import json
import os
import subprocess
import sys
from pathlib import Path

# JSON translation keys
QGC_FILE_TYPE_KEY = "fileType"
TRANSLATE_KEYS_KEY = "translateKeys"
ARRAY_ID_KEYS_KEY = "arrayIDKeys"
DISAMBIGUATION_PREFIX = "#loc.disambiguation#"


def get_repo_root() -> Path:
    """Find repository root directory."""
    current = Path(__file__).resolve()
    for parent in [current] + list(current.parents):
        if (parent / ".git").exists():
            return parent
    return Path.cwd()


def find_lupdate() -> Path | None:
    """Find Qt lupdate executable.

    Checks:
    1. QT_ROOT_DIR environment variable
    2. ~/Qt/6.x.x/*/bin/lupdate (sorted by version, newest first)
    """
    # Check QT_ROOT_DIR first (CI environment)
    qt_root = os.environ.get("QT_ROOT_DIR")
    if qt_root:
        lupdate = Path(qt_root) / "bin" / "lupdate"
        if lupdate.exists() and os.access(lupdate, os.X_OK):
            return lupdate

    # Search in ~/Qt for local development
    home = Path.home()
    qt_paths = sorted(
        home.glob("Qt/6.*/*/bin/lupdate"),
        key=lambda p: tuple(int(x) for x in p.parts[-4].split(".") if x.isdigit()),
        reverse=True,
    )

    for lupdate in qt_paths:
        if lupdate.exists() and os.access(lupdate, os.X_OK):
            return lupdate

    return None


def run_lupdate(lupdate: Path, src_dir: Path, output_file: Path) -> bool:
    """Run Qt lupdate to extract C++/QML strings."""
    print(f"Using lupdate: {lupdate}")

    try:
        result = subprocess.run(
            [str(lupdate), str(src_dir), "-ts", str(output_file), "-no-obsolete"],
            check=True,
            capture_output=True,
            text=True,
        )
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error: lupdate failed", file=sys.stderr)
        print(e.stderr, file=sys.stderr)
        return False


def escape_xml_string(xml_str: str) -> str:
    """Escape string for XML output."""
    xml_str = xml_str.replace("&", "&amp;")
    xml_str = xml_str.replace("<", "&lt;")
    xml_str = xml_str.replace(">", "&gt;")
    xml_str = xml_str.replace("'", "&apos;")
    xml_str = xml_str.replace('"', "&quot;")
    return xml_str


def parse_json_object_for_translate_keys(
    json_object_hierarchy: str,
    json_object: dict,
    translate_keys: list[str],
    array_id_keys: list[str],
    loc_string_dict: dict[str, list[str]],
) -> None:
    """Extract translatable strings from JSON object."""
    for translate_key in translate_keys:
        if translate_key in json_object:
            loc_str = json_object[translate_key]
            if not isinstance(loc_str, str):
                continue
            current_hierarchy = f"{json_object_hierarchy}.{translate_key}"
            if loc_str in loc_string_dict:
                loc_string_dict[loc_str].append(current_hierarchy)
            else:
                loc_string_dict[loc_str] = [current_hierarchy]

    for key in json_object:
        current_hierarchy = f"{json_object_hierarchy}.{key}"
        value = json_object[key]
        if isinstance(value, dict):
            parse_json_object_for_translate_keys(
                current_hierarchy, value, translate_keys, array_id_keys, loc_string_dict
            )
        elif isinstance(value, list):
            parse_json_array_for_translate_keys(
                current_hierarchy, value, translate_keys, array_id_keys, loc_string_dict
            )


def parse_json_array_for_translate_keys(
    json_object_hierarchy: str,
    json_array: list,
    translate_keys: list[str],
    array_id_keys: list[str],
    loc_string_dict: dict[str, list[str]],
) -> None:
    """Extract translatable strings from JSON array."""
    for index, json_object in enumerate(json_array):
        if not isinstance(json_object, dict):
            continue

        array_index_str = str(index)
        for array_id_key in array_id_keys:
            if array_id_key in json_object:
                array_index_str = str(json_object[array_id_key])
                break

        current_hierarchy = f"{json_object_hierarchy}[{array_index_str}]"
        parse_json_object_for_translate_keys(
            current_hierarchy, json_object, translate_keys, array_id_keys, loc_string_dict
        )


def add_loc_keys_based_on_qgc_file_type(json_path: str, json_dict: dict) -> None:
    """Add translation keys based on QGC file type."""
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


def parse_json_file(json_path: Path, loc_string_dict: dict[str, list[str]]) -> None:
    """Parse a single JSON file for translatable strings."""
    try:
        with open(json_path, "rb") as f:
            json_dict = json.load(f)
    except (json.JSONDecodeError, OSError):
        return

    if not isinstance(json_dict, dict):
        return

    add_loc_keys_based_on_qgc_file_type(str(json_path), json_dict)

    if TRANSLATE_KEYS_KEY not in json_dict:
        return

    translate_keys = json_dict[TRANSLATE_KEYS_KEY].split(",")
    array_id_keys = json_dict.get(ARRAY_ID_KEYS_KEY, "").split(",")
    array_id_keys = [k for k in array_id_keys if k]  # Filter empty

    parse_json_object_for_translate_keys(
        "", json_dict, translate_keys, array_id_keys, loc_string_dict
    )


def walk_directory_for_json_files(
    directory: Path,
    multi_file_loc_array: list[tuple[str, str, dict[str, list[str]]]],
) -> None:
    """Walk directory tree collecting JSON translation strings."""
    for path in directory.rglob("*.json"):
        if not path.is_file():
            continue

        single_file_loc_dict: dict[str, list[str]] = {}
        parse_json_file(path, single_file_loc_dict)

        if single_file_loc_dict:
            filename = path.name
            # Check for duplicate filenames
            for entry in multi_file_loc_array:
                if entry[0] == filename:
                    print(f"Error: Duplicate filenames: {filename}")
                    print(f"  Path 1: {entry[1]}")
                    print(f"  Path 2: {path}")
                    sys.exit(1)
            multi_file_loc_array.append((filename, str(path), single_file_loc_dict))


def generate_ts_content(
    multi_file_loc_array: list[tuple[str, str, dict[str, list[str]]]]
) -> str:
    """Generate Qt .ts file content from extracted strings."""
    lines = [
        '<?xml version="1.0" encoding="utf-8"?>',
        "<!DOCTYPE TS>",
        '<TS version="2.1">',
    ]

    for filename, filepath, loc_string_dict in multi_file_loc_array:
        lines.append("<context>")
        lines.append(f"    <name>{filename}</name>")

        for loc_str, hierarchies in loc_string_dict.items():
            original_loc_str = loc_str
            disambiguation = ""

            if loc_str.startswith(DISAMBIGUATION_PREFIX):
                work_str = loc_str[len(DISAMBIGUATION_PREFIX):]
                terminator_index = work_str.find("#")
                if terminator_index == -1:
                    print(f"Bad disambiguation in {filename}: '{loc_str}'")
                    sys.exit(1)
                disambiguation = work_str[:terminator_index]
                loc_str = work_str[terminator_index + 1:]

            lines.append("    <message>")

            if disambiguation:
                lines.append(f"        <comment>{disambiguation}</comment>")

            extra_comment = ", ".join(hierarchies) + ", "
            loc_str_escaped = escape_xml_string(loc_str)

            lines.append(f"        <extracomment>{extra_comment}</extracomment>")

            if extra_comment.endswith(".enumStrings, "):
                lines.append(
                    "        <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>"
                )

            lines.append(f'        <location filename="{filepath}"/>')
            lines.append(f"        <source>{loc_str_escaped}</source>")
            lines.append('        <translation type="unfinished"></translation>')
            lines.append("    </message>")

        lines.append("</context>")

    lines.append("</TS>")
    return "\n".join(lines) + "\n"


def write_json_ts_file(
    multi_file_loc_array: list[tuple[str, str, dict[str, list[str]]]],
    output_file: Path,
) -> None:
    """Write JSON translations to .ts file."""
    content = generate_ts_content(multi_file_loc_array)
    with codecs.open(str(output_file), "w", "utf-8") as f:
        f.write(content)


def extract_json_translations(src_dir: Path, output_file: Path) -> bool:
    """Extract translations from JSON files."""
    print("Extracting JSON strings...")

    multi_file_loc_array: list[tuple[str, str, dict[str, list[str]]]] = []
    walk_directory_for_json_files(src_dir, multi_file_loc_array)

    if not multi_file_loc_array:
        print("No JSON files with translatable strings found")
        return True

    write_json_ts_file(multi_file_loc_array, output_file)
    print(f"Wrote {len(multi_file_loc_array)} contexts to {output_file}")
    return True


def parse_args(args: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Update Qt and JSON translation files for QGroundControl.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                    # Run both lupdate and JSON extraction
  %(prog)s --json-only        # Only extract JSON strings
  %(prog)s --lupdate-only     # Only run Qt lupdate
  %(prog)s --src-dir ./src    # Custom source directory
""",
    )

    parser.add_argument(
        "--json-only",
        action="store_true",
        help="Only extract JSON translation strings",
    )
    parser.add_argument(
        "--lupdate-only",
        action="store_true",
        help="Only run Qt lupdate (C++/QML)",
    )
    parser.add_argument(
        "--src-dir",
        type=Path,
        default=None,
        help="Source directory (default: <repo>/src)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="Output directory (default: <repo>/translations)",
    )

    return parser.parse_args(args)


def main() -> int:
    """Main entry point."""
    args = parse_args()

    repo_root = get_repo_root()
    src_dir = args.src_dir or repo_root / "src"
    output_dir = args.output_dir or repo_root / "translations"

    if not src_dir.exists():
        print(f"Error: Source directory not found: {src_dir}", file=sys.stderr)
        return 1

    output_dir.mkdir(parents=True, exist_ok=True)

    success = True

    # Run lupdate unless --json-only
    if not args.json_only:
        lupdate = find_lupdate()
        if lupdate is None:
            print("Error: lupdate not found", file=sys.stderr)
            print("Set QT_ROOT_DIR or install Qt to ~/Qt/", file=sys.stderr)
            return 1

        qgc_ts = output_dir / "qgc.ts"
        if not run_lupdate(lupdate, src_dir, qgc_ts):
            success = False

    # Extract JSON translations unless --lupdate-only
    if not args.lupdate_only:
        json_ts = output_dir / "qgc-json.ts"
        if not extract_json_translations(src_dir, json_ts):
            success = False

    if success:
        print("Translation files updated")
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(main())
