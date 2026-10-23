"""Fedora/RHEL (dnf) and Arch Linux (pacman) installers; Debian has its own in _debian."""

from __future__ import annotations

from typing import TYPE_CHECKING, NamedTuple

from . import _common as _c
from ._packages import get_packages

if TYPE_CHECKING:
    from collections.abc import Callable


class PackageManager(NamedTuple):
    label: str
    install: Callable[..., bool]
    clean: list[str]
    refresh: list[str] | None = None


PACKAGE_MANAGERS: dict[str, PackageManager] = {
    "fedora": PackageManager("Fedora", _c.run_dnf_install_with_retry, ["dnf", "clean", "all"]),
    "arch": PackageManager(
        "Arch Linux",
        _c.run_pacman_install_with_retry,
        ["pacman", "-Sc", "--noconfirm"],
        # -Syu not -Sy: a partial sync against a stale DB is an Arch partial-upgrade footgun.
        refresh=["pacman", "-Syu", "--noconfirm"],
    ),
}


def install_linux(platform: str, dry_run: bool = False, category: str | None = None) -> bool:
    """Install platform's packages (default: all but opt-in categories), then the build tools."""
    manager = PACKAGE_MANAGERS[platform]
    _c.log_info(f"Installing {manager.label} dependencies...")

    packages = get_packages(platform, category)
    if not packages:
        print(f"Unknown category: {category}")
        return False
    if manager.refresh and not _c.run_command(manager.refresh, dry_run, sudo=True):
        return False
    print(f"\nInstalling {len(packages)} packages...")
    if not manager.install(packages, dry_run, sudo=True):
        return False

    if (not category or category == "core") and not _c.install_build_tools(dry_run):
        return False

    if not category:
        print("\nCleaning up...")
        _c.run_command(manager.clean, dry_run, sudo=True)

    print(f"\n{manager.label} dependencies installed successfully!")
    return True


__all__ = ["PACKAGE_MANAGERS", "PackageManager", "install_linux"]
