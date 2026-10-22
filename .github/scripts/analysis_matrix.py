#!/usr/bin/env python3
"""Emit the analysis.yml job matrix as a JSON 'include' list.

Every clang-tidy shard repeats several minutes of configure and code
generation before it analyzes its slice, so pull requests get only as many
shards as their estimated compilation units need. Changed headers are
expanded through a textual reverse-include scan of src/, which may overcount
but does not miss ordinary includes. Inputs that can select a full scan keep
every shard. tools/analyze.py still selects the exact units after configure.
"""

from __future__ import annotations

import argparse
import json
import math
import os
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import write_github_output

Leg = dict[str, str | int]

SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx"})
HEADER_SUFFIXES = frozenset({".h", ".hh", ".hpp", ".hxx", ".inl"})
MAX_TIDY_SHARDS = 4
UNITS_PER_SHARD = 50
# analysis.yml pull_request paths outside src/: changes here may select a full scan.
FULL_SCAN_INPUTS = (
    "cmake/",
    "tools/",
    "CMakeLists.txt",
    "CMakePresets.json",
    ".clang-tidy",
    ".qmllint.ini",
    ".github/build-config.json",
    ".github/scripts/analysis_matrix.py",
    ".github/workflows/analysis.yml",
)
INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)


def changed_files(root: Path, base: str) -> list[str]:
    """Return paths changed since the merge base, as analysis.yml computes them."""
    result = subprocess.run(
        ["git", "diff", "--name-only", f"{base}...HEAD"],
        cwd=root,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(f"Unable to determine changed files: {result.stderr.strip()}")
    return [line for line in result.stdout.splitlines() if line]


def may_select_full_scan(path: str) -> bool:
    if path.startswith("src/"):
        return Path(path).name == "CMakeLists.txt" or path.endswith(".cmake")
    return any(
        path.startswith(entry) if entry.endswith("/") else path == entry
        for entry in FULL_SCAN_INPUTS
    )


def estimate_units(root: Path, changed: list[str]) -> int:
    """Count changed src/ sources plus src/ sources that may include a changed header."""
    units = {
        path
        for path in changed
        if path.startswith("src/")
        and Path(path).suffix in SOURCE_SUFFIXES
        and (root / path).is_file()
    }
    pending = [Path(path).name for path in changed if Path(path).suffix in HEADER_SUFFIXES]
    if not pending:
        return len(units)

    includers: dict[str, set[Path]] = defaultdict(set)
    for file in (root / "src").rglob("*"):
        if file.suffix in SOURCE_SUFFIXES | HEADER_SUFFIXES and file.is_file():
            for included in INCLUDE.findall(file.read_text(encoding="utf-8", errors="ignore")):
                includers[Path(included).name].add(file)

    seen = set(pending)
    while pending:
        for file in includers.get(pending.pop(), ()):
            if file.suffix in SOURCE_SUFFIXES:
                units.add(file.relative_to(root).as_posix())
            elif file.name not in seen:
                seen.add(file.name)
                pending.append(file.name)
    return len(units)


def tidy_shard_count(root: Path, base: str) -> int:
    if not base:
        return MAX_TIDY_SHARDS
    changed = changed_files(root, base)
    if any(may_select_full_scan(path) for path in changed):
        print("Analysis inputs changed; using every clang-tidy shard")
        return MAX_TIDY_SHARDS
    units = estimate_units(root, changed)
    print(f"Estimated clang-tidy compilation units: {units}")
    return max(1, min(MAX_TIDY_SHARDS, math.ceil(units / UNITS_PER_SHARD)))


def build_matrix(event: str, tool: str, root: Path, base: str) -> list[Leg]:
    """Return one leg per manual tool, or Clazy plus the clang-tidy shards for a PR."""
    if event == "workflow_dispatch":
        return [{"tool": tool, "shard": 1, "shard_count": 1}]
    count = tidy_shard_count(root, base)
    return [{"tool": "clazy", "shard": 1, "shard_count": 1}] + [
        {"tool": "clang-tidy", "shard": shard, "shard_count": count}
        for shard in range(1, count + 1)
    ]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--event", default=os.environ.get("EVENT_NAME", ""))
    parser.add_argument("--tool", default=os.environ.get("DISPATCH_TOOL") or "clazy")
    parser.add_argument("--base", default=os.environ.get("PR_BASE_SHA", ""))
    parser.add_argument("--root", type=Path, default=Path.cwd())
    args = parser.parse_args(argv)

    include = build_matrix(args.event, args.tool, args.root, args.base)
    serialized = json.dumps(include, separators=(",", ":"))
    write_github_output({"include": serialized})
    print(f"include={serialized}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
