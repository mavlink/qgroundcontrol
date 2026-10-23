#!/usr/bin/env python3
"""Find exactly one build artifact by glob pattern and emit its path.

Fails when the build directory is missing or the pattern matches no file or
several files. --copy-to DEST also copies the match to DEST.
"""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import gh_error, write_github_output


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


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--pattern", required=True, help="Glob pattern (e.g. '*.apk')")
    parser.add_argument(
        "--top-level",
        action="store_true",
        help="Match only files directly inside --build-dir",
    )
    parser.add_argument(
        "--copy-to",
        type=Path,
        metavar="DEST",
        help="Copy the match to this file path",
    )
    args = parser.parse_args(argv)

    if not args.build_dir.is_dir():
        gh_error(f"Build directory does not exist: {args.build_dir}")
        return 1

    try:
        artifact = find_unique(args.build_dir, args.pattern, recursive=not args.top_level)
    except ValueError as error:
        gh_error(str(error))
        return 1
    if artifact is None:
        gh_error(f"No artifact matching {args.pattern} found")
        return 1
    print(f"Found artifact: {artifact}")
    if args.copy_to is not None:
        shutil.copy2(artifact, args.copy_to)
        print(f"Copied artifact to {args.copy_to}")
    write_github_output({"path": str(artifact)})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
