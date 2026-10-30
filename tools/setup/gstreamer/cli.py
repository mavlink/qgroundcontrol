"""
Unified GStreamer build script for QGroundControl.

Builds GStreamer with plugins needed for QGC video streaming across all platforms:
- Linux, macOS, Windows: Meson-based builds
- Android, iOS: Cerbero-based builds

Usage:
    ./build_gstreamer.py --platform linux [OPTIONS]
    ./build_gstreamer.py --platform macos --arch universal [OPTIONS]
    ./build_gstreamer.py --platform android --arch arm64 [OPTIONS]

Options:
    --platform PLATFORM   Target platform (linux, macos, windows, android, ios)
    --arch ARCH           Target architecture (platform-specific, default: native;
                          Windows names x86_64 'x64')
    --version VERSION     GStreamer version (default: the platform's in build-config.json)
    --type TYPE           Build type: release or debug (default: release)
    --prefix DIR          Install prefix (default: <work-dir>/gst-<platform>-<arch>)
    --work-dir DIR        Working directory for source (default: system temp)
    --qt-prefix DIR       Qt installation path (added to PKG_CONFIG_PATH)
    --jobs N              Parallel jobs (default: auto)
    --clean               Clean build directory before building
    --simulator           iOS simulator build (iOS only; arm64, x86_64 or universal)
    -h, --help            Show this help message
"""

from __future__ import annotations

import argparse
import subprocess
import tempfile
from pathlib import Path

from common.build_config import get_gstreamer_version
from common.logging import log_error, log_warn
from common.proc import MissingToolError

from .cerbero import CerberoBuilder
from .config import BuildConfig
from .host import detect_host_arch
from .meson import MesonBuilder


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build GStreamer for QGroundControl",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )

    parser.add_argument(
        "--platform",
        "-p",
        required=True,
        choices=["linux", "macos", "windows", "android", "ios"],
        help="Target platform",
    )
    parser.add_argument(
        "--arch",
        "-a",
        default="",
        help="Target architecture (default: native or universal; Windows names x86_64 'x64')",
    )
    parser.add_argument(
        "--version",
        "-v",
        default="",
        help="GStreamer version (default: the platform's in build-config.json)",
    )
    parser.add_argument(
        "--type",
        "-t",
        dest="build_type",
        default="release",
        choices=["release", "debug"],
        help="Build type (default: release)",
    )
    parser.add_argument(
        "--prefix",
        default="",
        help="Install prefix (default: <work-dir>/gst-<platform>-<arch>)",
    )
    parser.add_argument(
        "--work-dir",
        "-w",
        default="",
        help="Working directory (default: system temp)",
    )
    parser.add_argument("--qt-prefix", "-q", default="", help="Qt installation path")
    parser.add_argument("--jobs", "-j", type=int, default=0, help="Parallel jobs (default: auto)")
    parser.add_argument("--clean", "-c", action="store_true", help="Clean build directory")
    parser.add_argument("--simulator", action="store_true", help="iOS simulator build")

    return parser.parse_args()


def get_default_arch(plat: str) -> str:
    """Get default architecture for platform."""
    if plat in ("macos", "android", "ios"):
        return "universal"
    return detect_host_arch()


def resolve_arch(plat: str, arch: str) -> str:
    """The requested or default arch; the Windows toolchain names x86_64 'x64'."""
    arch = arch or get_default_arch(plat)
    return "x64" if plat == "windows" and arch == "x86_64" else arch


def main() -> int:
    args = parse_args()

    # Resolve defaults
    version = args.version or get_gstreamer_version(args.platform, start=Path(__file__).resolve())
    if not version:
        log_error("GStreamer version is required; pass --version or provide build-config.json")
        return 1
    arch = resolve_arch(args.platform, args.arch)
    prefix = Path(args.prefix) if args.prefix else None
    work_dir = Path(args.work_dir) if args.work_dir else Path(tempfile.gettempdir())
    qt_prefix = Path(args.qt_prefix) if args.qt_prefix else None
    jobs = args.jobs if args.jobs > 0 else None

    config = BuildConfig(
        platform=args.platform,
        arch=arch,
        version=version,
        build_type=args.build_type,
        prefix=prefix,
        work_dir=work_dir,
        qt_prefix=qt_prefix,
        jobs=jobs,
        clean=args.clean,
        simulator=args.simulator,
    )

    # Select builder
    if args.platform in ("linux", "macos", "windows"):
        builder = MesonBuilder(config)
    else:
        builder = CerberoBuilder(config)

    try:
        builder.run()
        return 0
    except subprocess.CalledProcessError as e:
        log_error(f"Build failed: {e}")
        return 1
    except (RuntimeError, ValueError, MissingToolError) as e:
        log_error(str(e))
        return 1
    except KeyboardInterrupt:
        log_warn("Build cancelled")
        return 130
