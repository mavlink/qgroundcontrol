#!/usr/bin/env python3
"""Configure QGroundControl CMake build.

Usage:
    ./tools/configure.py                     # Default Debug build
    ./tools/configure.py --release           # Release build
    ./tools/configure.py --preset Linux-debug  # Configure into the preset's binaryDir
    ./tools/configure.py --qt-root ~/Qt/6.8.0/gcc_64  # Explicit Qt

Environment:
    QT_ROOT_DIR - Qt installation (auto-detected if not set)
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import _bootstrap as _bootstrap
from common.file_traversal import find_repo_root
from qgc_tools.cmake_configure import CMakeConfig, configure


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Configure QGroundControl CMake build",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Environment:
  QT_ROOT_DIR         Qt installation (auto-detected if not set)

Examples:
  %(prog)s --release
  %(prog)s -B build-debug --debug
  %(prog)s --preset Linux-debug
  %(prog)s --qt-root ~/Qt/6.8.0/gcc_64 --release
""",
    )

    parser.add_argument(
        "-S",
        "--source-dir",
        type=Path,
        default=Path("."),
        help="Source directory (default: current directory)",
    )
    parser.add_argument(
        "-B",
        "--build-dir",
        type=Path,
        help="Build directory (default: the preset's binaryDir)",
    )
    parser.add_argument(
        "-t",
        "--build-type",
        choices=["Debug", "Release", "RelWithDebInfo", "MinSizeRel"],
        default="Debug",
        help="Build type selecting the default* preset (default: Debug)",
    )
    parser.add_argument(
        "--release",
        action="store_const",
        const="Release",
        dest="build_type",
        help="Shorthand for --build-type Release",
    )
    parser.add_argument(
        "--debug",
        action="store_const",
        const="Debug",
        dest="build_type",
        help="Shorthand for --build-type Debug",
    )
    parser.add_argument(
        "--preset",
        help="CMake configure preset (default: matching default* preset)",
    )
    parser.add_argument(
        "--qt-root",
        type=Path,
        help="Qt installation directory",
    )
    parser.add_argument(
        "--no-qt-cmake",
        action="store_true",
        help="Use cmake instead of qt-cmake",
    )
    parser.add_argument(
        "extra_args",
        nargs="*",
        help="Additional CMake arguments",
    )

    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Main entry point."""
    args = parse_args(argv)

    # Default source dir to repo root when run from tools/
    source_dir = args.source_dir
    if source_dir == Path(".") and Path(__file__).parent.name == "tools":
        source_dir = find_repo_root(Path(__file__))

    config = CMakeConfig(
        source_dir=source_dir.resolve(),
        build_dir=args.build_dir,
        build_type=args.build_type or "Debug",
        preset=args.preset,
        use_qt_cmake=not args.no_qt_cmake,
        qt_root=args.qt_root,
        extra_args=args.extra_args,
    )

    return configure(config)


if __name__ == "__main__":
    sys.exit(main())
