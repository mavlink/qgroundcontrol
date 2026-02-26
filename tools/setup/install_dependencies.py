#!/usr/bin/env python3
"""
Install system dependencies for QGroundControl development.

Supports Debian/Ubuntu, macOS, and Windows. Auto-detects platform or allows override.

Usage:
    python tools/setup/install_dependencies.py           # Auto-detect platform
    python tools/setup/install_dependencies.py --dry-run # Show what would be installed
    python tools/setup/install_dependencies.py --list    # List packages by category
    python tools/setup/install_dependencies.py --platform debian  # Force platform
"""

from __future__ import annotations

import argparse
import json
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

# Package categories for Debian/Ubuntu
DEBIAN_PACKAGES: dict[str, list[str]] = {
    "core": [
        "software-properties-common",
        "gnupg2",
        "ca-certificates",
        "appstream",
        "binutils",
        "build-essential",
        "ccache",
        "cmake",
        "cppcheck",
        "file",
        "gdb",
        "gettext",
        "git",
        "libfuse2",
        "fuse3",
        "libtool",
        "locales",
        "mold",
        "ninja-build",
        "patchelf",
        "pipx",
        "pkgconf",
        "python3",
        "python3-pip",
        "rsync",
        "unzip",
        "valgrind",
        "wget",
        "zsync",
    ],
    "qt": [
        "libatspi2.0-dev",
        "libfontconfig1-dev",
        "libfreetype-dev",
        "libgtk-3-dev",
        "libsm-dev",
        "libx11-dev",
        "libx11-xcb-dev",
        "libxcb-cursor-dev",
        "libxcb-glx0-dev",
        "libxcb-icccm4-dev",
        "libxcb-image0-dev",
        "libxcb-keysyms1-dev",
        "libxcb-present-dev",
        "libxcb-randr0-dev",
        "libxcb-render-util0-dev",
        "libxcb-render0-dev",
        "libxcb-shape0-dev",
        "libxcb-shm0-dev",
        "libxcb-sync-dev",
        "libxcb-util-dev",
        "libxcb-xfixes0-dev",
        "libxcb-xinerama0-dev",
        "libxcb-xkb-dev",
        "libxcb1-dev",
        "libxext-dev",
        "libxfixes-dev",
        "libxi-dev",
        "libxkbcommon-dev",
        "libxkbcommon-x11-dev",
        "libxrender-dev",
        "libunwind-dev",
    ],
    "gstreamer": [
        "libgstreamer1.0-dev",
        "libgstreamer-plugins-bad1.0-dev",
        "libgstreamer-plugins-base1.0-dev",
        "libgstreamer-plugins-good1.0-dev",
        "libgstreamer-gl1.0-0",
        "gstreamer1.0-plugins-bad",
        "gstreamer1.0-plugins-base",
        "gstreamer1.0-plugins-good",
        "gstreamer1.0-plugins-ugly",
        "gstreamer1.0-plugins-rtp",
        "gstreamer1.0-gl",
        "gstreamer1.0-libav",
        "gstreamer1.0-rtsp",
        "gstreamer1.0-x",
        "libblas3",
        "libopenblas0",
        "python3-gi",
        "python3-gst-1.0",
    ],
    "gstreamer_optional": [
        "gstreamer1.0-qt6",
    ],
    "sdl": [
        "libusb-1.0-0-dev",
    ],
    "audio": [
        "libpulse-dev",
    ],
    "misc": [
        "libvulkan-dev",
        "libpipewire-0.3-dev",
    ],
}

# macOS packages (Homebrew)
MACOS_PACKAGES: list[str] = [
    "cmake",
    "ninja",
    "ccache",
    "git",
    "pkgconf",
    "create-dmg",
    "mold",
]

# Windows GStreamer
WINDOWS_GSTREAMER_BASE_URL = "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer/windows"
WINDOWS_GSTREAMER_INSTALL_DIR = "C:\\gstreamer"
WINDOWS_GSTREAMER_PREFIX = "C:\\gstreamer\\1.0\\msvc_x86_64"
WINDOWS_VULKAN_INSTALL_DIR = "C:\\VulkanSDK\\latest"
WINDOWS_VULKAN_URL = "https://sdk.lunarg.com/sdk/download/latest/windows/vulkan-sdk.exe"

# Pipx packages for Debian
PIPX_PACKAGES: list[str] = [
    "cmake",
    "ninja",
    "gcovr",
]

APT_BASE_OPTIONS: list[str] = [
    "-o", "DPkg::Lock::Timeout=300",
    "-o", "Acquire::Retries=3",
]


def get_repo_root() -> Path:
    """Find repository root directory."""
    current = Path(__file__).resolve()
    for parent in [current] + list(current.parents):
        if (parent / ".git").exists():
            return parent
    return Path.cwd()


def load_build_config() -> dict:
    """Load build configuration from .github/build-config.json."""
    config_path = get_repo_root() / ".github" / "build-config.json"
    if config_path.exists():
        try:
            with open(config_path) as f:
                return json.load(f)
        except json.JSONDecodeError as e:
            print(f"Error: invalid JSON in {config_path}: {e}", file=sys.stderr)
            return {}
    return {}


def get_config_value(key: str) -> str | None:
    """Get a top-level value from build config by key name."""
    config = load_build_config()
    value = config.get(key)
    return value if isinstance(value, str) else None


def detect_platform() -> str | None:
    """Detect current platform."""
    if sys.platform == "win32":
        return "windows"
    elif sys.platform == "darwin":
        return "macos"
    elif sys.platform.startswith("linux"):
        # Check for Debian-based
        if Path("/etc/debian_version").exists():
            return "debian"
        return "linux"
    return None


def has_command(cmd: str) -> bool:
    """Check if a command is available."""
    return shutil.which(cmd) is not None


def get_debian_packages(
    category: str | None = None,
    include_optional: bool = False,
) -> list[str]:
    """Get Debian packages, optionally filtered by category."""
    if category:
        return list(DEBIAN_PACKAGES.get(category, []))

    packages = []
    for cat, pkgs in DEBIAN_PACKAGES.items():
        if cat.endswith("_optional") and not include_optional:
            continue
        packages.extend(pkgs)
    return list(dict.fromkeys(packages))  # Remove duplicates, preserve order


def get_macos_packages() -> list[str]:
    """Get macOS Homebrew packages."""
    return list(MACOS_PACKAGES)


def get_apt_install_command(packages: list[str]) -> list[str]:
    """Build apt-get install command."""
    return [
        "apt-get",
        *APT_BASE_OPTIONS,
        "install",
        "-y",
        "-qq",
        "--no-install-recommends",
    ] + packages


def get_apt_update_command() -> list[str]:
    """Build apt-get update command."""
    return [
        "apt-get",
        *APT_BASE_OPTIONS,
        "update",
        "-y",
        "-qq",
    ]


def run_apt_install_with_retry(
    packages: list[str],
    dry_run: bool = False,
    sudo: bool = False,
    max_attempts: int = 2,
) -> bool:
    """Install apt packages, refreshing package lists between attempts."""
    install_cmd = get_apt_install_command(packages)
    for attempt in range(1, max_attempts + 1):
        if run_command(install_cmd, dry_run, sudo=sudo):
            return True
        if attempt >= max_attempts:
            break
        print(
            f"  apt install failed (attempt {attempt}/{max_attempts}); "
            "refreshing package lists and retrying..."
        )
        if not run_command(get_apt_update_command(), dry_run, sudo=sudo):
            return False
    return False


def get_brew_install_command(packages: list[str]) -> list[str]:
    """Build brew install command."""
    return ["brew", "install"] + packages


def check_apt_package_available(package: str) -> bool:
    """Check if an apt package is available."""
    result = subprocess.run(
        ["apt-cache", "show", package],
        capture_output=True,
    )
    return result.returncode == 0


def get_gstreamer_macos_urls(version: str) -> tuple[str, str]:
    """Get GStreamer download URLs for macOS."""
    base_url = f"https://gstreamer.freedesktop.org/data/pkg/osx/{version}"
    runtime = f"{base_url}/gstreamer-1.0-{version}-universal.pkg"
    devel = f"{base_url}/gstreamer-1.0-devel-{version}-universal.pkg"
    return runtime, devel


def run_command(cmd: list[str], dry_run: bool = False, sudo: bool = False) -> bool:
    """Run a command, optionally with sudo."""
    # os.geteuid is not available on Windows; treat missing API as non-root.
    is_root = hasattr(os, "geteuid") and os.geteuid() == 0
    if sudo and not is_root:
        cmd = ["sudo"] + cmd

    if dry_run:
        print(f"  Would run: {shlex.join(cmd)}")
        return True

    print(f"  Running: {shlex.join(cmd[:5])}{'...' if len(cmd) > 5 else ''}")
    result = subprocess.run(cmd)
    return result.returncode == 0


def is_ci() -> bool:
    """Check if running in a CI environment."""
    return os.environ.get("CI") == "true" or os.environ.get("GITHUB_ACTIONS") == "true"


def _set_env_var_ci(name: str, value: str) -> None:
    """Set an environment variable via GITHUB_ENV for CI."""
    github_env = os.environ.get("GITHUB_ENV", "")
    if github_env:
        with open(github_env, "a", encoding="utf-8") as f:
            f.write(f"{name}={value}\n")
    os.environ[name] = value


def _set_env_var_local(name: str, value: str) -> None:
    """Set a machine-level environment variable via Windows registry."""
    if sys.platform != "win32":
        raise RuntimeError("Local env var persistence is only supported on Windows")
    import winreg

    key_path = r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment"
    with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, key_path, 0, winreg.KEY_SET_VALUE) as key:
        winreg.SetValueEx(key, name, 0, winreg.REG_EXPAND_SZ, value)


def set_env_var(name: str, value: str) -> None:
    """Set an environment variable (CI or local)."""
    if is_ci():
        _set_env_var_ci(name, value)
    else:
        _set_env_var_local(name, value)


def add_to_path(path_entry: str) -> None:
    """Add a directory to the system PATH (CI or local)."""
    if is_ci():
        github_path = os.environ.get("GITHUB_PATH", "")
        if github_path:
            with open(github_path, "a", encoding="utf-8") as f:
                f.write(f"{path_entry}\n")
    else:
        if sys.platform != "win32":
            raise RuntimeError("Local PATH persistence is only supported on Windows")
        import winreg

        key_path = r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment"
        with winreg.OpenKey(
            winreg.HKEY_LOCAL_MACHINE, key_path, 0,
            winreg.KEY_QUERY_VALUE | winreg.KEY_SET_VALUE,
        ) as key:
            current, _ = winreg.QueryValueEx(key, "Path")
            if path_entry not in current:
                winreg.SetValueEx(key, "Path", 0, winreg.REG_EXPAND_SZ, f"{current};{path_entry}")


def download_file(
    url: str,
    dest: Path,
    dry_run: bool = False,
    timeout: float = 300.0,
    retries: int = 3,
) -> bool:
    """Download a file from a URL."""
    import urllib.error
    import urllib.request

    if dry_run:
        print(f"  Would download: {url} -> {dest.name}")
        return True

    req = urllib.request.Request(url, headers={"User-Agent": "qgc-deps-installer/1.0"})
    for attempt in range(1, retries + 1):
        print(f"  Downloading {dest.name} (attempt {attempt}/{retries})...")
        try:
            with urllib.request.urlopen(req, timeout=timeout) as response, open(dest, "wb") as out:
                shutil.copyfileobj(response, out)
            return True
        except (TimeoutError, urllib.error.URLError, OSError) as e:
            if attempt == retries:
                print(f"Failed to download {url}: {e}", file=sys.stderr)
                return False
            backoff_seconds = attempt * 2
            print(
                f"  Download attempt {attempt} failed for {dest.name}: {e}. "
                f"Retrying in {backoff_seconds}s...",
                file=sys.stderr,
            )
            time.sleep(backoff_seconds)
    return False


def install_debian(
    dry_run: bool = False,
    category: str | None = None,
    skip_system_packages: bool = False,
) -> bool:
    """Install Debian/Ubuntu dependencies."""
    print("Installing Debian/Ubuntu dependencies...")

    if not skip_system_packages:
        # Update package lists
        apt_update_cmd = get_apt_update_command()
        if not run_command(apt_update_cmd, dry_run, sudo=True):
            return False

        bootstrap_packages = ["software-properties-common", "gnupg2", "ca-certificates"]

        # Ensure "universe" is enabled before installing full package set.
        if not category:
            print("\nInstalling bootstrap packages...")
            if not run_apt_install_with_retry(bootstrap_packages, dry_run, sudo=True):
                return False
            if not run_command(["add-apt-repository", "-y", "universe"], dry_run, sudo=True):
                return False
            if not run_command(apt_update_cmd, dry_run, sudo=True):
                return False

        # Get packages
        if category:
            packages = get_debian_packages(category)
            if not packages:
                print(f"Unknown category: {category}")
                return False
        else:
            packages = get_debian_packages()
            packages = [pkg for pkg in packages if pkg not in bootstrap_packages]

        # Install main packages
        print(f"\nInstalling {len(packages)} packages...")
        if not run_apt_install_with_retry(packages, dry_run, sudo=True):
            return False

        # Check for optional packages
        for pkg in DEBIAN_PACKAGES.get("gstreamer_optional", []):
            if check_apt_package_available(pkg):
                print(f"\nInstalling optional package: {pkg}")
                run_apt_install_with_retry([pkg], dry_run, sudo=True)
            else:
                print(f"\nSkipping optional package (not available): {pkg}")
    else:
        print("\nSkipping apt package installation (--skip-system-packages)")

    # Install pipx packages
    if not category or category == "core":
        print("\nInstalling pipx packages...")
        run_command(["pipx", "ensurepath"], dry_run)
        for pkg in PIPX_PACKAGES:
            if not run_command(["pipx", "install", pkg], dry_run):
                print(f"Error: Failed to install pipx package: {pkg}", file=sys.stderr)
                return False

    # Cleanup
    if not skip_system_packages and not category:
        print("\nCleaning up...")
        run_command(["apt-get", "clean"], dry_run, sudo=True)

    print("\nDebian dependencies installed successfully!")
    return True


def install_macos(dry_run: bool = False) -> bool:
    """Install macOS dependencies."""
    print("Installing macOS dependencies...")

    # Check/install Homebrew
    if not has_command("brew"):
        print("\nInstalling Homebrew...")
        if not dry_run:
            homebrew_url = "https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh"
            dl = subprocess.run(
                ["curl", "-fsSL", homebrew_url],
                capture_output=True,
            )
            if dl.returncode != 0:
                print("Failed to download Homebrew installer", file=sys.stderr)
                return False
            result = subprocess.run(
                ["/bin/bash"],
                input=dl.stdout,
            )
            if result.returncode != 0:
                print("Failed to install Homebrew", file=sys.stderr)
                return False

    # Update Homebrew
    run_command(["brew", "update"], dry_run)

    # Install packages
    packages = get_macos_packages()
    print(f"\nInstalling {len(packages)} Homebrew packages...")
    if not run_command(get_brew_install_command(packages), dry_run):
        return False

    # Install GStreamer
    gst_version = get_config_value("gstreamer_macos_version")
    if not gst_version:
        print("\nWarning: GSTREAMER_MACOS_VERSION not found in build-config.json")
        print("Skipping GStreamer installation")
    else:
        print(f"\nInstalling GStreamer {gst_version}...")
        runtime_url, devel_url = get_gstreamer_macos_urls(gst_version)

        if dry_run:
            print(f"  Would download: {runtime_url}")
            print(f"  Would download: {devel_url}")
            print("  Would run: sudo installer -pkg ... -target /")
        else:
            with tempfile.TemporaryDirectory() as tmpdir:
                runtime_pkg = Path(tmpdir) / runtime_url.split("/")[-1]
                devel_pkg = Path(tmpdir) / devel_url.split("/")[-1]
                download_targets = [
                    ("runtime", runtime_url, runtime_pkg),
                    ("devel", devel_url, devel_pkg),
                ]

                print("  Downloading GStreamer packages in parallel...")
                with ThreadPoolExecutor(max_workers=2) as executor:
                    futures = {
                        executor.submit(download_file, url, pkg_path): label
                        for label, url, pkg_path in download_targets
                    }
                    for future in as_completed(futures):
                        label = futures[future]
                        if not future.result():
                            print(f"Failed to download GStreamer {label} package", file=sys.stderr)
                            return False

                for label, _, pkg_path in download_targets:
                    print(f"  Installing GStreamer {label} package...")
                    result = subprocess.run(
                        ["sudo", "installer", "-pkg", str(pkg_path), "-target", "/"],
                    )
                    if result.returncode != 0:
                        print(f"Failed to install GStreamer {label} package", file=sys.stderr)
                        return False

    print("\nmacOS dependencies installed successfully!")
    return True


def install_windows_gstreamer(version: str, dry_run: bool = False) -> bool:
    """Install GStreamer on Windows (AMD64 only)."""
    arch = os.environ.get("PROCESSOR_ARCHITECTURE", "")
    if arch != "AMD64":
        print(f"Skipping GStreamer: only supported on AMD64 (detected: {arch or 'unknown'})")
        return True

    base_url = f"{WINDOWS_GSTREAMER_BASE_URL}/{version}"
    runtime_name = f"gstreamer-1.0-msvc-x86_64-{version}.msi"
    devel_name = f"gstreamer-1.0-devel-msvc-x86_64-{version}.msi"

    print(f"\nInstalling GStreamer {version}...")
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        runtime_msi = tmp / runtime_name
        devel_msi = tmp / devel_name

        download_targets = [
            ("runtime", f"{base_url}/{runtime_name}", runtime_msi),
            ("devel", f"{base_url}/{devel_name}", devel_msi),
        ]
        if dry_run:
            for _, url, path in download_targets:
                if not download_file(url, path, dry_run):
                    return False
        else:
            print("  Downloading GStreamer installers in parallel...")
            with ThreadPoolExecutor(max_workers=2) as executor:
                futures = {
                    executor.submit(download_file, url, path): label
                    for label, url, path in download_targets
                }
                for future in as_completed(futures):
                    label = futures[future]
                    if not future.result():
                        print(
                            f"Failed to download GStreamer {label} installer",
                            file=sys.stderr,
                        )
                        return False

        install_dir = WINDOWS_GSTREAMER_INSTALL_DIR
        for label, msi in [("runtime", runtime_msi), ("devel", devel_msi)]:
            print(f"  Installing GStreamer {label}...")
            if not run_command([
                "msiexec.exe", "/i", str(msi),
                "/passive", f"INSTALLDIR={install_dir}", "ADDLOCAL=ALL",
            ], dry_run):
                return False

    prefix = WINDOWS_GSTREAMER_PREFIX
    set_env_var("GSTREAMER_1_0_ROOT_MSVC_X86_64", prefix)
    set_env_var("GSTREAMER_1_0_ROOT_X86_64", prefix)
    add_to_path(f"{prefix}\\bin")
    print(f"GStreamer {version} installed to {prefix}")
    return True


def install_windows_vulkan(dry_run: bool = False) -> bool:
    """Install Vulkan SDK on Windows."""
    print("\nInstalling Vulkan SDK...")
    install_dir = WINDOWS_VULKAN_INSTALL_DIR
    with tempfile.TemporaryDirectory() as tmpdir:
        installer = Path(tmpdir) / "vulkan-sdk.exe"
        if not download_file(WINDOWS_VULKAN_URL, installer, dry_run):
            return False

        if not run_command([
            str(installer),
            "--root", install_dir,
            "--accept-licenses", "--default-answer", "--confirm-command",
            "install",
            "com.lunarg.vulkan.glm", "com.lunarg.vulkan.volk",
            "com.lunarg.vulkan.vma", "com.lunarg.vulkan.debug",
        ], dry_run):
            return False

    set_env_var("VULKAN_SDK", install_dir)
    add_to_path(f"{install_dir}\\Bin")
    print("Vulkan SDK installed")
    return True


def install_windows(
    dry_run: bool = False,
    gstreamer_version: str | None = None,
    skip_gstreamer: bool = False,
    vulkan: bool = False,
) -> bool:
    """Install Windows dependencies."""
    print("Installing Windows dependencies...")
    arch = os.environ.get("PROCESSOR_ARCHITECTURE", "unknown")
    print(f"Architecture: {arch}")

    if not skip_gstreamer:
        version = gstreamer_version or get_config_value("gstreamer_windows_version")
        if not version:
            print(
                "Error: GStreamer version not found. "
                "Use --gstreamer-version or check build-config.json",
                file=sys.stderr,
            )
            return False
        if not install_windows_gstreamer(version, dry_run):
            return False

    if vulkan:
        if not install_windows_vulkan(dry_run):
            return False

    print("\nWindows dependencies installed!")
    return True


def print_packages(platform: str, category: str | None = None) -> None:
    """Print packages as a single space-separated line (machine-readable)."""
    if platform == "debian":
        print(" ".join(get_debian_packages(category)))
    elif platform == "macos":
        print(" ".join(get_macos_packages()))
    else:
        print(f"Error: --print-packages not supported for {platform}", file=sys.stderr)
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
        print(f"\nPipx packages:")
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
  macos     macOS (Homebrew)
  windows   Windows (MSI/exe installers)

Categories (Debian only):
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
        "--dry-run",
        action="store_true",
        help="Show what would be installed without installing",
    )
    parser.add_argument(
        "--platform",
        choices=["debian", "macos", "windows"],
        help="Override platform detection",
    )
    parser.add_argument(
        "--list",
        dest="list_packages",
        action="store_true",
        help="List packages by category",
    )
    parser.add_argument(
        "--print-packages",
        action="store_true",
        help="Print space-separated package list (machine-readable, for CI caching)",
    )
    parser.add_argument(
        "--category",
        help="Install only specific category (Debian only)",
    )
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
        "--skip-gstreamer",
        action="store_true",
        help="Skip GStreamer installation (Windows only)",
    )
    parser.add_argument(
        "--vulkan",
        action="store_true",
        help="Install Vulkan SDK (Windows only)",
    )

    return parser.parse_args(args)


def main() -> int:
    """Main entry point."""
    args = parse_args()

    if args.list_packages:
        list_packages(args.platform)
        return 0

    platform = args.platform or detect_platform()

    if args.print_packages:
        print_packages(platform or "debian", args.category)
        return 0

    if platform is None:
        print("Error: Could not detect platform", file=sys.stderr)
        print("Use --platform to specify: debian, macos, windows", file=sys.stderr)
        return 1

    print(f"Platform: {platform}")
    if args.dry_run:
        print("Mode: DRY RUN (no changes will be made)\n")

    if platform == "debian":
        success = install_debian(args.dry_run, args.category, args.skip_system_packages)
    elif platform == "macos":
        if args.category:
            print("Warning: --category is only supported for Debian", file=sys.stderr)
        success = install_macos(args.dry_run)
    elif platform == "windows":
        success = install_windows(
            args.dry_run, args.gstreamer_version, args.skip_gstreamer, args.vulkan,
        )
    else:
        print(f"Error: Unsupported platform: {platform}", file=sys.stderr)
        return 1

    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
