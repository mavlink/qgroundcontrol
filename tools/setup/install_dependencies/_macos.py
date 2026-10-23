"""macOS installer (Homebrew + GStreamer .pkg downloads)."""

from __future__ import annotations

import os
import subprocess
import tempfile
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

from . import _common as _c
from ._packages import get_packages

HOMEBREW_INSTALL_URL = "https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh"
# GStreamer's framework packages: install label -> common.gstreamer package key.
GSTREAMER_PACKAGES = {"runtime": "macos", "devel": "macos_devel"}


def _install_homebrew() -> bool:
    with tempfile.TemporaryDirectory() as tmpdir:
        script = Path(tmpdir) / "install.sh"
        if not _c.download_file(HOMEBREW_INSTALL_URL, script):
            return False
        # Read from stdin, the installer runs non-interactively.
        if subprocess.run(["/bin/bash"], input=script.read_bytes()).returncode != 0:
            _c.log_error("Failed to install Homebrew")
            return False
    return True


def install_macos(dry_run: bool = False, packages: list[str] | None = None) -> bool:
    """Install macOS dependencies: Homebrew packages (default: the full list) and GStreamer."""
    _c.log_info("Installing macOS dependencies...")

    if not _c.has_command("brew"):
        print("\nInstalling Homebrew...")
        if not dry_run and not _install_homebrew():
            return False

    # CI runner images are refreshed weekly; `brew update` adds 30-60s for nothing.
    if os.environ.get("HOMEBREW_NO_UPDATE") != "1" and not _c.is_ci():
        _c.run_command(["brew", "update"], dry_run)

    packages = get_packages("macos") if packages is None else packages
    print(f"\nInstalling {len(packages)} Homebrew packages...")
    if not _c.run_command(_c.get_brew_install_command(packages), dry_run):
        return False

    gst_version = _c.get_build_config_value("gstreamer.version.macos")
    macos_gst_root = Path("/Library/Frameworks/GStreamer.framework")
    if not gst_version:
        print()
        _c.log_warn("GSTREAMER_MACOS_VERSION not found in build-config.json")
        _c.log_warn("Skipping GStreamer installation")
    elif macos_gst_root.exists():
        print(f"GStreamer already installed at {macos_gst_root}; skipping")
    else:
        print(f"\nInstalling GStreamer {gst_version}...")
        checksums = {
            label: _c.pinned_checksum(gst_version, package) or ""
            for label, package in GSTREAMER_PACKAGES.items()
        }
        if not all(checksums.values()):
            _c.log_error(f"Missing GStreamer {gst_version} macOS checksums")
            return False

        with tempfile.TemporaryDirectory() as tmpdir:
            downloads = {
                label: (
                    # Upstream first, then QGC's mirror, like cmake/GStreamer/Download.cmake.
                    (_c.package_url(package, gst_version), _c.mirror_url(package, gst_version)),
                    Path(tmpdir) / _c.package_filename(package, gst_version),
                )
                for label, package in GSTREAMER_PACKAGES.items()
            }
            print("  Downloading GStreamer packages in parallel...")
            with ThreadPoolExecutor(max_workers=len(downloads)) as executor:
                futures = {
                    executor.submit(
                        _c.download_first, urls, pkg_path, checksums[label], dry_run
                    ): label
                    for label, (urls, pkg_path) in downloads.items()
                }
                for future in as_completed(futures):
                    if not future.result():
                        _c.log_error(f"Failed to download GStreamer {futures[future]} package")
                        return False

            for label, (_, pkg_path) in downloads.items():
                print(f"  Installing GStreamer {label} package...")
                if not _c.run_command(
                    ["installer", "-pkg", str(pkg_path), "-target", "/"], dry_run, sudo=True
                ):
                    _c.log_error(f"Failed to install GStreamer {label} package")
                    return False

    print("\nmacOS dependencies installed successfully!")
    return True


__all__ = ["GSTREAMER_PACKAGES", "HOMEBREW_INSTALL_URL", "install_macos"]
