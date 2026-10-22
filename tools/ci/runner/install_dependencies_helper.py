#!/usr/bin/env python3
"""Post-install fixups for CI dependency caching on Linux.

Handles:
- Enabling the Ubuntu universe repository
- Repairing apt alternatives after cache-apt-pkgs-action restore
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import gh_error, gh_warning, write_github_output
from common.proc import run_captured
from setup.install_dependencies._packages import get_debian_packages

APT_OPTS = ["-o", "DPkg::Lock::Timeout=300", "-o", "Acquire::Retries=3"]


def _sudo(cmd: list[str], *, check: bool = True) -> subprocess.CompletedProcess:
    """Run cmd under sudo, streaming output. Captured callers use run_captured directly."""
    return subprocess.run(["sudo", *cmd], check=check)


def _ldconfig_has_blas() -> bool:
    result = run_captured(["ldconfig", "-p"])
    return bool(re.search(r"\blibblas\.so\.3\b", result.stdout))


def _get_multiarch() -> str:
    result = run_captured(["dpkg-architecture", "-qDEB_HOST_MULTIARCH"])
    return result.stdout.strip() if result.returncode == 0 else ""


def enable_universe() -> None:
    """Enable universe; the following bounded install step refreshes package lists."""
    sources_dirs = [Path("/etc/apt/sources.list"), Path("/etc/apt/sources.list.d")]
    universe_found = False

    for source in sources_dirs:
        if not source.exists():
            continue
        paths = (
            [source]
            if source.is_file()
            else list(source.glob("*.list")) + list(source.glob("*.sources"))
        )
        for p in paths:
            try:
                text = p.read_text(errors="replace")
                if re.search(r"^(deb|Components:).*universe", text, re.MULTILINE):
                    universe_found = True
                    break
            except OSError:
                continue
        if universe_found:
            break

    if not universe_found:
        _sudo(["apt-get", *APT_OPTS, "install", "-y", "-qq", "software-properties-common"])
        _sudo(["add-apt-repository", "-y", "universe"])


def _package_installed(package: str) -> bool:
    result = run_captured(["dpkg-query", "-W", "--showformat=${db:Status-Status}", package])
    return result.returncode == 0 and result.stdout.strip() == "installed"


def fix_apt_alternatives() -> None:
    """Repair apt alternatives and BLAS symlinks after cache restore."""
    result = _sudo(["dpkg", "--configure", "-a"], check=False)
    if result.returncode != 0:
        gh_warning("dpkg --configure -a failed; package state may be inconsistent")

    # Category installs (e.g. Android's core) have no BLAS to repair.
    if not _package_installed("libblas3") or _ldconfig_has_blas():
        return

    gh_warning("libblas.so.3 missing from linker cache; attempting repair")
    _sudo(
        ["apt-get", *APT_OPTS, "install", "-y", "-qq", "--reinstall", "libblas3", "libopenblas0"],
        check=False,
    )

    multiarch = _get_multiarch()
    if multiarch:
        _sudo(["update-alternatives", "--auto", f"libblas.so.3-{multiarch}"], check=False)

        blas_link = Path(f"/usr/lib/{multiarch}/libblas.so.3")
        if not blas_link.exists():
            candidates = list(Path("/usr/lib").rglob("libblas.so.3"))
            if candidates:
                candidate = candidates[0]
                gh_warning(f"Creating compatibility symlink {blas_link} -> {candidate}")
                _sudo(["ln", "-sf", str(candidate), str(blas_link)])

    _sudo(["ldconfig"])

    if not _ldconfig_has_blas():
        gh_error("libblas.so.3 is still missing after repair attempt")
        sys.exit(1)


def print_packages(category: str = "") -> None:
    """Resolve the debian apt package list and emit it as a GITHUB_OUTPUT value."""
    packages = " ".join(get_debian_packages(category or None))
    if not packages:
        gh_error(f"install_dependencies has no Debian packages for category {category!r}")
        sys.exit(1)
    write_github_output({"packages": packages})


def main() -> None:
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("enable-universe")
    sub.add_parser("fix-apt-alternatives")
    packages = sub.add_parser("print-packages")
    packages.add_argument("--category", default="", help="Only this category (default: all)")

    args = parser.parse_args()
    if args.command == "print-packages":
        print_packages(args.category)
        return
    commands = {
        "enable-universe": enable_universe,
        "fix-apt-alternatives": fix_apt_alternatives,
    }
    commands[args.command]()


if __name__ == "__main__":
    main()
