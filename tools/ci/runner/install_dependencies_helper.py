#!/usr/bin/env python3
"""CI entry points for the install-dependencies action.

- plan: the Linux apt download-cache key and restore keys, as step outputs
- install: the Linux apt packages and validated extra packages, through apt_install_retry.py
- macos: the Homebrew packages and the GStreamer framework, through install_dependencies
- windows: MSVC and NSIS for windows.yml, through install_dependencies

macos and windows make up to three attempts; one that outlasts its timeout is killed with every
process it started (the Visual Studio installer runs its own workers) before the next attempt.
CI skips install_dependencies' Python build tools and the distro copies of CI_PROVIDED_TOOLS.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import platform
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import env_default, env_flag, gh_error, write_github_output
from common.paths import TOOLS_DIR
from common.proc import run_with_retry
from setup.install_dependencies._debian import enable_universe
from setup.install_dependencies._macos import install_macos
from setup.install_dependencies._packages import (
    get_debian_packages,
    get_packages,
    validate_extra_packages,
)

from ci.runner import apt_install_retry

INSTALL_DEPENDENCIES = TOOLS_DIR / "setup" / "install_dependencies"
EXTRA_PACKAGES_TIMEOUT = 300
# build-setup installs the pinned CMake and Ninja and the cache action the pinned ccache (and mold
# on Linux). Distro copies would only shadow or duplicate them on PATH.
CI_PROVIDED_TOOLS = frozenset({"ccache", "cmake", "mold", "ninja", "ninja-build"})
INSTALL_ATTEMPTS = 3
MACOS_ATTEMPT_TIMEOUT = 10 * 60
WINDOWS_ATTEMPT_TIMEOUT = 20 * 60


def debian_packages(category: str) -> list[str]:
    """The apt packages for category (blank: the full desktop set); exit if there are none."""
    packages = [
        package
        for package in get_debian_packages(category or None)
        if package not in CI_PROVIDED_TOOLS
    ]
    if not packages:
        gh_error(f"install_dependencies has no Debian packages for category {category!r}")
        raise SystemExit(1)
    return packages


def _os_release(path: Path = Path("/etc/os-release")) -> dict[str, str]:
    fields: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        key, sep, value = line.partition("=")
        if sep:
            fields[key.strip()] = value.strip().strip("\"'")
    return fields


def plan(category: str, *, os_release: Path = Path("/etc/os-release")) -> None:
    """Write the .deb cache key: category, distro, and runner architecture, then month, then packages.

    actions/cache never re-saves on an exact key hit, so the month stamp keeps the cache from going
    stale as -updates bumps package versions. A category's smaller set has its own prefix so it
    never restores into, or saves as, the full set's cache, and the distro stamp separates runner
    images whose .debs don't overlap.
    """
    packages = debian_packages(category)
    release = _os_release(os_release)
    distro = f"{release.get('ID', 'linux')}-{release.get('VERSION_ID', 'rolling')}"
    arch = os.environ.get("RUNNER_ARCH") or platform.machine()
    family = "-".join(filter(None, ("apt-debs", category, distro, arch)))
    month = datetime.now(timezone.utc).strftime("%Y-%m")
    digest = hashlib.sha256("\n".join(packages).encode()).hexdigest()
    write_github_output(
        {
            "key": f"{family}-{month}-{digest}",
            "restore-keys": f"{family}-{month}-\n{family}-",
        }
    )


def install(category: str, extra_packages: list[str]) -> int:
    """Install the planned packages, then the extras with a shorter timeout."""
    packages = debian_packages(category)
    try:
        extra_packages = validate_extra_packages(extra_packages)
    except ValueError as error:
        gh_error(str(error))
        return 1
    if not enable_universe():
        gh_error("Could not enable the universe repository")
        return 1
    status = apt_install_retry.main(
        ["--update", "--no-install-recommends", "--autoclean", *packages]
    )
    if status or not extra_packages:
        return status
    return apt_install_retry.main(
        [
            "--install-timeout",
            str(EXTRA_PACKAGES_TIMEOUT),
            "--label",
            "extra apt packages",
            *extra_packages,
        ]
    )


def install_with_retries(command: list[str], timeout: int) -> int:
    """Run an installer command up to INSTALL_ATTEMPTS times; return its last exit code."""
    try:
        result = run_with_retry(
            command,
            max_attempts=INSTALL_ATTEMPTS,
            retry_result_if=lambda completed: completed.returncode != 0,
            timeout=timeout,
            kill_tree_on_timeout=True,
            check=False,
        )
    except subprocess.TimeoutExpired:
        gh_error(
            f"The last of {INSTALL_ATTEMPTS} install attempts timed out after {timeout // 60} minutes"
        )
        return 1
    return result.returncode


def install_homebrew() -> int:
    """Install the Homebrew packages CI doesn't provide elsewhere, then the GStreamer framework."""
    packages = [package for package in get_packages("macos") if package not in CI_PROVIDED_TOOLS]
    return 0 if install_macos(packages=packages) else 1


def install_windows(*, msvc_arm64: bool) -> int:
    """Install MSVC and NSIS; CMake downloads GStreamer at configure time."""
    command = [
        sys.executable,
        str(INSTALL_DEPENDENCIES),
        "--platform",
        "windows",
        "--skip-gstreamer",
        "--msvc",
        "--nsis",
    ]
    if msvc_arm64:
        command.append("--msvc-arm64")
    return install_with_retries(command, WINDOWS_ATTEMPT_TIMEOUT)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    plan_parser = sub.add_parser("plan")
    install_parser = sub.add_parser("install")
    for command in (plan_parser, install_parser):
        command.add_argument(
            "--category",
            default=env_default("INPUT_CATEGORY"),
            help="Only this install_dependencies category (default: the full desktop set)",
        )
    install_parser.add_argument(
        "--extra-packages",
        default=env_default("INPUT_EXTRA_PACKAGES"),
        help="Space-separated additional apt packages",
    )
    macos = sub.add_parser("macos")
    macos.add_argument(
        "--single-attempt",
        action="store_true",
        help="Install once in this process, without retries or a timeout",
    )
    windows = sub.add_parser("windows")
    windows.add_argument(
        "--msvc-arm64",
        action="store_true",
        default=env_flag("INPUT_MSVC_ARM64"),
        help="Also add the ARM64 cross compiler component",
    )

    args = parser.parse_args(argv)
    if args.command == "plan":
        plan(args.category)
        return 0
    if args.command == "install":
        return install(args.category, args.extra_packages.split())
    if args.command == "macos":
        if args.single_attempt:
            return install_homebrew()
        # Each attempt runs in a child process, so a timeout can kill brew and the installers.
        attempt = [sys.executable, str(Path(__file__).resolve()), "macos", "--single-attempt"]
        return install_with_retries(attempt, MACOS_ATTEMPT_TIMEOUT)
    return install_windows(msvc_arm64=args.msvc_arm64)


if __name__ == "__main__":
    raise SystemExit(main())
