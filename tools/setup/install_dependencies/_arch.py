"""Arch Linux (pacman) dependency installer."""

from __future__ import annotations

from . import _common as _c
from ._packages import get_arch_packages


def install_arch(
    dry_run: bool = False,
    category: str | None = None,
    skip_system_packages: bool = False,
) -> bool:
    """Install Arch Linux dependencies via pacman."""
    _c.log_info("Installing Arch Linux dependencies...")

    if not skip_system_packages:
        # -Syu not -Sy: a partial sync against a stale DB is an Arch partial-upgrade footgun.
        if not _c.run_command(["pacman", "-Syu", "--noconfirm"], dry_run, sudo=True):
            return False
        packages = get_arch_packages(category)
        if category and not packages:
            print(f"Unknown category: {category}")
            return False
        print(f"\nInstalling {len(packages)} packages...")
        if not _c.run_pacman_install_with_retry(packages, dry_run, sudo=True):
            return False
    else:
        print("\nSkipping pacman package installation (--skip-system-packages)")

    if (not category or category == "core") and not _c.run_pipx_install(dry_run):
        return False

    if not skip_system_packages and not category:
        print("\nCleaning up...")
        _c.run_command(["pacman", "-Sc", "--noconfirm"], dry_run, sudo=True)

    print("\nArch dependencies installed successfully!")
    return True


__all__ = ["install_arch"]
