"""Debian/Ubuntu (apt) dependency installer."""

from __future__ import annotations

import re
from pathlib import Path

from . import _common as _c
from ._packages import OPTIONAL_DEBIAN_PACKAGES, get_packages

APT_DIR = Path("/etc/apt")


def universe_enabled(apt_dir: Path = APT_DIR) -> bool:
    """True when an apt source already lists Ubuntu's universe component."""
    sources = apt_dir / "sources.list.d"
    paths = [apt_dir / "sources.list", *sources.glob("*.list"), *sources.glob("*.sources")]
    for path in paths:
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        if re.search(r"^(deb|Components:).*universe", text, re.MULTILINE):
            return True
    return False


def enable_universe(dry_run: bool = False, apt_dir: Path = APT_DIR) -> bool:
    """Enable universe on Ubuntu without refreshing; the caller's next apt-get update does that.

    A no-op on plain Debian, which has no universe, and when a source already lists it.
    """
    if not _c.is_ubuntu() or universe_enabled(apt_dir):
        return True
    print("\nEnabling the universe repository...")
    if not _c.has_command("add-apt-repository") and not _c.run_apt_install_with_retry(
        ["software-properties-common"], dry_run, sudo=True
    ):
        return False
    return _c.run_command(["add-apt-repository", "-y", "-n", "universe"], dry_run, sudo=True)


def drop_unavailable_optional(packages: list[str], dry_run: bool = False) -> list[str]:
    """Remove OPTIONAL_DEBIAN_PACKAGES this release does not package; dry runs keep them."""
    if dry_run:
        return packages
    missing = {
        package
        for package in packages
        if package in OPTIONAL_DEBIAN_PACKAGES and not _c.check_apt_package_available(package)
    }
    for package in sorted(missing):
        _c.log_warn(f"Package {package} is not available on this release; skipping")
    return [package for package in packages if package not in missing]


def install_debian(dry_run: bool = False, category: str | None = None) -> bool:
    """Install Debian/Ubuntu dependencies."""
    _c.log_info("Installing Debian/Ubuntu dependencies...")

    packages = get_packages("debian", category)
    if not packages:
        print(f"Unknown category: {category}")
        return False

    # The full set includes universe packages; categories need only main.
    if not category and not enable_universe(dry_run):
        return False
    if not _c.run_command(_c.get_apt_update_command(), dry_run, sudo=True):
        return False

    packages = drop_unavailable_optional(packages, dry_run)
    print(f"\nInstalling {len(packages)} packages...")
    if not _c.run_apt_install_with_retry(packages, dry_run, sudo=True):
        return False

    if (not category or category in ("core", "android")) and not _c.install_build_tools(dry_run):
        return False

    if not category:
        print("\nCleaning up...")
        _c.run_command(["apt-get", "clean"], dry_run, sudo=True)

    print("\nDebian dependencies installed successfully!")
    return True


__all__ = [
    "drop_unavailable_optional",
    "enable_universe",
    "install_debian",
    "universe_enabled",
]
