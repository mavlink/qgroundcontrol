#!/usr/bin/env python3
"""Find a build artifact by glob pattern in a directory.

Usage:
    find_artifact.py --build-dir DIR --pattern PATTERN

Outputs (GITHUB_OUTPUT):
    path  - Full path to the artifact
    found - 'true' or 'false'
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--pattern", required=True, help="Glob pattern (e.g. '*.apk')")
    args = parser.parse_args()

    github_output = os.environ.get("GITHUB_OUTPUT")

    def set_output(key: str, value: str) -> None:
        if github_output:
            with open(github_output, "a", encoding="utf-8") as f:
                f.write(f"{key}={value}\n")

    if not args.build_dir.is_dir():
        print(f"::warning::Build directory does not exist: {args.build_dir}")
        set_output("found", "false")
        return 0

    matches = list(args.build_dir.rglob(args.pattern))
    files = [m for m in matches if m.is_file()]

    if not files:
        print(f"::warning::No artifact matching {args.pattern} found")
        set_output("found", "false")
        return 0

    artifact = files[0]
    print(f"Found artifact: {artifact}")
    set_output("path", str(artifact))
    set_output("found", "true")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
