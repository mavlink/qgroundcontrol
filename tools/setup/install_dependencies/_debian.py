"""Debian/Ubuntu installer + the just-binary fallback path."""

from __future__ import annotations

import platform
import re
import subprocess
import tarfile
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from . import _common as _c
from ._packages import DEBIAN_PACKAGES, PIPX_PACKAGES, get_debian_packages

JUST_VERSION = "1.51.0"
JUST_MIN_VERSION = (1, 30)
JUST_TARGETS: dict[str, str] = {
    "x86_64": "x86_64-unknown-linux-musl",
    "aarch64": "aarch64-unknown-linux-musl",
}

DEBIAN_PACKAGE_ALTERNATIVES: dict[str, list[str]] = {
    "libgstreamer-plugins-good1.0-dev": ["libgstreamer-plugins-extra1.0-dev"],
}


def resolve_package_alternatives(packages: list[str]) -> list[str]:
    """Replace packages with their first available alternative when the primary is missing."""
    needs_check = {pkg for pkg in packages if pkg in DEBIAN_PACKAGE_ALTERNATIVES}
    if not needs_check:
        return packages

    all_candidates = sorted(
        {name for pkg in needs_check for name in [pkg, *DEBIAN_PACKAGE_ALTERNATIVES[pkg]]}
    )
    with ThreadPoolExecutor() as pool:
        availability = dict(
            zip(
                all_candidates,
                pool.map(_c.check_apt_package_available, all_candidates),
                strict=False,
            )
        )

    resolved = []
    for pkg in packages:
        if pkg not in needs_check:
            resolved.append(pkg)
            continue
        chosen = pkg
        if not availability.get(pkg):
            for alt in DEBIAN_PACKAGE_ALTERNATIVES[pkg]:
                if availability.get(alt):
                    print(f"  Package {pkg} not available; using {alt}")
                    chosen = alt
                    break
        resolved.append(chosen)
    return resolved


def _detect_just_version() -> tuple[int, ...] | None:
    """Return installed just's (major, minor, patch) tuple, or None if absent/unparsable."""
    if not _c.has_command("just"):
        return None
    try:
        out = subprocess.run(
            ["just", "--version"], capture_output=True, text=True, check=True, timeout=5
        ).stdout
    except (subprocess.SubprocessError, OSError):
        return None
    match = re.search(r"(\d+)\.(\d+)(?:\.(\d+))?", out)
    if not match:
        return None
    return tuple(int(g) for g in match.groups(default="0"))


def install_just_debian(dry_run: bool = False) -> bool:
    """Install `just` via apt when available, else from upstream prebuilt binary.

    Apt on Ubuntu 22.04 ships just 1.21 which can't parse the project justfile
    (needs home_directory() >= 1.30). Force the upstream binary when the
    installed version is too old, even if `just` is already on PATH.
    """
    installed = _detect_just_version()
    if installed and installed >= JUST_MIN_VERSION:
        return True
    if installed:
        _c.log_warn(
            f"just {'.'.join(map(str, installed))} is older than required "
            f"{'.'.join(map(str, JUST_MIN_VERSION))}; upgrading"
        )

    if installed is None and _c.check_apt_package_available("just"):
        print("\nInstalling just (apt)...")
        if not _c.run_apt_install_with_retry(["just"], dry_run, sudo=True):
            return False
        post_apt = _detect_just_version()
        if post_apt and post_apt >= JUST_MIN_VERSION:
            return True
        _c.log_warn(
            f"apt's just is {('.'.join(map(str, post_apt)) if post_apt else 'unknown')}; "
            "falling back to upstream binary"
        )

    machine = platform.machine().lower()
    target = JUST_TARGETS.get(machine)
    if not target:
        _c.log_warn(f"no prebuilt 'just' for arch '{machine}'; install manually")
        return True

    url = f"https://github.com/casey/just/releases/download/{JUST_VERSION}/just-{JUST_VERSION}-{target}.tar.gz"
    print(f"\nInstalling just {JUST_VERSION} (prebuilt: {target})...")
    if dry_run:
        print(f"  Would download: {url}")
        print("  Would install: /usr/local/bin/just")
        return True

    with tempfile.TemporaryDirectory() as tmp:
        archive = Path(tmp) / "just.tar.gz"
        if not _c.download_file(url, archive):
            return False
        try:
            _c.require_tar_data_filter()
            with tarfile.open(archive, "r:gz") as tar:
                tar.extract("just", path=tmp, filter="data")
        except (tarfile.TarError, KeyError) as e:
            _c.log_error(f"Failed to extract just: {e}")
            return False
        return _c.run_command(
            ["install", "-m", "0755", f"{tmp}/just", "/usr/local/bin/just"], dry_run, sudo=True
        )


def install_debian(
    dry_run: bool = False,
    category: str | None = None,
    skip_system_packages: bool = False,
) -> bool:
    """Install Debian/Ubuntu dependencies."""
    _c.log_info("Installing Debian/Ubuntu dependencies...")

    if not skip_system_packages:
        apt_update_cmd = _c.get_apt_update_command()
        if not _c.run_command(apt_update_cmd, dry_run, sudo=True):
            return False

        bootstrap_packages = ["software-properties-common", "gnupg2", "ca-certificates"]

        # Ensure "universe" is enabled before installing full package set.
        if not category:
            print("\nInstalling bootstrap packages...")
            if not _c.run_apt_install_with_retry(bootstrap_packages, dry_run, sudo=True):
                return False
            if not _c.run_command(["add-apt-repository", "-y", "universe"], dry_run, sudo=True):
                return False
            if not _c.run_command(apt_update_cmd, dry_run, sudo=True):
                return False

        if category:
            packages = get_debian_packages(category)
            if not packages:
                print(f"Unknown category: {category}")
                return False
        else:
            packages = get_debian_packages()
            packages = [pkg for pkg in packages if pkg not in bootstrap_packages]

        # Resolve alternatives for packages renamed/replaced in newer distro releases.
        packages = resolve_package_alternatives(packages)

        print(f"\nInstalling {len(packages)} packages...")
        if not _c.run_apt_install_with_retry(packages, dry_run, sudo=True):
            return False

        for pkg in DEBIAN_PACKAGES.get("gstreamer_optional", []):
            if _c.check_apt_package_available(pkg):
                print(f"\nInstalling optional package: {pkg}")
                _c.run_apt_install_with_retry([pkg], dry_run, sudo=True)
            else:
                print(f"\nSkipping optional package (not available): {pkg}")
    else:
        print("\nSkipping apt package installation (--skip-system-packages)")

    if not category or category == "core":
        print("\nInstalling pipx packages...")
        _c.run_command(["pipx", "ensurepath"], dry_run)
        for pkg in PIPX_PACKAGES:
            if not _c.run_command(["pipx", "install", pkg], dry_run):
                _c.log_error(f"Failed to install pipx package: {pkg}")
                return False

        if not install_just_debian(dry_run):
            _c.log_error("Failed to install just")
            return False

    if not skip_system_packages and not category:
        print("\nCleaning up...")
        _c.run_command(["apt-get", "clean"], dry_run, sudo=True)

    print("\nDebian dependencies installed successfully!")
    return True


__all__ = [
    "DEBIAN_PACKAGE_ALTERNATIVES",
    "JUST_MIN_VERSION",
    "JUST_TARGETS",
    "JUST_VERSION",
    "_detect_just_version",
    "install_debian",
    "install_just_debian",
    "resolve_package_alternatives",
]
