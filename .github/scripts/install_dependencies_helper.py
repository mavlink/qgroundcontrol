#!/usr/bin/env python3
"""Post-install fixups for CI dependency caching on Linux.

Handles:
- Normalizing official Ubuntu apt mirrors to reliable HTTPS endpoints
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

from common.gh_actions import gh_error, gh_warning, write_github_output
from common.proc import run_captured

APT_OPTS = ["-o", "DPkg::Lock::Timeout=300", "-o", "Acquire::Retries=3"]
UBUNTU_APT_URL_PATTERN = re.compile(
    r"http://(?P<host>[a-z0-9.-]+\.ubuntu\.com)(?P<suffix>(?::\d+)?(?:/[^\s#]*)?)",
    re.IGNORECASE,
)


def _sudo(cmd: list[str], *, check: bool = True) -> subprocess.CompletedProcess:
    """Run cmd under sudo, streaming output. Captured callers use run_captured directly."""
    return subprocess.run(["sudo", *cmd], check=check)


def _ldconfig_has_blas() -> bool:
    result = run_captured(["ldconfig", "-p"])
    return bool(re.search(r"\blibblas\.so\.3\b", result.stdout))


def _get_multiarch() -> str:
    result = run_captured(["dpkg-architecture", "-qDEB_HOST_MULTIARCH"])
    return result.stdout.strip() if result.returncode == 0 else ""


def _normalize_ubuntu_apt_urls(text: str) -> str:
    """Use canonical HTTPS endpoints for official Ubuntu apt repositories."""

    def replace_url(match: re.Match[str]) -> str:
        host = match.group("host").lower()
        suffix = match.group("suffix")

        if host.endswith(".ec2.ports.ubuntu.com"):
            host = "ports.ubuntu.com"
        elif host.endswith(".ec2.archive.ubuntu.com"):
            host = "archive.ubuntu.com"
        elif host not in {"archive.ubuntu.com", "ports.ubuntu.com", "security.ubuntu.com"}:
            return match.group(0)

        return f"https://{host}{suffix}"

    return UBUNTU_APT_URL_PATTERN.sub(replace_url, text)


def normalize_apt_sources(apt_root: Path = Path("/etc/apt")) -> None:
    """Normalize Ubuntu URLs in legacy and deb822 apt source files."""
    sources = [apt_root / "sources.list"]
    sources_dir = apt_root / "sources.list.d"
    if sources_dir.is_dir():
        sources.extend(sorted(sources_dir.glob("*.list")))
        sources.extend(sorted(sources_dir.glob("*.sources")))

    changed_sources = 0
    for source in sources:
        if not source.is_file():
            continue

        try:
            original = source.read_text(errors="replace")
        except OSError as error:
            gh_warning(f"Unable to read apt source {source}: {error}")
            continue

        normalized = _normalize_ubuntu_apt_urls(original)
        if normalized == original:
            continue

        try:
            source.write_text(normalized)
        except PermissionError:
            subprocess.run(
                ["sudo", "tee", str(source)],
                check=True,
                input=normalized,
                stdout=subprocess.DEVNULL,
                text=True,
            )
        changed_sources += 1

    print(f"Normalized {changed_sources} apt source file(s)")


def enable_universe() -> None:
    """Enable the Ubuntu universe repository if not already present."""
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

    _sudo(["apt-get", *APT_OPTS, "update", "-y", "-qq"])


def fix_apt_alternatives() -> None:
    """Repair apt alternatives and BLAS symlinks after cache restore."""
    result = _sudo(["dpkg", "--configure", "-a"], check=False)
    if result.returncode != 0:
        gh_warning("dpkg --configure -a failed; package state may be inconsistent")

    if _ldconfig_has_blas():
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


def install_optional_packages() -> None:
    """Install optional apt packages that are available."""
    result = run_captured(
        [
            sys.executable,
            "tools/setup/install_dependencies",
            "--platform",
            "debian",
            "--category",
            "gstreamer_optional",
            "--print-available-packages",
        ],
    )
    packages = result.stdout.strip()
    if packages:
        print(f"Installing optional packages: {packages}")
        _sudo(["apt-get", *APT_OPTS, "install", "-y", "-qq", *packages.split()], check=False)


def detect_python_version() -> None:
    """Write Python minor version to GITHUB_OUTPUT for cache keys."""
    minor = f"{sys.version_info.major}.{sys.version_info.minor}"
    write_github_output({"minor": minor})


def print_packages() -> None:
    """Resolve the debian apt package list and emit it as a GITHUB_OUTPUT value."""
    result = run_captured(
        [
            sys.executable,
            "tools/setup/install_dependencies",
            "--platform",
            "debian",
            "--print-packages",
        ],
        check=True,
    )
    write_github_output({"packages": result.stdout.strip()})


def main() -> None:
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("normalize-apt-sources")
    sub.add_parser("enable-universe")
    sub.add_parser("fix-apt-alternatives")
    sub.add_parser("install-optional")
    sub.add_parser("detect-python-version")
    sub.add_parser("print-packages")

    args = parser.parse_args()
    commands = {
        "normalize-apt-sources": normalize_apt_sources,
        "enable-universe": enable_universe,
        "fix-apt-alternatives": fix_apt_alternatives,
        "install-optional": install_optional_packages,
        "detect-python-version": detect_python_version,
        "print-packages": print_packages,
    }
    commands[args.command]()


if __name__ == "__main__":
    main()
