#!/usr/bin/env python3
"""Configure QGroundControl CMake build.

Usage:
    ./tools/configure.py                     # Default Debug build
    ./tools/configure.py --release           # Release build
    ./tools/configure.py --testing           # With unit tests
    ./tools/configure.py --coverage          # With coverage
    ./tools/configure.py --qt-root ~/Qt/6.8.0/gcc_64  # Explicit Qt

Environment:
    QT_ROOT_DIR - Qt installation (auto-detected if not set)
    CMAKE_GENERATOR - Generator (default: Ninja)
"""

from __future__ import annotations

import argparse
import os
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
  CMAKE_GENERATOR     Default generator

Examples:
  %(prog)s --release --testing
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
        default=Path("build"),
        help="Build directory (default: build)",
    )
    parser.add_argument(
        "-t",
        "--build-type",
        choices=["Debug", "Release", "RelWithDebInfo", "MinSizeRel"],
        default="Debug",
        help="Build type (default: Debug)",
    )
    parser.add_argument(
        "-G",
        "--generator",
        default=os.environ.get("CMAKE_GENERATOR", "Ninja"),
        help="CMake generator (default: Ninja)",
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
        "--testing",
        action="store_true",
        help="Enable unit tests (QGC_BUILD_TESTING=ON)",
    )
    parser.add_argument(
        "--coverage",
        action="store_true",
        help="Enable code coverage (QGC_ENABLE_COVERAGE=ON)",
    )
    preset_group = parser.add_mutually_exclusive_group()
    preset_group.add_argument(
        "--preset",
        help="CMake configure preset (default: matching default* preset)",
    )
    preset_group.add_argument(
        "--no-preset",
        action="store_true",
        help="Use legacy command-line configuration for an unsupported custom setup",
    )
    parser.add_argument(
        "--stable",
        action="store_true",
        help="Build as stable release (QGC_STABLE_BUILD=ON)",
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
        generator=args.generator,
        testing=args.testing,
        coverage=args.coverage,
        preset=args.preset,
        use_preset=not args.no_preset,
        stable=args.stable,
        use_qt_cmake=not args.no_qt_cmake,
        qt_root=args.qt_root,
        extra_args=args.extra_args,
    )

    return configure(config)


if __name__ == "__main__":
    sys.exit(main())
