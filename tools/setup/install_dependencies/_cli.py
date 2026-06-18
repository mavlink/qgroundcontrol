"""CLI orchestration: parse args, print/list packages, dispatch to platform installer."""

from __future__ import annotations

import argparse
import sys

from . import _common as _c
from ._arch import install_arch
from ._debian import install_debian
from ._fedora import install_fedora
from ._macos import install_macos
from ._packages import (
    DEBIAN_PACKAGES,
    MACOS_PACKAGES,
    PIPX_PACKAGES,
    get_arch_packages,
    get_debian_packages,
    get_fedora_packages,
    get_macos_packages,
    validate_extra_packages,
)
from ._windows import install_windows

LINUX_PLATFORMS = ("debian", "fedora", "arch")


def print_packages(platform: str, category: str | None = None) -> None:
    """Print packages as a single space-separated line (machine-readable)."""
    if platform == "debian":
        print(" ".join(get_debian_packages(category)))
    elif platform == "fedora":
        print(" ".join(get_fedora_packages(category)))
    elif platform == "arch":
        print(" ".join(get_arch_packages(category)))
    elif platform == "macos":
        if category:
            _c.log_warn(f"--category {category} ignored for macos (no categorized package list)")
        print(" ".join(get_macos_packages()))
    else:
        _c.log_error(f"--print-packages not supported for {platform}")
        sys.exit(1)


def list_packages(platform: str | None = None) -> None:
    """List packages by category."""
    if platform in (None, "debian"):
        print("Debian/Ubuntu packages:")
        print("=" * 40)
        for category, packages in DEBIAN_PACKAGES.items():
            print(f"\n{category} ({len(packages)} packages):")
            for pkg in packages:
                print(f"  - {pkg}")
        print("\nPipx packages:")
        for pkg in PIPX_PACKAGES:
            print(f"  - {pkg}")

    if platform in (None, "macos"):
        print("\n" + "=" * 40)
        print("macOS packages (Homebrew):")
        print("=" * 40)
        for pkg in MACOS_PACKAGES:
            print(f"  - {pkg}")
        print("\nGStreamer: Downloaded from gstreamer.freedesktop.org")


def parse_args(args: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Install system dependencies for QGroundControl development.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Platforms:
  debian    Debian/Ubuntu (apt-get)
  fedora    Fedora/RHEL (dnf)
  arch      Arch Linux (pacman)
  macos     macOS (Homebrew)
  windows   Windows (MSI/exe installers)

Categories (Linux only):
  core        Build tools (cmake, ninja, git, etc.)
  qt          Qt6 development libraries
  gstreamer   GStreamer multimedia
  sdl         SDL/USB libraries
  audio       Audio libraries
  misc        Vulkan, PipeWire

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
        "--platform",
        choices=["debian", "fedora", "arch", "macos", "windows"],
        help="Override platform detection",
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
        "--skip-system-packages",
        action="store_true",
        help="Skip apt package installation (Debian only), useful when another step pre-installs system packages",
    )
    parser.add_argument(
        "--gstreamer-version",
        help="GStreamer version to install (Windows only, overrides build-config.json)",
    )
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
    parser.add_argument(
        "--print-available-packages",
        action="store_true",
        help="Print available apt packages in the selected Debian category",
    )
    parser.add_argument(
        "--validate-extra-packages",
        nargs="*",
        default=None,
        help="Validate extra apt package names and print them back",
    )

    return parser.parse_args(args)


def main() -> int:
    """Main entry point."""
    args = parse_args()

    if args.list_packages:
        list_packages(args.platform)
        return 0

    platform = args.platform or _c.detect_platform()

    if args.print_packages:
        print_packages(platform or "debian", args.category)
        return 0

    if args.print_available_packages:
        if (platform or "debian") != "debian":
            _c.log_error("--print-available-packages is only supported for debian")
            return 1
        if not args.category:
            _c.log_error("--category is required with --print-available-packages")
            return 1
        print(" ".join(_c.get_available_debian_packages(args.category)))
        return 0

    if args.validate_extra_packages is not None:
        try:
            print(" ".join(validate_extra_packages(args.validate_extra_packages)))
        except ValueError as e:
            _c.log_error(f"{e}")
            return 1
        return 0

    if platform is None:
        _c.log_error("Could not detect platform")
        _c.log_info("Use --platform to specify: debian, fedora, arch, macos, windows")
        return 1

    print(f"Platform: {platform}")
    if args.dry_run:
        print("Mode: DRY RUN (no changes will be made)\n")

    if platform == "debian":
        success = install_debian(args.dry_run, args.category, args.skip_system_packages)
    elif platform == "fedora":
        success = install_fedora(args.dry_run, args.category, args.skip_system_packages)
    elif platform == "arch":
        success = install_arch(args.dry_run, args.category, args.skip_system_packages)
    elif platform == "macos":
        if args.category:
            _c.log_warn("--category is only supported for Debian")
        success = install_macos(args.dry_run)
    elif platform == "windows":
        success = install_windows(
            dry_run=args.dry_run,
            gstreamer_version=args.gstreamer_version,
            skip_gstreamer=args.skip_gstreamer,
            vulkan=args.vulkan,
            msvc=args.msvc or args.msvc_arm64,
            msvc_arm64=args.msvc_arm64,
            nsis=args.nsis,
        )
    else:
        _c.log_error(f"Unsupported platform: {platform}")
        return 1

    return 0 if success else 1


__all__ = ["list_packages", "main", "parse_args", "print_packages"]
