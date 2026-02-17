#!/usr/bin/env python3
"""Configure QGroundControl CMake build.

Usage:
    ./tools/configure.py                     # Default Debug build
    ./tools/configure.py --release           # Release build
    ./tools/configure.py --testing           # With unit tests
    ./tools/configure.py --coverage          # With coverage
    ./tools/configure.py --unity             # Unity build (faster)
    ./tools/configure.py --qt-root ~/Qt/6.8.0/gcc_64  # Explicit Qt

Environment:
    QT_ROOT_DIR - Qt installation (auto-detected if not set)
    CMAKE_GENERATOR - Generator (default: Ninja)
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class BuildConfig:
    """Build configuration options."""

    source_dir: Path = field(default_factory=lambda: Path("."))
    build_dir: Path = field(default_factory=lambda: Path("build"))
    build_type: str = "Debug"
    generator: str = "Ninja"
    testing: bool = False
    coverage: bool = False
    stable: bool = False
    unity_build: bool = False
    unity_batch_size: int = 16
    use_qt_cmake: bool = True
    qt_root: Path | None = None
    extra_args: list[str] = field(default_factory=list)


def parse_version(path: Path) -> tuple[int, ...]:
    """Extract version tuple from Qt path for sorting."""
    # Match patterns like 6.8.0, 6.10.1, etc.
    match = re.search(r"/(\d+)\.(\d+)\.(\d+)/", str(path))
    if match:
        return tuple(int(x) for x in match.groups())
    return (0, 0, 0)


def find_qt_cmake(qt_root: Path | None = None) -> Path | None:
    """Find qt-cmake executable, preferring newest version.

    Search order:
    1. Explicit qt_root parameter
    2. QT_ROOT_DIR environment variable
    3. Common installation paths (newest version first)
    """
    # Check explicit qt_root
    if qt_root:
        qt_cmake = qt_root / "bin" / "qt-cmake"
        if qt_cmake.exists() and os.access(qt_cmake, os.X_OK):
            return qt_cmake

    # Check QT_ROOT_DIR environment variable
    env_root = os.environ.get("QT_ROOT_DIR")
    if env_root:
        qt_cmake = Path(env_root) / "bin" / "qt-cmake"
        if qt_cmake.exists() and os.access(qt_cmake, os.X_OK):
            return qt_cmake

    # Common Qt installation patterns
    patterns = [
        Path.home() / "Qt" / "*" / "gcc_64" / "bin" / "qt-cmake",
        Path.home() / "Qt" / "*" / "clang_64" / "bin" / "qt-cmake",
        Path.home() / "Qt" / "*" / "macos" / "bin" / "qt-cmake",
        Path("/opt/Qt") / "*" / "gcc_64" / "bin" / "qt-cmake",
        Path("/usr/lib/qt6/bin/qt-cmake"),
    ]

    # Windows patterns
    if sys.platform == "win32":
        patterns.extend(
            [
                Path("C:/Qt") / "*" / "msvc2022_64" / "bin" / "qt-cmake.bat",
                Path("C:/Qt") / "*" / "msvc2019_64" / "bin" / "qt-cmake.bat",
            ]
        )

    for pattern in patterns:
        # Handle glob patterns
        if "*" in str(pattern):
            parent = pattern.parent.parent.parent  # Go up to Qt root
            if parent.exists():
                matches = list(parent.glob(str(pattern.relative_to(parent))))
                if matches:
                    # Sort by version (newest first)
                    matches.sort(key=parse_version, reverse=True)
                    for match in matches:
                        if match.exists() and os.access(match, os.X_OK):
                            return match
        else:
            if pattern.exists() and os.access(pattern, os.X_OK):
                return pattern

    return None


def configure(config: BuildConfig) -> int:
    """Run CMake configuration."""
    # Determine cmake command
    if config.use_qt_cmake:
        qt_cmake = find_qt_cmake(config.qt_root)
        if qt_cmake:
            cmake_cmd = str(qt_cmake)
            print(f"Using: {cmake_cmd}")
        else:
            print("Warning: qt-cmake not found, using cmake", file=sys.stderr)
            cmake_cmd = "cmake"
    else:
        cmake_cmd = "cmake"

    # Build CMake arguments
    args = [
        cmake_cmd,
        "-S",
        str(config.source_dir),
        "-B",
        str(config.build_dir),
        "-G",
        config.generator,
        f"-DCMAKE_BUILD_TYPE={config.build_type}",
    ]

    # Feature flags
    if config.testing:
        args.append("-DQGC_BUILD_TESTING=ON")
    else:
        args.append("-DQGC_BUILD_TESTING=OFF")

    if config.coverage:
        args.append("-DQGC_ENABLE_COVERAGE=ON")

    if config.stable:
        args.append("-DQGC_STABLE_BUILD=ON")

    if config.unity_build:
        args.append("-DCMAKE_UNITY_BUILD=ON")
        args.append(f"-DCMAKE_UNITY_BUILD_BATCH_SIZE={config.unity_batch_size}")

    # Extra arguments
    args.extend(config.extra_args)

    print(f"Build type: {config.build_type}")
    print(f"Build dir: {config.build_dir}")

    # Run cmake
    result = subprocess.run(args)

    if result.returncode != 0:
        return result.returncode

    # Output for CI if available
    github_output = os.environ.get("GITHUB_OUTPUT")
    if github_output:
        with open(github_output, "a") as f:
            f.write(f"build_dir={config.build_dir.resolve()}\n")

    print(f"Configured: {config.build_dir}")
    return 0


def parse_args() -> argparse.Namespace:
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
    parser.add_argument(
        "--stable",
        action="store_true",
        help="Build as stable release (QGC_STABLE_BUILD=ON)",
    )
    parser.add_argument(
        "--unity",
        action="store_true",
        help="Enable unity build (faster compilation)",
    )
    parser.add_argument(
        "--unity-batch",
        type=int,
        default=16,
        metavar="SIZE",
        help="Unity build batch size (default: 16)",
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

    return parser.parse_args()


def main() -> int:
    """Main entry point."""
    args = parse_args()

    # Default source dir to repo root when run from tools/
    source_dir = args.source_dir
    if source_dir == Path(".") and Path(__file__).parent.name == "tools":
        source_dir = Path(__file__).parent.parent

    config = BuildConfig(
        source_dir=source_dir.resolve(),
        build_dir=args.build_dir,
        build_type=args.build_type or "Debug",
        generator=args.generator,
        testing=args.testing,
        coverage=args.coverage,
        stable=args.stable,
        unity_build=args.unity,
        unity_batch_size=args.unity_batch,
        use_qt_cmake=not args.no_qt_cmake,
        qt_root=args.qt_root,
        extra_args=args.extra_args,
    )

    return configure(config)


if __name__ == "__main__":
    sys.exit(main())
