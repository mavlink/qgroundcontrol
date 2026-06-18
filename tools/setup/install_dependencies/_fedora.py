"""Fedora/RHEL (dnf) dependency installer."""

from __future__ import annotations

from . import _common as _c
from ._packages import get_fedora_packages


def install_fedora(
    dry_run: bool = False,
    category: str | None = None,
    skip_system_packages: bool = False,
) -> bool:
    """Install Fedora/RHEL dependencies via dnf."""
    _c.log_info("Installing Fedora dependencies...")

    if not skip_system_packages:
        packages = get_fedora_packages(category)
        if category and not packages:
            print(f"Unknown category: {category}")
            return False
        print(f"\nInstalling {len(packages)} packages...")
        if not _c.run_dnf_install_with_retry(packages, dry_run, sudo=True):
            return False
    else:
        print("\nSkipping dnf package installation (--skip-system-packages)")

    if (not category or category == "core") and not _c.run_pipx_install(dry_run):
        return False

    if not skip_system_packages and not category:
        print("\nCleaning up...")
        _c.run_command(["dnf", "clean", "all"], dry_run, sudo=True)

    print("\nFedora dependencies installed successfully!")
    return True


__all__ = ["install_fedora"]
