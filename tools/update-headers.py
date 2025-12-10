#!/usr/bin/env python3
"""
Update or validate license headers in QGroundControl source files.

Usage:
    ./HeaderUpdater.py                    # Update headers in src/
    ./HeaderUpdater.py --check            # Check only, don't modify (for CI)
    ./HeaderUpdater.py --check src/       # Check specific directory
    ./HeaderUpdater.py src/Vehicle/       # Update specific directory
    ./HeaderUpdater.py -v                 # Verbose output

Exit codes:
    0 - All files have correct headers (or headers updated)
    1 - Files need header updates (--check mode)
"""

import argparse
import datetime
import re
import sys
from pathlib import Path

# File extensions to process
TARGET_EXTENSIONS = {".cpp", ".cc", ".h", ".hpp", ".qml"}

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

# Pattern to match existing QGC header
HEADER_PATTERN = re.compile(r"\(c\)\s*(2009-\d{4}|\d{4})\s*QGROUNDCONTROL", re.IGNORECASE)


def check_header(content: str, current_year: int) -> tuple[bool, str]:
    """
    Check if file has correct header.

    Returns:
        (is_correct, reason)
    """
    match = HEADER_PATTERN.search(content)

    if not match:
        return False, "missing header"

    year_str = match.group(1)
    if year_str.startswith("2009-"):
        header_year = int(year_str.split("-")[1])
    else:
        header_year = int(year_str)

    if header_year != current_year:
        return False, f"outdated year ({header_year} -> {current_year})"

    return True, "ok"


def update_header(content: str, current_year: int) -> str:
    """Update header in file content."""
    match = HEADER_PATTERN.search(content)

    if match:
        # Update existing header
        return HEADER_PATTERN.sub(f"(c) 2009-{current_year} QGROUNDCONTROL", content)
    else:
        # Prepend new header
        new_header = HEADER_TEMPLATE.format(year=current_year)
        return new_header + "\n" + content


def find_source_files(directory: Path) -> list[Path]:
    """Find all source files in directory."""
    files = []
    for ext in TARGET_EXTENSIONS:
        files.extend(directory.rglob(f"*{ext}"))
    return sorted(files)


def main():
    parser = argparse.ArgumentParser(
        description="Update or validate license headers in source files",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "directory",
        nargs="?",
        type=Path,
        help="Directory to process (default: src/)",
    )
    parser.add_argument(
        "-c",
        "--check",
        action="store_true",
        help="Check only, don't modify files (for CI)",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Show all files, not just those needing updates",
    )
    parser.add_argument(
        "-q",
        "--quiet",
        action="store_true",
        help="Only show summary",
    )

    args = parser.parse_args()

    # Find repo root
    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent

    # Determine directory to process
    if args.directory:
        if args.directory.is_absolute():
            target_dir = args.directory
        else:
            target_dir = repo_root / args.directory
    else:
        target_dir = repo_root / "src"

    if not target_dir.exists():
        print(f"Error: Directory not found: {target_dir}", file=sys.stderr)
        sys.exit(1)

    current_year = datetime.datetime.now().year
    files = find_source_files(target_dir)

    if not files:
        print(f"No source files found in {target_dir}", file=sys.stderr)
        sys.exit(0)

    needs_update = []
    updated = []
    errors = []

    for filepath in files:
        try:
            content = filepath.read_text(encoding="utf-8", errors="replace")
        except Exception as e:
            errors.append((filepath, str(e)))
            continue

        is_correct, reason = check_header(content, current_year)

        if is_correct:
            if args.verbose and not args.quiet:
                print(f"  ✓ {filepath.relative_to(repo_root)}")
        else:
            rel_path = filepath.relative_to(repo_root)

            if args.check:
                needs_update.append((rel_path, reason))
                if not args.quiet:
                    print(f"  ✗ {rel_path}: {reason}")
            else:
                # Update the file
                try:
                    new_content = update_header(content, current_year)
                    filepath.write_text(new_content, encoding="utf-8")
                    updated.append(rel_path)
                    if not args.quiet:
                        print(f"  ✓ {rel_path}: updated ({reason})")
                except Exception as e:
                    errors.append((filepath, str(e)))

    # Summary
    print()
    if args.check:
        print(f"Checked {len(files)} files")
        if needs_update:
            print(f"  {len(needs_update)} files need header updates:")
            for path, reason in needs_update[:10]:
                print(f"    - {path}: {reason}")
            if len(needs_update) > 10:
                print(f"    ... and {len(needs_update) - 10} more")
            print()
            print("Run without --check to update headers")
            sys.exit(1)
        else:
            print("  All headers are correct")
    else:
        print(f"Processed {len(files)} files")
        if updated:
            print(f"  {len(updated)} files updated")
        else:
            print("  No updates needed")

    if errors:
        print(f"  {len(errors)} errors:", file=sys.stderr)
        for path, err in errors:
            print(f"    - {path}: {err}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
