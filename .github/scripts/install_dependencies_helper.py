#!/usr/bin/env python3
"""Post-install fixups for CI dependency caching on Linux.

Handles:
- Enabling the Ubuntu universe repository
- Repairing apt alternatives after cache-apt-pkgs-action restore
- Installing optional packages
- Detecting Python version for pipx cache keys
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import write_github_output  # noqa: E402

APT_OPTS = ["-o", "DPkg::Lock::Timeout=300", "-o", "Acquire::Retries=3"]


def _run(cmd: list[str], *, check: bool = True, **kwargs) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, check=check, **kwargs)


def _sudo(cmd: list[str], **kwargs) -> subprocess.CompletedProcess:
    return _run(["sudo", *cmd], **kwargs)


def _ldconfig_has_blas() -> bool:
    result = _run(["ldconfig", "-p"], capture_output=True, text=True, check=False)
    return bool(re.search(r"\blibblas\.so\.3\b", result.stdout))


def _get_multiarch() -> str:
    result = _run(
        ["dpkg-architecture", "-qDEB_HOST_MULTIARCH"],
        capture_output=True, text=True, check=False,
    )
    return result.stdout.strip() if result.returncode == 0 else ""


def enable_universe() -> None:
    """Enable the Ubuntu universe repository if not already present."""
    sources_dirs = [Path("/etc/apt/sources.list"), Path("/etc/apt/sources.list.d")]
    universe_found = False

    for source in sources_dirs:
        if not source.exists():
            continue
        paths = [source] if source.is_file() else list(source.glob("*.list")) + list(source.glob("*.sources"))
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

    _sudo(["apt-get", *APT_OPTS, "update", "-y", "-qq"])


def fix_apt_alternatives() -> None:
    """Repair apt alternatives and BLAS symlinks after cache restore."""
    result = _sudo(["dpkg", "--configure", "-a"], check=False)
    if result.returncode != 0:
        print("::warning::dpkg --configure -a failed; package state may be inconsistent")

    if _ldconfig_has_blas():
        return

    print("::warning::libblas.so.3 missing from linker cache; attempting repair")
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
                print(f"::warning::Creating compatibility symlink {blas_link} -> {candidate}")
                _sudo(["ln", "-sf", str(candidate), str(blas_link)])

    _sudo(["ldconfig"])

    if not _ldconfig_has_blas():
        print("::error::libblas.so.3 is still missing after repair attempt")
        sys.exit(1)


def install_optional_packages() -> None:
    """Install optional apt packages that are available."""
    result = _run(
        [sys.executable, "tools/setup/install_dependencies.py",
         "--platform", "debian", "--category", "gstreamer_optional", "--print-available-packages"],
        capture_output=True, text=True, check=False,
    )
    packages = result.stdout.strip()
    if packages:
        print(f"Installing optional packages: {packages}")
        _sudo(["apt-get", *APT_OPTS, "install", "-y", "-qq", *packages.split()], check=False)


def detect_python_version() -> None:
    """Write Python minor version to GITHUB_OUTPUT for cache keys."""
    minor = f"{sys.version_info.major}.{sys.version_info.minor}"
    write_github_output({"minor": minor})


def main() -> None:
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("enable-universe")
    sub.add_parser("fix-apt-alternatives")
    sub.add_parser("install-optional")
    sub.add_parser("detect-python-version")

    args = parser.parse_args()
    commands = {
        "enable-universe": enable_universe,
        "fix-apt-alternatives": fix_apt_alternatives,
        "install-optional": install_optional_packages,
        "detect-python-version": detect_python_version,
    }
    commands[args.command]()


if __name__ == "__main__":
    main()
