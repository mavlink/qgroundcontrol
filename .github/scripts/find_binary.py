#!/usr/bin/env python3
"""Find QGroundControl binary in build directory.

Handles different build layouts:
    - build/QGroundControl (single-config generators like Ninja)
    - build/Release/QGroundControl (multi-config generators)
    - build/Debug/QGroundControl (multi-config generators)
    - Windows: .exe extension
    - macOS: .app bundle

Usage:
    find_binary.py [--build-dir DIR] [--build-type TYPE] [--platform PLATFORM]

Outputs (for GitHub Actions):
    binary_path - Full path to the binary
    binary_name - Just the filename
    binary_dir  - Directory containing the binary
"""

from __future__ import annotations

import argparse
import os
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass
class BinaryInfo:
    """Information about the found binary."""

    path: Path
    name: str
    directory: Path


def detect_platform() -> str:
    """Detect current platform."""
    if sys.platform == "win32":
        return "windows"
    elif sys.platform == "darwin":
        return "macos"
    else:
        return "linux"


def get_binary_name(platform: str) -> str:
    """Get binary name for platform."""
    if platform == "windows":
        return "QGroundControl.exe"
    elif platform == "macos":
        return "QGroundControl.app"
    else:
        return "QGroundControl"


def find_binary(
    build_dir: Path,
    build_type: str | None = None,
    platform: str | None = None,
) -> BinaryInfo | None:
    """Find QGroundControl binary in build directory.

    Args:
        build_dir: Build directory to search
        build_type: Build type hint (Release, Debug, etc.)
        platform: Target platform (linux, macos, windows)

    Returns:
        BinaryInfo if found, None otherwise
    """
    if platform is None:
        platform = detect_platform()

    binary_name = get_binary_name(platform)

    print(f"Searching for {binary_name} in {build_dir}...")

    # Build search order based on build type
    search_paths: list[Path] = []

    if build_type:
        # Prioritize the specified build type
        search_paths.append(build_dir / build_type / binary_name)
        search_paths.append(build_dir / binary_name)
    else:
        # Default search order: single-config first, then Release, then Debug
        search_paths.extend(
            [
                build_dir / binary_name,
                build_dir / "Release" / binary_name,
                build_dir / "Debug" / binary_name,
                build_dir / "RelWithDebInfo" / binary_name,
                build_dir / "MinSizeRel" / binary_name,
            ]
        )

    # Search standard paths first
    for path in search_paths:
        if path.exists():
            return BinaryInfo(
                path=path.resolve(),
                name=binary_name,
                directory=path.parent.resolve(),
            )

    # Fallback: recursive search
    print("Standard paths not found, searching recursively...")

    # For files (Linux/Windows)
    if platform != "macos":
        for found in build_dir.rglob(binary_name):
            if found.is_file():
                return BinaryInfo(
                    path=found.resolve(),
                    name=binary_name,
                    directory=found.parent.resolve(),
                )

    # For macOS .app bundles (directories)
    if platform == "macos":
        for found in build_dir.rglob(binary_name):
            if found.is_dir():
                return BinaryInfo(
                    path=found.resolve(),
                    name=binary_name,
                    directory=found.parent.resolve(),
                )

    return None


def output_github_actions(info: BinaryInfo) -> None:
    """Write outputs for GitHub Actions."""
    github_output = os.environ.get("GITHUB_OUTPUT")
    if github_output:
        with open(github_output, "a") as f:
            f.write(f"binary_path={info.path}\n")
            f.write(f"binary_name={info.name}\n")
            f.write(f"binary_dir={info.directory}\n")


def parse_args() -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Find QGroundControl binary in build directory",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Outputs (GITHUB_OUTPUT):
    binary_path  - Full path to binary
    binary_name  - Filename only
    binary_dir   - Directory containing binary

Examples:
    %(prog)s --build-dir build
    %(prog)s --build-type Release
    %(prog)s --platform windows
""",
    )

    parser.add_argument(
        "--build-dir",
        type=Path,
        default=Path("build"),
        help="Build directory (default: build)",
    )
    parser.add_argument(
        "--build-type",
        choices=["Release", "Debug", "RelWithDebInfo", "MinSizeRel"],
        help="Build type hint (helps prioritize search)",
    )
    parser.add_argument(
        "--platform",
        choices=["linux", "macos", "windows", "android", "ios"],
        help="Target platform (auto-detected if not specified)",
    )

    return parser.parse_args()


def main() -> int:
    """Main entry point."""
    args = parse_args()

    result = find_binary(
        build_dir=args.build_dir,
        build_type=args.build_type,
        platform=args.platform,
    )

    if result is None:
        print(f"::error::Binary not found in {args.build_dir}", file=sys.stderr)
        return 1

    print(f"Found: {result.path}")

    # Output for GitHub Actions
    output_github_actions(result)

    # Also output to stdout for non-CI usage
    print(f"binary_path={result.path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
