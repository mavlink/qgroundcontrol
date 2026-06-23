#!/usr/bin/env python3
"""
CPM helper for CI: cache configuration and dependency fingerprinting.

Subcommands:
    fingerprint     Hash CMake files that declare CPM/FetchContent dependencies
    configure-cache Configure CPM_SOURCE_CACHE env + GitHub output
"""

from __future__ import annotations

import argparse
import hashlib
import re
import sys
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import append_github_env, write_github_output


def compute_cpm_fingerprint(root: Path) -> str:
    """Hash CMake files that declare CPM or FetchContent dependencies."""
    declaration_re = re.compile(r"CPM(Add|Find)Package|FetchContent_Declare")
    candidates: list[Path] = []
    for exact in [
        root / "CMakeLists.txt",
        root / "cmake/modules/CPM.cmake",
        root / ".github/build-config.json",
    ]:
        if exact.exists():
            candidates.append(exact)

    for directory in ("cmake",):
        base = root / directory
        if base.exists():
            candidates.extend(base.rglob("*.cmake"))

    for directory in ("src", "test"):
        base = root / directory
        if base.exists():
            candidates.extend(base.rglob("CMakeLists.txt"))

    dep_files: list[Path] = []
    for path in candidates:
        if not path.is_file():
            continue
        if path.name == "build-config.json":
            dep_files.append(path)
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        if declaration_re.search(text):
            dep_files.append(path)

    digest = hashlib.sha256()
    rel_paths = sorted({path.relative_to(root).as_posix() for path in dep_files})
    for rel_path in rel_paths:
        path = root / rel_path
        try:
            content = path.read_bytes()
        except OSError:
            continue
        content = content.replace(b"\r\n", b"\n").replace(
            b"\r", b"\n"
        )  # Windows autocrlf parity for cpm-modules-shared- key.
        digest.update(rel_path.encode("utf-8"))
        digest.update(b"\0")
        digest.update(content)
        digest.update(b"\0")
    return digest.hexdigest()


def configure_cpm_cache(path_value: str) -> Path:
    """Normalize and create the CPM cache path and export it."""
    cache_path = Path(path_value.replace("\\", "/"))
    cache_path.mkdir(parents=True, exist_ok=True)
    posix_path = cache_path.as_posix()
    append_github_env({"CPM_SOURCE_CACHE": posix_path})
    write_github_output({"path": posix_path})
    return cache_path


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="CPM helper for CI: cache configuration and dependency fingerprinting",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = parser.add_subparsers(dest="command")

    fp = sub.add_parser("fingerprint", help="Compute CPM dependency fingerprint")
    fp.add_argument("--root", type=Path, default=Path("."), help="Repository root")

    cfg = sub.add_parser("configure-cache", help="Configure CPM source cache path")
    cfg.add_argument("--path", required=True)

    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    if args.command == "fingerprint":
        fingerprint = compute_cpm_fingerprint(args.root.resolve())
        print(fingerprint)
        write_github_output({"fingerprint": fingerprint})
        return 0

    if args.command == "configure-cache":
        cache_path = configure_cpm_cache(args.path)
        print(cache_path)
        return 0

    print("Error: a subcommand is required (fingerprint, configure-cache)", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
