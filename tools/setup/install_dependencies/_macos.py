"""macOS installer (Homebrew + GStreamer .pkg downloads)."""

from __future__ import annotations

import os
import subprocess
import tempfile
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

from . import _common as _c
from ._packages import get_macos_packages


def get_gstreamer_macos_urls(version: str) -> tuple[str, str]:
    """Get GStreamer download URLs for macOS."""
    base_url = f"https://gstreamer.freedesktop.org/data/pkg/osx/{version}"
    runtime = f"{base_url}/gstreamer-1.0-{version}-universal.pkg"
    devel = f"{base_url}/gstreamer-1.0-devel-{version}-universal.pkg"
    return runtime, devel

def install_macos(dry_run: bool = False) -> bool:
    """Install macOS dependencies."""
    _c.log_info("Installing macOS dependencies...")

    if not _c.has_command("brew"):
        print("\nInstalling Homebrew...")
        if not dry_run:
            homebrew_url = "https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh"
            dl = subprocess.run(
                ["curl", "-fsSL", homebrew_url],
                capture_output=True,
            )
            if dl.returncode != 0:
                _c.log_error("Failed to download Homebrew installer")
                return False
            result = subprocess.run(
                ["/bin/bash"],
                input=dl.stdout,
            )
            if result.returncode != 0:
                _c.log_error("Failed to install Homebrew")
                return False

    # CI runner images are refreshed weekly; `brew update` adds 30-60s for nothing.
    if os.environ.get("HOMEBREW_NO_UPDATE") != "1" and not _c.is_ci():
        _c.run_command(["brew", "update"], dry_run)

    packages = get_macos_packages()
    print(f"\nInstalling {len(packages)} Homebrew packages...")
    if not _c.run_command(_c.get_brew_install_command(packages), dry_run):
        return False

    gst_version = _c.get_config_value("gstreamer.version.macos")
    macos_gst_root = Path("/Library/Frameworks/GStreamer.framework")
    if not gst_version:
        print()
        _c.log_warn("GSTREAMER_MACOS_VERSION not found in build-config.json")
        _c.log_warn("Skipping GStreamer installation")
    elif macos_gst_root.exists():
        print(f"GStreamer already installed at {macos_gst_root}; skipping")
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
                        executor.submit(_c.download_file, url, pkg_path): label
                        for label, url, pkg_path in download_targets
                    }
                    for future in as_completed(futures):
                        label = futures[future]
                        if not future.result():
                            _c.log_error(f"Failed to download GStreamer {label} package")
                            return False

                for label, _, pkg_path in download_targets:
                    print(f"  Installing GStreamer {label} package...")
                    result = subprocess.run(
                        ["sudo", "installer", "-pkg", str(pkg_path), "-target", "/"],
                    )
                    if result.returncode != 0:
                        _c.log_error(f"Failed to install GStreamer {label} package")
                        return False

    print("\nmacOS dependencies installed successfully!")
    return True

__all__ = ["get_gstreamer_macos_urls", "install_macos"]
