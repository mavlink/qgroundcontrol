"""CLI orchestration: parse args, print/list packages, dispatch to platform installer."""

from __future__ import annotations

import argparse
import sys

from . import _common as _c
from ._debian import install_debian
from ._linux import install_linux
from ._macos import install_macos
from ._packages import PACKAGE_TABLES, debian_jdk_package, get_packages
from ._windows import install_windows

PLATFORM_LABELS: dict[str, str] = {
    "debian": "Debian/Ubuntu (apt-get)",
    "fedora": "Fedora/RHEL (dnf)",
    "arch": "Arch Linux (pacman)",
    "macos": "macOS (Homebrew)",
    "windows": "Windows (installers)",
}
_BUILD_TOOLS_NOTE = "Also syncs tools/pyproject.toml's locked `build` group (CMake, Ninja, just)."


def _list_notes(platform: str) -> list[str]:
    if platform == "debian":
        return [f"android: core + {debian_jdk_package()}", _BUILD_TOOLS_NOTE]
    if platform in ("fedora", "arch"):
        return [_BUILD_TOOLS_NOTE]
    if platform == "macos":
        return ["GStreamer: the official framework packages, falling back to QGC's S3 mirror"]
    return [
        "No packages: installs GStreamer (unless --skip-gstreamer), and Visual Studio Build Tools"
        " (--msvc), NSIS (--nsis) and the Vulkan SDK (--vulkan) on request"
    ]


def print_packages(platform: str, category: str | None = None) -> None:
    """Print packages as a single space-separated line (machine-readable)."""
    if not PACKAGE_TABLES.get(platform):
        _c.log_error(f"--print-packages not supported for {platform}")
        sys.exit(1)
    if platform == "macos" and category:
        _c.log_warn(f"--category {category} ignored for macos (no categorized package list)")
        category = None
    print(" ".join(get_packages(platform, category)))


def list_packages(platform: str | None = None) -> None:
    """List packages by category for platform, or for every platform."""
    for index, name in enumerate([platform] if platform else PACKAGE_TABLES):
        if index:
            print()
        print(f"{PLATFORM_LABELS[name]}:")
        print("=" * 40)
        for category, packages in PACKAGE_TABLES[name].items():
            print(f"\n{category} ({len(packages)} packages):")
            for pkg in packages:
                print(f"  - {pkg}")
        print()
        for note in _list_notes(name):
            print(note)


def parse_args(args: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    platforms = "\n".join(f"  {name:<9} {label}" for name, label in PLATFORM_LABELS.items())
    parser = argparse.ArgumentParser(
        description="Install system dependencies for QGroundControl development.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""
Platforms:
{platforms}

Categories (Linux only):
  core        Build tools (cmake, ninja, git, etc.)
  qt          Qt6 development libraries
  gstreamer   GStreamer multimedia
  sdl         SDL/USB libraries
  audio       Audio libraries
  misc        Vulkan, PipeWire
  android     core plus the JDK for Gradle (Debian only)
  gstreamer_build  build_gstreamer.py's Linux source-build dependencies (Debian only)
  gstreamer_build_android  build_gstreamer.py's Android (Cerbero) build dependencies (Debian only)

Examples:
  %(prog)s                      # Auto-detect and install all
  %(prog)s --dry-run            # Show what would be installed
  %(prog)s --list               # List all packages
  %(prog)s --platform debian    # Force Debian installation
  %(prog)s --category qt        # Install only Qt dependencies
  %(prog)s --platform windows --vulkan  # Windows with Vulkan SDK
""",
    )

    parser.add_argument(
        "--dry-run", action="store_true", help="Show what would be installed without installing"
    )
    parser.add_argument(
        "--platform", choices=list(PLATFORM_LABELS), help="Override platform detection"
    )
    parser.add_argument(
        "--list", dest="list_packages", action="store_true", help="List packages by category"
    )
    parser.add_argument(
        "--print-packages",
        action="store_true",
        help="Print space-separated package list (machine-readable, for CI caching)",
    )
    parser.add_argument("--category", help="Install only specific category (Linux only)")
    parser.add_argument(
        "--skip-gstreamer", action="store_true", help="Skip GStreamer installation (Windows only)"
    )
    parser.add_argument("--vulkan", action="store_true", help="Install Vulkan SDK (Windows only)")
    parser.add_argument(
        "--msvc",
        action="store_true",
        help="Install Visual Studio Build Tools / VCTools (Windows only)",
    )
    parser.add_argument(
        "--msvc-arm64",
        action="store_true",
        help="Also add the ARM64 cross compiler component (implies --msvc, Windows only)",
    )
    parser.add_argument(
        "--nsis",
        action="store_true",
        help="Install NSIS / makensis for the installer build (Windows only)",
    )

    return parser.parse_args(args)


def main(argv: list[str] | None = None) -> int:
    """Main entry point."""
    args = parse_args(argv)

    if args.list_packages:
        list_packages(args.platform)
        return 0

    platform = args.platform or _c.detect_platform()

    if args.print_packages:
        print_packages(platform or "debian", args.category)
        return 0

    if platform is None:
        _c.log_error("Could not detect platform")
        _c.log_info(f"Use --platform to specify: {', '.join(PLATFORM_LABELS)}")
        return 1
    if platform not in PLATFORM_LABELS:
        _c.log_error(f"Unsupported platform: {platform}")
        return 1

    print(f"Platform: {platform}")
    if args.dry_run:
        print("Mode: DRY RUN (no changes will be made)\n")

    if args.category and platform in ("macos", "windows"):
        _c.log_warn("--category is only supported on Linux")

    if platform == "debian":
        success = install_debian(args.dry_run, args.category)
    elif platform == "macos":
        success = install_macos(args.dry_run)
    elif platform == "windows":
        success = install_windows(
            dry_run=args.dry_run,
            skip_gstreamer=args.skip_gstreamer,
            vulkan=args.vulkan,
            msvc=args.msvc or args.msvc_arm64,
            msvc_arm64=args.msvc_arm64,
            nsis=args.nsis,
        )
    else:
        success = install_linux(platform, args.dry_run, args.category)

    return 0 if success else 1


__all__ = ["PLATFORM_LABELS", "list_packages", "main", "parse_args", "print_packages"]
