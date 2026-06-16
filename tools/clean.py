#!/usr/bin/env python3
"""Clean build artifacts and caches.

Examples:
    ./tools/clean.py              # Clean build directory
    ./tools/clean.py --all        # Clean everything (build, caches, generated files)
    ./tools/clean.py --cache      # Clean only caches (ccache, pip, etc.)
    ./tools/clean.py --dry-run    # Show what would be removed
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path
from typing import TYPE_CHECKING

from _bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common import find_repo_root, run_captured
from common.cli import add_dry_run
from common.io import chdir
from common.logging import log_info, log_ok, log_warn

if TYPE_CHECKING:
    from collections.abc import Iterable

BUILD_TARGETS: tuple[tuple[str, str], ...] = (
    ("build", "build directory"),
    ("CMakeUserPresets.json", "CMake user presets"),
)

CACHE_TARGETS: tuple[tuple[str, str], ...] = (
    (".cache", "local cache directory"),
    (".ccache", "ccache directory"),
    (".clangd", "clangd index"),
)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "-a", "--all", action="store_true", help="Clean build + caches + generated files"
    )
    parser.add_argument("-c", "--cache", action="store_true", help="Clean only caches")
    add_dry_run(parser, help="Show what would be removed; do not delete")
    return parser.parse_args(argv)


def repo_root() -> Path:
    return find_repo_root(Path(__file__))


def remove_path(path: Path, desc: str, *, dry_run: bool) -> None:
    if not path.exists() and not path.is_symlink():
        return
    if dry_run:
        log_info(f"Would remove: {desc}")
        return
    log_info(f"Removing: {desc}")
    if path.is_dir() and not path.is_symlink():
        shutil.rmtree(path, ignore_errors=True)
    else:
        path.unlink(missing_ok=True)


def remove_many(root: Path, entries: Iterable[tuple[str, str]], *, dry_run: bool) -> None:
    for name, desc in entries:
        remove_path(root / name, desc, dry_run=dry_run)


def clean_build(root: Path, *, dry_run: bool) -> None:
    remove_many(root, BUILD_TARGETS, dry_run=dry_run)
    for user_file in sorted(root.glob("*.user")):
        remove_path(user_file, f"Qt Creator user file: {user_file.name}", dry_run=dry_run)
    cmake_files = root / "CMakeFiles"
    if cmake_files.is_dir():
        remove_path(cmake_files, "CMake files: CMakeFiles", dry_run=dry_run)


def clean_cache(root: Path, *, dry_run: bool) -> None:
    remove_many(root, CACHE_TARGETS, dry_run=dry_run)
    if shutil.which("ccache"):
        if dry_run:
            log_info("Would clear ccache statistics")
        else:
            log_info("Clearing ccache statistics")
            run_captured(["ccache", "--zero-stats"])


def clean_generated(root: Path, *, dry_run: bool) -> None:
    for cache_dir in sorted(root.rglob("__pycache__")):
        remove_path(cache_dir, f"Python cache: {cache_dir.relative_to(root)}", dry_run=dry_run)
    for pyc in sorted(root.rglob("*.pyc")):
        remove_path(pyc, f"Python bytecode: {pyc.relative_to(root)}", dry_run=dry_run)


def report_disk_usage(root: Path) -> None:
    try:
        result = subprocess.run(
            ["du", "-sh", str(root)],
            capture_output=True,
            text=True,
            check=True,
        )
        print()
        log_info(f"Disk usage: {result.stdout.split()[0]}")
    except (subprocess.CalledProcessError, FileNotFoundError, IndexError):
        pass


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    root = repo_root()

    with chdir(root):
        if args.dry_run:
            log_warn("Dry run mode - no files will be removed")

        if args.cache:
            clean_cache(root, dry_run=args.dry_run)
        elif args.all:
            clean_build(root, dry_run=args.dry_run)
            clean_cache(root, dry_run=args.dry_run)
            clean_generated(root, dry_run=args.dry_run)
        else:
            clean_build(root, dry_run=args.dry_run)

        log_ok("Clean complete")

        if not args.dry_run:
            report_disk_usage(root)

    return 0


if __name__ == "__main__":
    sys.exit(main())
