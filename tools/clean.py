#!/usr/bin/env python3
"""Clean build artifacts and caches.

Usage:
    ./tools/clean.py              # Clean build directory
    ./tools/clean.py --all        # Clean everything (build, caches, generated files)
    ./tools/clean.py --cache      # Clean only caches (ccache, clangd index, etc.)
    ./tools/clean.py --dry-run    # Show what would be deleted without removing

This script removes:
    - build/           CMake build directory
    - .cache/          Local caches (ccache, clangd index)
    - *.user           Qt Creator user files
    - CMakeUserPresets.json
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class CleanStats:
    """Statistics about items to be cleaned."""

    files: list[Path] = field(default_factory=list)
    directories: list[Path] = field(default_factory=list)
    sizes: list[str] = field(default_factory=list)

    @property
    def total_items(self) -> int:
        return len(self.files) + len(self.directories)


def get_size_str(path: Path) -> str:
    """Get human-readable size of path."""
    try:
        if path.is_file():
            size = path.stat().st_size
        elif path.is_dir():
            size = sum(f.stat().st_size for f in path.rglob("*") if f.is_file())
        else:
            return "?"

        for unit in ["B", "K", "M", "G"]:
            if size < 1024:
                return f"{size:.1f}{unit}"
            size /= 1024
        return f"{size:.1f}T"
    except (OSError, PermissionError):
        return "?"


class Cleaner:
    """Clean build artifacts and caches."""

    def __init__(self, repo_root: Path, dry_run: bool = False):
        self.repo_root = repo_root
        self.dry_run = dry_run
        self.stats = CleanStats()

    def _remove(self, path: Path, description: str | None = None) -> None:
        """Remove a file or directory, tracking statistics."""
        if not path.exists():
            return

        desc = description or str(path.relative_to(self.repo_root))
        size = get_size_str(path)

        if self.dry_run:
            print(f"Would delete: {desc} ({size})")
            self.stats.sizes.append(size)
            if path.is_dir():
                self.stats.directories.append(path)
            else:
                self.stats.files.append(path)
        else:
            print(f"Removing: {desc}")
            if path.is_dir():
                shutil.rmtree(path)
            else:
                path.unlink()

    def clean_build(self) -> None:
        """Clean build directory and CMake artifacts."""
        self._remove(self.repo_root / "build", "build directory")
        self._remove(self.repo_root / "CMakeUserPresets.json", "CMake user presets")

        # Qt Creator user files
        for user_file in self.repo_root.glob("*.user"):
            self._remove(user_file, f"Qt Creator user file: {user_file.name}")

        # CMake generated files in source
        for cmake_dir in self.repo_root.glob("CMakeFiles"):
            self._remove(cmake_dir, f"CMake files: {cmake_dir.name}")

    def clean_cache(self) -> None:
        """Clean local caches."""
        self._remove(self.repo_root / ".cache", "local cache directory")
        self._remove(self.repo_root / ".clangd", "clangd index")

        # Clear ccache statistics (not the cache itself)
        if shutil.which("ccache"):
            if self.dry_run:
                print("Would clear: ccache statistics")
            else:
                print("Clearing ccache statistics")
                subprocess.run(
                    ["ccache", "--zero-stats"],
                    capture_output=True,
                    check=False,
                )

    def clean_generated(self) -> None:
        """Clean generated files."""
        # Python cache directories
        for pycache in self.repo_root.rglob("__pycache__"):
            if pycache.is_dir():
                self._remove(pycache, f"Python cache: {pycache.relative_to(self.repo_root)}")

        # Python bytecode files
        for pyc in self.repo_root.rglob("*.pyc"):
            self._remove(pyc, f"Python bytecode: {pyc.relative_to(self.repo_root)}")

    def print_summary(self) -> None:
        """Print dry-run summary."""
        if not self.dry_run or self.stats.total_items == 0:
            return

        print()
        print("Dry run complete. Would delete:")
        print(f"  - {len(self.stats.files)} files")
        print(f"  - {len(self.stats.directories)} directories")

        if self.stats.sizes:
            print(f"  - Sizes: {', '.join(self.stats.sizes)}")

        print()
        print("Run without --dry-run to actually delete these items")

    def print_disk_usage(self) -> None:
        """Print current disk usage."""
        size = get_size_str(self.repo_root)
        print(f"Disk usage: {size}")


def parse_args() -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Clean build artifacts and caches",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
This script removes:
    - build/           CMake build directory
    - .cache/          Local caches (ccache, clangd index)
    - *.user           Qt Creator user files
    - CMakeUserPresets.json

Examples:
    %(prog)s              # Clean build directory
    %(prog)s --all        # Clean everything
    %(prog)s --dry-run    # Show what would be deleted
""",
    )

    parser.add_argument(
        "-a",
        "--all",
        action="store_true",
        help="Clean everything (build, caches, generated)",
    )
    parser.add_argument(
        "-c",
        "--cache",
        action="store_true",
        help="Clean only caches",
    )
    parser.add_argument(
        "-n",
        "--dry-run",
        action="store_true",
        help="Show what would be deleted without removing",
    )

    return parser.parse_args()


def main() -> int:
    """Main entry point."""
    args = parse_args()

    repo_root = Path(__file__).parent.parent
    cleaner = Cleaner(repo_root, dry_run=args.dry_run)

    if args.dry_run:
        print("DRY RUN MODE - No files will be removed")
        print()

    if args.cache:
        cleaner.clean_cache()
    elif args.all:
        cleaner.clean_build()
        cleaner.clean_cache()
        cleaner.clean_generated()
    else:
        cleaner.clean_build()

    if args.dry_run:
        cleaner.print_summary()
    else:
        print("Clean complete")
        print()
        cleaner.print_disk_usage()

    return 0


if __name__ == "__main__":
    sys.exit(main())
