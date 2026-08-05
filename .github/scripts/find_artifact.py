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

from common.gh_actions import gh_warning, write_github_output


def find_first(build_dir: Path, pattern: str) -> Path | None:
    """Return the first file under build_dir matching pattern, or None."""
    for match in build_dir.rglob(pattern):
        if match.is_file():
            return match
    return None


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
    args = parser.parse_args()

    if not args.build_dir.is_dir():
        gh_warning(f"Build directory does not exist: {args.build_dir}")
        if args.pattern is not None:
            write_github_output({"found": "false"})
        else:
            write_github_output({name: "" for name, _ in args.match})
        return 0

    if args.pattern is not None:
        artifact = find_first(args.build_dir, args.pattern)
        if artifact is None:
            gh_warning(f"No artifact matching {args.pattern} found")
            write_github_output({"found": "false"})
            return 0
        print(f"Found artifact: {artifact}")
        write_github_output({"path": str(artifact), "found": "true"})
        return 0

    outputs: dict[str, str] = {}
    for name, pattern in args.match:
        artifact = find_first(args.build_dir, pattern)
        if artifact is None:
            gh_warning(f"No artifact matching {pattern} found ({name})")
            outputs[name] = ""
        else:
            print(f"Found {name}: {artifact}")
            outputs[name] = str(artifact)
    write_github_output(outputs)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
