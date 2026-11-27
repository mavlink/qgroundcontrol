"""macOS installer (Homebrew packages)."""

from __future__ import annotations

import os
import subprocess
import tempfile
from pathlib import Path

from . import _common as _c
from ._packages import get_packages

HOMEBREW_INSTALL_URL = "https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh"


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
    """Install the Homebrew packages (default: the full list).

    CMake downloads the GStreamer SDK at configure time.
    """
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

    print("\nmacOS dependencies installed successfully!")
    return True


__all__ = ["HOMEBREW_INSTALL_URL", "install_macos"]
