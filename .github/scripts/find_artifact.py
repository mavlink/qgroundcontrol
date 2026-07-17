#!/usr/bin/env python3
"""Find build artifacts by glob pattern in a directory.

Two modes:
  Single pattern (back-compat): --pattern X → emits path= and found=true|false
  Multi pattern: --match name=X (repeatable) → emits <name>=<path-or-empty>
"""

from __future__ import annotations

import argparse
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh_error, gh_warning, write_github_output


def find_first(build_dir: Path, pattern: str, *, recursive: bool = True) -> Path | None:
    """Return the first file under build_dir matching pattern, or None."""
    matches = find_matches(build_dir, pattern, recursive=recursive)
    return matches[0] if matches else None


def find_matches(build_dir: Path, pattern: str, *, recursive: bool = True) -> list[Path]:
    """Return every matching file in deterministic path order."""
    iterator = build_dir.rglob(pattern) if recursive else build_dir.glob(pattern)
    return sorted(match for match in iterator if match.is_file())


def find_unique(build_dir: Path, pattern: str, *, recursive: bool = True) -> Path | None:
    """Return one matching file, raising when the pattern is ambiguous."""
    matches = find_matches(build_dir, pattern, recursive=recursive)
    if len(matches) > 1:
        paths = ", ".join(str(path) for path in matches)
        raise ValueError(f"Pattern {pattern!r} matched multiple artifacts: {paths}")
    return matches[0] if matches else None


def _parse_match(spec: str) -> tuple[str, str]:
    name, _, pattern = spec.partition("=")
    if not name or not pattern:
        raise argparse.ArgumentTypeError(f"--match expects name=pattern (got: {spec!r})")
    return name, pattern


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--pattern", help="Single glob pattern (e.g. '*.apk')")
    group.add_argument(
        "--match",
        action="append",
        type=_parse_match,
        metavar="NAME=PATTERN",
        help="Output key + glob pattern; repeat for multiple artifacts",
    )
    parser.add_argument(
        "--top-level",
        action="store_true",
        help="Match only files directly inside --build-dir",
    )
    parser.add_argument(
        "--required",
        action="store_true",
        help="Fail when the build directory or any requested artifact is missing",
    )
    args = parser.parse_args()

    if not args.build_dir.is_dir():
        report = gh_error if args.required else gh_warning
        report(f"Build directory does not exist: {args.build_dir}")
        if args.pattern is not None:
            write_github_output({"found": "false"})
        else:
            write_github_output({name: "" for name, _ in args.match})
        return 1 if args.required else 0

    if args.pattern is not None:
        try:
            artifact = find_unique(args.build_dir, args.pattern, recursive=not args.top_level)
        except ValueError as error:
            gh_error(str(error))
            return 1
        if artifact is None:
            report = gh_error if args.required else gh_warning
            report(f"No artifact matching {args.pattern} found")
            write_github_output({"found": "false"})
            return 1 if args.required else 0
        print(f"Found artifact: {artifact}")
        write_github_output({"path": str(artifact), "found": "true"})
        return 0

    outputs: dict[str, str] = {}
    missing = False
    for name, pattern in args.match:
        try:
            artifact = find_unique(args.build_dir, pattern, recursive=not args.top_level)
        except ValueError as error:
            gh_error(f"{name}: {error}")
            return 1
        if artifact is None:
            report = gh_error if args.required else gh_warning
            report(f"No artifact matching {pattern} found ({name})")
            outputs[name] = ""
            missing = True
        else:
            print(f"Found {name}: {artifact}")
            outputs[name] = str(artifact)
    write_github_output(outputs)
    return 1 if args.required and missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
