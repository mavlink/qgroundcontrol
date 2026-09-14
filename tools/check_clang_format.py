#!/usr/bin/env python3
"""Check C++ formatting without rewriting untouched regions of existing files."""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

from common.git import run_git


def comparison_base() -> str | None:
    """Use the PR merge base in CI, or HEAD for a local pre-commit invocation."""
    run_git("rev-parse", "--show-toplevel", check=True)
    if base := os.environ.get("PRE_COMMIT_FROM_REF"):
        head = os.environ.get("PRE_COMMIT_TO_REF", "HEAD")
        return run_git("merge-base", base, head, check=True).stdout.strip()
    result = run_git("rev-parse", "--verify", "HEAD")
    return result.stdout.strip() if result.returncode == 0 else None


def changed_ranges(path: Path, base: str | None) -> list[tuple[int, int]] | None:
    """None requests a whole-file check; an empty list means deletion-only changes."""
    if base is None:
        return None
    diff = run_git(
        "diff",
        "--no-ext-diff",
        "--no-textconv",
        "--no-color",
        "--no-renames",
        "--unified=0",
        base,
        "--",
        str(path),
        check=True,
    ).stdout
    if not diff or "\nBinary files " in diff:
        # Untracked files and unchanged files supplied by --all-files need a full check.
        return None
    ranges = []
    for match in re.finditer(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@", diff, re.MULTILINE):
        start = int(match[1])
        count = int(match[2]) if match[2] is not None else 1
        if count:
            ranges.append((start, start + count - 1))
    return ranges


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fix", action="store_true", help="Format the checked regions in place")
    parser.add_argument("files", nargs="+", type=Path)
    args = parser.parse_args(argv)
    failed = False
    try:
        base = comparison_base()
        for path in args.files:
            ranges = changed_ranges(path, base)
            if ranges == []:
                continue
            command = (
                ["clang-format", "-i"] if args.fix else ["clang-format", "--dry-run", "--Werror"]
            )
            command.extend(f"--lines={start}:{end}" for start, end in ranges or [])
            result = subprocess.run([*command, "--", str(path)], check=False)
            failed |= result.returncode != 0
    except (OSError, subprocess.CalledProcessError) as error:
        print(f"clang-format check failed: {error}", file=sys.stderr)
        return 1
    return int(failed)


if __name__ == "__main__":
    sys.exit(main())
