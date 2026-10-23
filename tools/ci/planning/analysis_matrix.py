#!/usr/bin/env python3
"""Emit the analysis.yml job matrix as a JSON 'include' list plus the analysis toolchain.

Every compiler-analysis job runs clang-tidy and Clazy over one dependency scan,
after several minutes of setup, so pull requests get only as many shards as
their estimated compilation units need and none when nothing compiled changed.
Changed headers are expanded through a textual reverse-include scan of src/,
which may overcount but does not miss ordinary includes. Changes to an analysis
input select every active unit on every shard; the result cache seeded by
master pushes keeps unchanged units cheap.
tools/analyze.py still selects the exact units after configure.

Each leg carries what its steps need: a `family` that selects the steps, the
packages to install, the CMake configure arguments, and either the
tools/analyze.py arguments or the IWYU path. Dispatch paths are validated here
and reach commands only as single arguments.
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

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.build_config import find_build_config, load_build_config
from common.gh_actions import annotate, parse_bool, write_github_output
from common.git import pr_diff_refs

Leg = dict[str, str | int | list[str]]

COMPILER_TOOLS = ["clang-tidy", "clazy"]
# Manual dispatch choices and the workflow step family each one runs.
DISPATCH_FAMILIES = {
    "clazy": "compiler",
    "clang-tidy": "compiler",
    "qmllint": "qml",
    "iwyu": "iwyu",
    "tsan": "tsan",
}
RESULT_CACHE_DIR = ".cache/analysis-results"
SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx"})
HEADER_SUFFIXES = frozenset({".h", ".hh", ".hpp", ".hxx", ".inl"})
MAX_TIDY_SHARDS = 4
UNITS_PER_SHARD = 50
# analysis.yml's trigger paths outside src/; each change scans every active unit.
# Entries ending in "/" match a directory tree, anything else one file.
FULL_SCAN_INPUTS = (
    "cmake/",
    "CMakeLists.txt",
    "CMakePresets.json",
    ".clang-tidy",
    ".github/build-config.json",
    "tools/ci/planning/analysis_matrix.py",
    ".github/workflows/analysis.yml",
    "tools/analyze.py",
    "tools/analyzers/__init__.py",
    "tools/analyzers/base.py",
    "tools/analyzers/clang_tidy.py",
    "tools/analyzers/clazy.py",
    "tools/analyzers/compiler.py",
    "tools/analyzers/dependencies.py",
    "tools/analyzers/result_cache.py",
    "tools/setup/build_clazy.py",
    "tools/ci/runner/analysis_tools.py",
    # Generators of headers that analyzed sources include.
    "tools/common/io.py",
    "tools/common/xml.py",
    "tools/generators/mavlink_enums.py",
    "tools/generators/mavlink_instance_fields.py",
)
# Configure inputs under src/: CMake scripts, configured templates and dependency patches.
SOURCE_TREE_INPUT_SUFFIXES = frozenset({".cmake", ".in", ".patch"})
INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)


def changed_files(root: Path, refs: tuple[str, str]) -> list[str]:
    """Return paths changed since the merge base; a rename lists both its old and new path."""
    result = subprocess.run(
        ["git", "diff", "--name-only", "--no-renames", f"{refs[0]}...{refs[1]}"],
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
        return (
            Path(path).name == "CMakeLists.txt" or Path(path).suffix in SOURCE_TREE_INPUT_SUFFIXES
        )
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


def pull_request_plan(root: Path, base: str, head: str = "") -> tuple[int, str]:
    """Return the shard count (0 when nothing needs analysis) and scan scope for a PR."""
    if not base:
        print("No pull request base; scanning every active unit")
        return MAX_TIDY_SHARDS, "all"
    refs = pr_diff_refs(base, head, cwd=root)
    if not refs:
        return MAX_TIDY_SHARDS, "all"
    changed = changed_files(root, refs)
    inputs = [path for path in changed if may_select_full_scan(path)]
    if inputs:
        print(f"Analysis inputs changed ({', '.join(inputs[:5])}); scanning every active unit")
        return MAX_TIDY_SHARDS, "all"
    units = estimate_units(root, changed)
    print(f"Estimated compilation units: {units}")
    return min(MAX_TIDY_SHARDS, math.ceil(units / UNITS_PER_SHARD)), "changed"


def validate_path(path: str) -> str:
    """Return a dispatch path that stays in the checkout and cannot be read as an option."""
    path = path.strip()
    if (
        path.startswith(("/", "-"))
        or ".." in path
        or not all(character.isprintable() for character in path)
    ):
        raise ValueError(
            f"Analysis path must be relative, without '..', a leading '-' "
            f"or control characters: {path!r}"
        )
    return path


def scope(analyze_all: bool, path: str) -> tuple[str, list[str]]:
    """Return the scan scope and its tools/analyze.py arguments; no path scans changed files."""
    if analyze_all:
        return "all", ["--all"]
    path = validate_path(path)
    return ("path", [path]) if path else ("changed", [])


def analysis_packages(family: str, llvm_version: str) -> str:
    """Return the extra apt packages for a family; tsan builds with the default toolchain."""
    if family == "tsan":
        return ""
    # The runner image already ships these; listing them keeps other images working.
    packages = [f"clang-{llvm_version}", f"clang-tools-{llvm_version}"]
    if family == "iwyu":
        packages.append("iwyu")
    return " ".join(packages)


def configure_args(family: str, llvm_version: str) -> list[str]:
    """Return the Linux-debug configure arguments for a family; tsan keeps the default compiler.

    Compiler analysis builds only generated headers (IWYU also autogen), so it skips PCH,
    unity batches, and autogen's inherited link dependencies; qmllint keeps the full build.
    """
    if family == "tsan":
        return ["-DQGC_ENABLE_COVERAGE=OFF", "-DQGC_ENABLE_TSAN=ON"]
    args = [
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-DQGC_BUILD_TESTING=OFF",
        "-DQGC_ENABLE_WERROR=OFF",
        f"-DCMAKE_C_COMPILER=clang-{llvm_version}",
        f"-DCMAKE_CXX_COMPILER=clang++-{llvm_version}",
        "-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON",
    ]
    if family != "qml":
        args += ["-DCMAKE_AUTOGEN_ORIGIN_DEPENDS=OFF", "-DCMAKE_GLOBAL_AUTOGEN_TARGET=ON"]
    return [*args, "-DQGC_UNITY_BUILD=OFF"]


def compiler_args(
    tools: list[str], shard: int, shard_count: int, scope_args: list[str], *, profile: bool = False
) -> list[str]:
    args = [arg for tool in tools for arg in ("--tool", tool)]
    args += ["--advisory", "--cache-dir", RESULT_CACHE_DIR]
    if shard_count > 1:
        args += ["--shard", str(shard), "--shard-count", str(shard_count)]
    if profile and "clang-tidy" in tools:
        args.append("--profile-checks")
    return args + scope_args


def compiler_legs(count: int, scan: str, llvm_version: str) -> list[Leg]:
    scope_args = ["--all"] if scan == "all" else []
    return [
        {
            "tool": COMPILER_TOOLS[0],
            "tools": COMPILER_TOOLS,
            "family": "compiler",
            "shard": shard,
            "shard_count": count,
            "scan": scan,
            "extra_packages": analysis_packages("compiler", llvm_version),
            "configure_args": configure_args("compiler", llvm_version),
            "analyze_args": compiler_args(COMPILER_TOOLS, shard, count, scope_args),
        }
        for shard in range(1, count + 1)
    ]


def dispatch_leg(
    tool: str,
    llvm_version: str,
    *,
    path: str = "",
    analyze_all: bool = True,
    profile_checks: bool = False,
) -> Leg:
    """Return the single job for a manually selected tool."""
    family = DISPATCH_FAMILIES[tool]
    leg: Leg = {
        "tool": tool,
        "tools": [tool],
        "family": family,
        "shard": 1,
        "shard_count": 1,
        "extra_packages": analysis_packages(family, llvm_version),
        "configure_args": configure_args(family, llvm_version),
    }
    if family == "compiler":
        leg["scan"], scope_args = scope(analyze_all, path)
        leg["analyze_args"] = compiler_args([tool], 1, 1, scope_args, profile=profile_checks)
    elif family == "qml":
        leg["scan"], scope_args = scope(analyze_all, path)
        leg["analyze_args"] = ["--tool", tool, "--qml-build", "--advisory", *scope_args]
    elif family == "iwyu":
        # IWYU has no changed-file mode, so anything but an explicit path checks all of src/.
        leg["path"] = (validate_path(path) if not analyze_all else "") or "src"
    return leg


def build_matrix(
    event: str,
    root: Path,
    base: str,
    *,
    llvm_version: str,
    head: str = "",
    tool: str = "clazy",
    path: str = "",
    analyze_all: bool = True,
    profile_checks: bool = False,
) -> list[Leg]:
    """Return one leg per manual tool, one full job for master pushes, or the PR's shards."""
    if event == "workflow_dispatch":
        return [
            dispatch_leg(
                tool,
                llvm_version,
                path=path,
                analyze_all=analyze_all,
                profile_checks=profile_checks,
            )
        ]
    if event == "push":
        # A single job saves one complete result cache for pull requests to restore.
        return compiler_legs(1, "all", llvm_version)
    return compiler_legs(*pull_request_plan(root, base, head), llvm_version)


def analysis_toolchain(root: Path) -> dict[str, str]:
    """Return the pinned analysis toolchain from build-config.json as step outputs."""
    config = load_build_config(find_build_config(root))
    return {key: str(value) for key, value in config["analysis"].items()}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--event", default=os.environ.get("EVENT_NAME", ""))
    parser.add_argument(
        "--tool", choices=DISPATCH_FAMILIES, default=os.environ.get("DISPATCH_TOOL") or "clazy"
    )
    parser.add_argument("--path", default=os.environ.get("DISPATCH_PATH", ""))
    parser.add_argument(
        "--analyze-all",
        action=argparse.BooleanOptionalAction,
        default=parse_bool(os.environ.get("DISPATCH_ANALYZE_ALL") or "true"),
    )
    parser.add_argument(
        "--profile-checks",
        action=argparse.BooleanOptionalAction,
        default=parse_bool(os.environ.get("DISPATCH_PROFILE_CHECKS")),
    )
    parser.add_argument("--base", default=os.environ.get("PR_BASE_SHA", ""))
    parser.add_argument("--head", default=os.environ.get("PR_HEAD_SHA", ""))
    parser.add_argument("--root", type=Path, default=Path.cwd())
    args = parser.parse_args(argv)

    toolchain = analysis_toolchain(args.root)
    try:
        include = build_matrix(
            args.event,
            args.root,
            args.base,
            llvm_version=toolchain["llvm_version"],
            head=args.head,
            tool=args.tool,
            path=args.path,
            analyze_all=args.analyze_all,
            profile_checks=args.profile_checks,
        )
    except ValueError as error:
        annotate("error", str(error))
        return 1
    serialized = json.dumps(include, separators=(",", ":"))
    write_github_output({"include": serialized, **toolchain})
    print(f"Planned {len(include)} analysis job(s)")
    for leg in include:
        print(f"  {json.dumps(leg)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
