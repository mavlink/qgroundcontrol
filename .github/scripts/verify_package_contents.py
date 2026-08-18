#!/usr/bin/env python3
"""Verify a Debian package ships only runtime files.

Regression guard for CPM-vendored dependencies (expat, zstd, mavlink, ...) leaking
development payload — headers, static archives, pkg-config files, CMake
package-config files — into the shipped .deb, where they collide with the matching
system -dev package on install (mavlink/qgroundcontrol#14874).
"""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path
from typing import TYPE_CHECKING

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh_error

if TYPE_CHECKING:
    from collections.abc import Callable

_FORBIDDEN_RULES: list[tuple[str, Callable[[str], bool]]] = [
    (
        "development header (usr/include)",
        lambda p: p == "usr/include" or p.startswith("usr/include/"),
    ),
    ("static archive", lambda p: p.endswith(".a") or p.endswith(".la")),
    ("pkg-config file", lambda p: p.endswith(".pc")),
    ("CMake package-config file", lambda p: p.endswith(".cmake")),
]


def list_deb_paths(deb_path: Path) -> list[str]:
    """Return the package's member paths (relative, no leading './') via dpkg-deb -c."""
    result = subprocess.run(
        ["dpkg-deb", "-c", str(deb_path)],
        capture_output=True,
        text=True,
        check=True,
    )
    paths = []
    for line in result.stdout.splitlines():
        fields = line.split(None, 5)
        if len(fields) < 6:
            continue
        raw_path = fields[5].split(" -> ", 1)[0]
        paths.append(raw_path.removeprefix("./"))
    return paths


def find_violations(paths: list[str]) -> list[tuple[str, str]]:
    """Return (path, reason) pairs for every forbidden path found."""
    violations = []
    for path in paths:
        if not path:
            continue
        for reason, predicate in _FORBIDDEN_RULES:
            if predicate(path):
                violations.append((path, reason))
                break
    return violations


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package-path", required=True, type=Path)
    args = parser.parse_args()

    if not args.package_path.is_file():
        gh_error(f"Package not found: {args.package_path}")
        return 1

    try:
        paths = list_deb_paths(args.package_path)
    except subprocess.CalledProcessError as exc:
        gh_error(f"dpkg-deb -c failed for {args.package_path}: {exc.stderr.strip()}")
        return 1

    violations = find_violations(paths)
    if violations:
        gh_error(
            f"{args.package_path.name} ships {len(violations)} development file(s) "
            "that belong to vendored CPM dependencies, not the runtime package:"
        )
        for path, reason in violations:
            print(f"  {path}  ({reason})")
        return 1

    print(
        f"OK: {args.package_path.name} contains no development payload ({len(paths)} entries checked)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
