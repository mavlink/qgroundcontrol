"""Shared helpers for install_dependencies submodules.

`__main__.py` puts the directory above `setup/` on sys.path: `tools/` in-repo, or
`/tmp/tools/` inside Docker builders (which COPY the package to
`/tmp/tools/setup/install_dependencies/` and the shared packages alongside it).
"""

from __future__ import annotations

import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path
from typing import TYPE_CHECKING

from common.build_config import get_build_config_value  # re-exported for submodules
from common.gh_actions import (  # re-exported for submodules
    append_github_env,
    append_github_path,
    is_ci,
)
from common.gstreamer import (  # URL helpers re-exported for submodules
    mirror_url,
    package_filename,
    package_url,
    pinned_checksum,
)
from common.logging import log_error, log_info, log_warn  # re-exported for submodules
from common.net import download_file as atomic_download_file
from common.platform import is_linux, is_macos, is_windows
from common.proc import run_captured  # re-exported for submodules

if TYPE_CHECKING:
    from collections.abc import Sequence

APT_BASE_OPTIONS: list[str] = [
    "-o",
    "DPkg::Lock::Timeout=300",
    "-o",
    "Acquire::Retries=3",
]


def _os_release_ids() -> set[str]:
    """Return the ID + ID_LIKE tokens from /etc/os-release (lowercased)."""
    ids: set[str] = set()
    try:
        text = Path("/etc/os-release").read_text(encoding="utf-8")
    except OSError:
        return ids
    for line in text.splitlines():
        key, _, value = line.partition("=")
        if key in ("ID", "ID_LIKE"):
            ids.update(value.strip().strip('"').lower().split())
    return ids


def is_ubuntu() -> bool:
    """True only on Ubuntu (not plain Debian) — gates Ubuntu-only apt repos."""
    return "ubuntu" in _os_release_ids()


def detect_platform() -> str | None:
    """Detect current platform (package-manager family)."""
    if is_windows():
        return "windows"
    if is_macos():
        return "macos"
    if is_linux():
        ids = _os_release_ids()
        if ids & {"fedora", "rhel", "centos"}:
            return "fedora"
        if "arch" in ids:
            return "arch"
        if Path("/etc/debian_version").exists() or {"debian", "ubuntu"} & ids:
            return "debian"
        return "linux"
    return None


def has_command(cmd: str) -> bool:
    """Check if a command is available."""
    return shutil.which(cmd) is not None


def get_apt_install_command(packages: list[str]) -> list[str]:
    """Build apt-get install command."""
    return [
        "apt-get",
        *APT_BASE_OPTIONS,
        "install",
        "-y",
        "-qq",
        "--no-install-recommends",
        *packages,
    ]


def get_apt_update_command() -> list[str]:
    """Build apt-get update command."""
    return [
        "apt-get",
        *APT_BASE_OPTIONS,
        "update",
        "-y",
        "-qq",
    ]


def _install_with_retry(
    install: list[str],
    refresh: list[str],
    *,
    tool: str,
    retry_note: str,
    dry_run: bool,
    sudo: bool,
    max_attempts: int,
) -> bool:
    """Run *install*, running *refresh* between failed attempts; give up if *refresh* fails."""
    for attempt in range(1, max_attempts + 1):
        if run_command(install, dry_run, sudo=sudo):
            return True
        if attempt >= max_attempts:
            break
        print(f"  {tool} install failed (attempt {attempt}/{max_attempts}); {retry_note}")
        if not run_command(refresh, dry_run, sudo=sudo):
            return False
    return False


def run_apt_install_with_retry(
    packages: list[str],
    dry_run: bool = False,
    sudo: bool = False,
    max_attempts: int = 2,
) -> bool:
    """Install apt packages, refreshing package lists between attempts."""
    return _install_with_retry(
        get_apt_install_command(packages),
        get_apt_update_command(),
        tool="apt",
        retry_note="refreshing package lists and retrying...",
        dry_run=dry_run,
        sudo=sudo,
        max_attempts=max_attempts,
    )


def get_dnf_install_command(packages: list[str]) -> list[str]:
    """Build a dnf install command tolerant of missing optional packages."""
    return [
        "dnf",
        "install",
        "-y",
        "--setopt=install_weak_deps=False",
        "--skip-unavailable",
        "--skip-broken",
        *packages,
    ]


def run_dnf_install_with_retry(
    packages: list[str],
    dry_run: bool = False,
    sudo: bool = False,
    max_attempts: int = 2,
) -> bool:
    """Install dnf packages, refreshing metadata between attempts."""
    return _install_with_retry(
        get_dnf_install_command(packages),
        ["dnf", "makecache"],
        tool="dnf",
        retry_note="retrying...",
        dry_run=dry_run,
        sudo=sudo,
        max_attempts=max_attempts,
    )


def run_pacman_install_with_retry(
    packages: list[str],
    dry_run: bool = False,
    sudo: bool = False,
    max_attempts: int = 2,
) -> bool:
    """Install pacman packages (--needed skips already-installed), refreshing between attempts."""
    return _install_with_retry(
        ["pacman", "-S", "--needed", "--noconfirm", *packages],
        ["pacman", "-Syy", "--noconfirm"],
        tool="pacman",
        retry_note="refreshing...",
        dry_run=dry_run,
        sudo=sudo,
        max_attempts=max_attempts,
    )


def install_build_tools(dry_run: bool = False) -> bool:
    """Install the locked build group (CMake, Ninja, just, ...) and expose its executables."""
    from qgc_tools.python_env import executable, sync_groups

    try:
        environment = sync_groups("build", dry_run=dry_run)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        log_error(f"Could not install build tools: {error}")
        return False
    if dry_run:
        return True
    scripts = str(executable("cmake", environment).parent)
    path = os.environ.get("PATH", "")
    if is_ci():
        append_github_path(scripts)
    elif scripts not in path.split(os.pathsep):
        log_info(f"CMake, Ninja and just are in {scripts}; activate it with . {scripts}/activate")
    os.environ["PATH"] = f"{scripts}{os.pathsep}{path}"
    return True


def _brew_installed_formulae() -> set[str]:
    """Return the set of currently installed Homebrew formulae names."""
    result = subprocess.run(
        ["brew", "list", "--formula", "-1"],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        return set()
    return {line.strip() for line in result.stdout.splitlines() if line.strip()}


def get_brew_install_command(packages: list[str]) -> list[str]:
    """Build brew install command, filtering out packages already installed.

    `brew install <already-installed>` emits a noisy "already installed and
    up-to-date" warning per package. Pre-filtering with `brew list` is
    cheaper than parsing the warnings and keeps CI logs clean.
    """
    if not packages:
        return ["true"]  # nothing to do; keep run_command happy
    installed = _brew_installed_formulae()
    missing = [p for p in packages if p not in installed]
    if not missing:
        return ["true"]
    return ["brew", "install", "--quiet", *missing]


def check_apt_package_available(package: str) -> bool:
    """Check if an apt package has an installation candidate (not merely referenced)."""
    result = subprocess.run(
        ["apt-cache", "policy", package],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        return False
    for line in result.stdout.splitlines():
        if "Candidate:" in line:
            return "(none)" not in line
    return False


def run_command(
    cmd: list[str],
    dry_run: bool = False,
    sudo: bool = False,
    ok_returncodes: tuple[int, ...] = (0,),
) -> bool:
    """Run a command, optionally with sudo."""
    # os.geteuid is not available on Windows; treat missing API as non-root.
    is_root = hasattr(os, "geteuid") and os.geteuid() == 0
    if sudo and not is_root:
        cmd = ["sudo", *cmd]

    if dry_run:
        print(f"  Would run: {shlex.join(cmd)}")
        return True

    print(f"  Running: {shlex.join(cmd[:5])}{'...' if len(cmd) > 5 else ''}")
    result = subprocess.run(cmd)
    return result.returncode in ok_returncodes


def _set_env_var_ci(name: str, value: str) -> None:
    """Set an environment variable via GITHUB_ENV for CI."""
    append_github_env({name: value})
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
        append_github_path(path_entry)
    else:
        if sys.platform != "win32":
            raise RuntimeError("Local PATH persistence is only supported on Windows")
        import winreg

        key_path = r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment"
        with winreg.OpenKey(
            winreg.HKEY_LOCAL_MACHINE,
            key_path,
            0,
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
    *,
    warn_on_failure: bool = False,
    expected_sha256: str | None = None,
) -> bool:
    """Download with bounded retries using only bootstrap-safe standard-library dependencies."""
    if dry_run:
        print(f"  Would download: {url} -> {dest.name}")
        return True

    try:
        atomic_download_file(
            url,
            dest,
            expected_sha256=expected_sha256,
            attempts=retries + 1,
            timeout=timeout,
        )
        return True
    except (OSError, RuntimeError) as error:
        log = log_warn if warn_on_failure else log_error
        log(f"Failed to download {url}: {error}")
        return False


def download_first(
    urls: Sequence[str], dest: Path, expected_sha256: str, dry_run: bool = False
) -> bool:
    """Download dest from the first of urls that succeeds, warning before each fallback."""
    for index, url in enumerate(urls):
        fallback = index + 1 < len(urls)
        if download_file(
            url, dest, dry_run, warn_on_failure=fallback, expected_sha256=expected_sha256
        ):
            return True
        if fallback:
            log_warn(f"Retrying from {urls[index + 1]}")
    return False


__all__ = [
    "APT_BASE_OPTIONS",
    "_set_env_var_ci",
    "_set_env_var_local",
    "add_to_path",
    "append_github_env",
    "check_apt_package_available",
    "detect_platform",
    "download_file",
    "download_first",
    "get_apt_install_command",
    "get_apt_update_command",
    "get_brew_install_command",
    "get_build_config_value",
    "get_dnf_install_command",
    "has_command",
    "install_build_tools",
    "is_ci",
    "is_ubuntu",
    "log_error",
    "log_info",
    "log_warn",
    "mirror_url",
    "package_filename",
    "package_url",
    "pinned_checksum",
    "run_apt_install_with_retry",
    "run_captured",
    "run_command",
    "run_dnf_install_with_retry",
    "run_pacman_install_with_retry",
    "set_env_var",
]
