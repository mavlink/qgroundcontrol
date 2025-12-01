#!/usr/bin/env python3
"""
QGroundControl License Header Updater

Updates or validates license headers in source files.

Usage:
    python HeaderUpdater.py                    # Update headers in src/
    python HeaderUpdater.py --check            # Check headers without modifying (CI mode)
    python HeaderUpdater.py --check src/ test/ # Check specific directories
    python HeaderUpdater.py src/ test/         # Update specific directories
"""

import argparse
import datetime
import os
import re
import sys
from pathlib import Path

# File extensions to process
TARGET_EXTENSIONS = {".cpp", ".cc", ".h", ".qml"}

# License header template
HEADER_TEMPLATE = """/****************************************************************************
 *
 * (c) 2009-{year} QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/
"""

# Pattern to match QGC license header (with flexible year format)
HEADER_PATTERN = re.compile(r"\(c\)\s*(2009-\d{4}|\d{4})\s+QGROUNDCONTROL")

# Directories to skip
SKIP_DIRS = {
    "build",
    "libs",
    "deploy",
    ".git",
    "node_modules",
    "__pycache__",
    "cpm_modules",
}


def find_source_files(directories: list[str]) -> list[Path]:
    """Find all source files in the given directories."""
    files = []
    for directory in directories:
        path = Path(directory)
        if not path.exists():
            print(f"Warning: Directory not found: {directory}", file=sys.stderr)
            continue

        for root, dirs, filenames in os.walk(path):
            # Skip certain directories
            dirs[:] = [d for d in dirs if d not in SKIP_DIRS]

            for filename in filenames:
                if any(filename.endswith(ext) for ext in TARGET_EXTENSIONS):
                    files.append(Path(root) / filename)

    return sorted(files)


def check_header(file_path: Path) -> tuple[bool, str]:
    """
    Check if a file has a valid QGC license header.

    Returns:
        (is_valid, message)
    """
    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        return False, f"Error reading file: {e}"

    # Check if header exists
    match = HEADER_PATTERN.search(content[:1000])  # Only check first 1000 chars

    if not match:
        return False, "Missing QGC license header"

    # Check if year is current
    current_year = datetime.datetime.now().year
    year_match = match.group(1)

    if year_match == str(current_year):
        return True, "Header OK (single year)"
    elif year_match == f"2009-{current_year}":
        return True, "Header OK"
    elif year_match.startswith("2009-"):
        return (
            False,
            f"Header year outdated: {year_match} (should be 2009-{current_year})",
        )
    else:
        return False, f"Header year format incorrect: {year_match}"


def update_header(file_path: Path, current_year: int) -> tuple[bool, str]:
    """
    Update the license header in a file.

    Returns:
        (was_modified, message)
    """
    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        return False, f"Error reading file: {e}"

    original_content = content

    # Check if header exists
    match = HEADER_PATTERN.search(content)

    if match:
        # Update existing header
        content = HEADER_PATTERN.sub(f"(c) 2009-{current_year} QGROUNDCONTROL", content)
    else:
        # Prepend new header
        new_header = HEADER_TEMPLATE.format(year=current_year)
        content = new_header + "\n" + content

    if content != original_content:
        try:
            file_path.write_text(content, encoding="utf-8")
            return True, "Header updated"
        except Exception as e:
            return False, f"Error writing file: {e}"

    return False, "Header already up to date"


def main():
    parser = argparse.ArgumentParser(
        description="Update or check QGC license headers in source files.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "directories",
        nargs="*",
        default=["src"],
        help="Directories to process (default: src)",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Check headers without modifying files (exit 1 if issues found)",
    )
    parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        help="Show all files, not just those with issues",
    )

    args = parser.parse_args()

    # Find repo root (look for COPYING.md)
    script_dir = Path(__file__).parent
    repo_root = script_dir.parent

    if not (repo_root / "COPYING.md").exists():
        print("Warning: COPYING.md not found in repo root", file=sys.stderr)

    # Resolve directories relative to repo root
    directories = [
        str(repo_root / d) if not os.path.isabs(d) else d for d in args.directories
    ]

    files = find_source_files(directories)
    current_year = datetime.datetime.now().year

    if not files:
        print("No source files found.")
        return 0

    print(f"Processing {len(files)} files...")

    issues = []
    updated = []

    for file_path in files:
        rel_path = (
            file_path.relative_to(repo_root)
            if file_path.is_relative_to(repo_root)
            else file_path
        )

        if args.check:
            is_valid, message = check_header(file_path)
            if not is_valid:
                issues.append((rel_path, message))
                print(f"  FAIL: {rel_path}: {message}")
            elif args.verbose:
                print(f"  OK: {rel_path}")
        else:
            was_modified, message = update_header(file_path, current_year)
            if was_modified:
                updated.append(rel_path)
                print(f"  Updated: {rel_path}")
            elif args.verbose:
                print(f"  Skipped: {rel_path}: {message}")

    print()

    if args.check:
        if issues:
            print(f"Found {len(issues)} file(s) with header issues.")
            return 1
        else:
            print("All headers are valid.")
            return 0
    else:
        if updated:
            print(f"Updated {len(updated)} file(s).")
        else:
            print("No files needed updating.")
        return 0


if __name__ == "__main__":
    sys.exit(main())
