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
    --prefix DIR          Install prefix (default on GitHub Actions: $RUNNER_TEMP/gstreamer,
                          or C:\\gstreamer-<arch> for Windows; else <work-dir>/gst-<platform>-<arch>)
    --work-dir DIR        Working directory for source (default: system temp; $RUNNER_TEMP for
                          Windows on GitHub Actions)
    --qt-prefix DIR       Qt installation path (added to PKG_CONFIG_PATH; default for macOS and
                          Windows on GitHub Actions: $QT_ROOT_DIR)
    --jobs N              Parallel jobs (default: auto)
    --clean               Clean build directory before building
    --simulator           iOS simulator build (iOS only; arm64, x86_64 or universal)
    -h, --help            Show this help message

On GitHub Actions, the resolved arch and prefix are also written to $GITHUB_OUTPUT.
"""

from __future__ import annotations

import argparse
import os
import platform
import subprocess
import tempfile
from pathlib import Path

from common.build_config import get_gstreamer_version
from common.gh_actions import write_github_output
from common.logging import log_error, log_info, log_ok, log_warn
from common.proc import MissingToolError

from .cerbero import CerberoBuilder
from .config import BuildConfig
from .host import detect_host_arch, run_cmd
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
        help=(
            "Install prefix (default on GitHub Actions: $RUNNER_TEMP/gstreamer, or "
            "C:\\gstreamer-<arch> for Windows; else <work-dir>/gst-<platform>-<arch>)"
        ),
    )
    parser.add_argument(
        "--work-dir",
        "-w",
        default="",
        help="Working directory (default: system temp; $RUNNER_TEMP for Windows on GitHub Actions)",
    )
    parser.add_argument(
        "--qt-prefix",
        "-q",
        default="",
        help="Qt installation path (default for macOS and Windows on GitHub Actions: $QT_ROOT_DIR)",
    )
    parser.add_argument("--jobs", "-j", type=int, default=0, help="Parallel jobs (default: auto)")
    parser.add_argument("--clean", "-c", action="store_true", help="Clean build directory")
    parser.add_argument("--simulator", action="store_true", help="iOS simulator build")
    parser.add_argument(
        "--verify",
        action="store_true",
        help="After install, run gst-launch-1.0 --version against the install prefix",
    )

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


def ci_prefix(plat: str, arch: str) -> Path | None:
    """The default install prefix on a GitHub Actions runner; None elsewhere."""
    runner_temp = os.environ.get("RUNNER_TEMP")
    if not runner_temp:
        return None
    if plat == "windows":
        return Path(f"C:\\gstreamer-{arch}")
    return Path(runner_temp) / "gstreamer"


def ci_work_dir(plat: str) -> Path | None:
    """$RUNNER_TEMP for a Windows build on a GitHub Actions runner; None elsewhere (system temp)."""
    runner_temp = os.environ.get("RUNNER_TEMP")
    return Path(runner_temp) if runner_temp and plat == "windows" else None


def ci_qt_prefix(plat: str) -> Path | None:
    """The Qt that build-setup installed for a macOS or Windows build on GitHub Actions; else None."""
    qt_root = os.environ.get("QT_ROOT_DIR")
    if not os.environ.get("RUNNER_TEMP") or not qt_root or plat not in ("macos", "windows"):
        return None
    return Path(qt_root)


def verify_install(config: BuildConfig) -> None:
    """Run `gst-launch-1.0 --version` against the install prefix to confirm it runs."""
    if config.platform in ("android", "ios"):
        log_info(f"Skipping verify on cross-compiled target: {config.platform}")
        return

    prefix = config.prefix
    assert prefix is not None  # __post_init__ guarantees this
    is_windows = config.platform == "windows"
    bin_name = "gst-launch-1.0.exe" if is_windows else "gst-launch-1.0"
    bin_path = prefix / "bin" / bin_name

    if not bin_path.exists():
        raise RuntimeError(f"gst-launch binary missing at {bin_path}")

    env = os.environ.copy()
    if config.platform == "linux":
        # GStreamer installs into multiarch libdir on Debian/Ubuntu (e.g. lib/x86_64-linux-gnu).
        libdir = prefix / "lib" / f"{platform.machine()}-linux-gnu"
        env["LD_LIBRARY_PATH"] = str(libdir)
        env["GST_PLUGIN_PATH"] = str(libdir / "gstreamer-1.0")
    elif config.platform == "macos":
        env["DYLD_LIBRARY_PATH"] = str(prefix / "lib")
        env["GST_PLUGIN_PATH"] = str(prefix / "lib" / "gstreamer-1.0")
    elif is_windows:
        env["PATH"] = f"{prefix / 'bin'}{os.pathsep}{env.get('PATH', '')}"
        env["GST_PLUGIN_PATH"] = str(prefix / "lib" / "gstreamer-1.0")

    run_cmd([str(bin_path), "--version"], env=env)
    log_ok(f"verify: {bin_path} runs and reports a version")


def main() -> int:
    args = parse_args()

    # Resolve defaults
    version = args.version or get_gstreamer_version(args.platform, start=Path(__file__).resolve())
    if not version:
        log_error("GStreamer version is required; pass --version or provide build-config.json")
        return 1
    arch = resolve_arch(args.platform, args.arch)
    prefix = Path(args.prefix) if args.prefix else ci_prefix(args.platform, arch)
    work_dir = Path(args.work_dir) if args.work_dir else ci_work_dir(args.platform)
    work_dir = work_dir or Path(tempfile.gettempdir())
    qt_prefix = Path(args.qt_prefix) if args.qt_prefix else ci_qt_prefix(args.platform)
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
    write_github_output({"arch": config.arch, "prefix": str(config.prefix)})

    # Select builder
    if args.platform in ("linux", "macos", "windows"):
        builder = MesonBuilder(config)
    else:
        builder = CerberoBuilder(config)

    try:
        builder.run()
        if args.verify:
            verify_install(config)
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
