"""Shared helpers + sys.path bootstrap for install_dependencies submodules.

Layout note: this file lives at tools/setup/install_dependencies/_common.py.
parents[2] resolves to `tools/` in-repo, OR to `/tmp/` inside the Docker
builders (which COPY the package to `/tmp/qt/install_dependencies/` and
`tools/common/` to `/tmp/common/`). Both contexts expose `common/` one
level above the package.
"""

from __future__ import annotations

import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

_tools_dir = Path(__file__).resolve().parents[2]
if str(_tools_dir) not in sys.path:
    sys.path.insert(0, str(_tools_dir))

from common.build_config import get_build_config_value  # noqa: E402
from common.env import is_ci  # noqa: E402  re-exported for submodules
from common.gh_actions import append_github_env  # noqa: E402
from common.io import require_tar_data_filter  # noqa: E402  re-exported for submodules
from common.logging import log_error, log_info, log_warn  # noqa: E402  re-exported for submodules
from common.platform import is_linux, is_macos, is_windows  # noqa: E402

APT_BASE_OPTIONS: list[str] = [
    "-o",
    "DPkg::Lock::Timeout=300",
    "-o",
    "Acquire::Retries=3",
]


def get_config_value(key: str) -> str | None:
    """Get a top-level string value from build config by key name."""
    value = get_build_config_value(key)
    return value or None


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
    for attempt in range(1, max_attempts + 1):
        if run_command(get_dnf_install_command(packages), dry_run, sudo=sudo):
            return True
        if attempt >= max_attempts:
            break
        print(f"  dnf install failed (attempt {attempt}/{max_attempts}); retrying...")
        if not run_command(["dnf", "makecache"], dry_run, sudo=sudo):
            return False
    return False


def run_pacman_install_with_retry(
    packages: list[str],
    dry_run: bool = False,
    sudo: bool = False,
    max_attempts: int = 2,
) -> bool:
    """Install pacman packages (--needed skips already-installed), refreshing between attempts."""
    install_cmd = ["pacman", "-S", "--needed", "--noconfirm", *packages]
    for attempt in range(1, max_attempts + 1):
        if run_command(install_cmd, dry_run, sudo=sudo):
            return True
        if attempt >= max_attempts:
            break
        print(f"  pacman install failed (attempt {attempt}/{max_attempts}); refreshing...")
        if not run_command(["pacman", "-Syy", "--noconfirm"], dry_run, sudo=sudo):
            return False
    return False


def run_pipx_install(dry_run: bool = False) -> bool:
    """Install the shared pipx-managed Python tools (gcovr, etc.)."""
    # Late import to avoid a circular dep at module load time.
    from ._packages import PIPX_PACKAGES

    print("\nInstalling pipx packages...")
    run_command(["pipx", "ensurepath"], dry_run)
    for pkg in PIPX_PACKAGES:
        if not run_command(["pipx", "install", pkg], dry_run):
            log_error(f"Failed to install pipx package: {pkg}")
            return False
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


def get_available_debian_packages(category: str) -> list[str]:
    """Return packages in *category* that exist in apt metadata."""
    from concurrent.futures import ThreadPoolExecutor

    # Late import to avoid a circular dep at module load time.
    from ._packages import get_debian_packages

    packages = get_debian_packages(category)
    with ThreadPoolExecutor() as pool:
        available = list(pool.map(check_apt_package_available, packages))
    return [pkg for pkg, ok in zip(packages, available, strict=False) if ok]


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
    if not is_windows():
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
        if not is_windows():
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
) -> bool:
    """Download a file from a URL."""
    if dry_run:
        print(f"  Would download: {url} -> {dest.name}")
        return True

    try:
        import httpx

        transport = httpx.HTTPTransport(retries=retries)
        with httpx.Client(
            transport=transport,
            timeout=timeout,
            headers={"User-Agent": "qgc-deps-installer/1.0"},
            follow_redirects=True,
        ) as client:
            print(f"  Downloading {dest.name}...")
            with client.stream("GET", url) as response:
                response.raise_for_status()
                with open(dest, "wb") as out:
                    for chunk in response.iter_bytes(chunk_size=65536):
                        out.write(chunk)
        return True
    except ImportError:
        import urllib.request

        req = urllib.request.Request(url, headers={"User-Agent": "qgc-deps-installer/1.0"})
        print(f"  Downloading {dest.name}...")
        with urllib.request.urlopen(req, timeout=timeout) as response, open(dest, "wb") as out:
            shutil.copyfileobj(response, out)
        return True
    except Exception as e:
        log_error(f"Failed to download {url}: {e}")
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
    "get_apt_install_command",
    "get_apt_update_command",
    "get_available_debian_packages",
    "get_brew_install_command",
    "get_config_value",
    "get_dnf_install_command",
    "has_command",
    "is_ci",
    "is_ubuntu",
    "log_error",
    "log_info",
    "log_warn",
    "require_tar_data_filter",
    "run_apt_install_with_retry",
    "run_command",
    "run_dnf_install_with_retry",
    "run_pacman_install_with_retry",
    "run_pipx_install",
    "set_env_var",
]
