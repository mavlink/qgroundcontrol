#!/usr/bin/env python3
"""Assemble an arm64 sysroot inside the Debian/Ubuntu cross-build container."""

from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))

from common.proc import run_checked_with_retry


def configure_sources(apt_dir: Path, suite: str, mirror: str) -> None:
    if not re.fullmatch(r"[a-z][-a-z0-9]*", suite):
        raise ValueError(f"Invalid Ubuntu suite: {suite!r}")
    if not re.fullmatch(r"https?://[A-Za-z0-9._/-]+", mirror):
        raise ValueError(f"Invalid ports mirror: {mirror!r}")
    sources = apt_dir / "sources.list.d"
    sources.mkdir(parents=True, exist_ok=True)
    for path in [apt_dir / "sources.list", *sources.glob("*.list")]:
        if path.is_file() and "arm64-ports" not in path.name:
            path.write_text(
                re.sub(
                    r"^(deb(?:-src)?\s+)(https?://)",
                    r"\1[arch=amd64] \2",
                    path.read_text(),
                    flags=re.MULTILINE,
                )
            )
    for path in sources.glob("*.sources"):
        stanzas = re.split(r"(\n[ \t]*\n)", path.read_text())
        for index, stanza in enumerate(stanzas):
            if re.search(r"^Types:", stanza, re.MULTILINE) and not re.search(
                r"^Architectures:", stanza, re.MULTILINE
            ):
                stanzas[index] = re.sub(
                    r"^(Types:[^\n]*)", r"\1\nArchitectures: amd64", stanza, flags=re.MULTILINE
                )
        path.write_text("".join(stanzas))
    (sources / "ubuntu-arm64-ports.list").write_text(
        "".join(
            f"deb [arch=arm64] {mirror} {suite}{suffix} main restricted universe multiverse\n"
            for suffix in ("", "-updates", "-security")
        )
    )


def arm64_closure(output: str) -> list[str]:
    return sorted(
        {
            line.strip()
            for line in output.splitlines()
            if line and not line[0].isspace() and line.endswith(":arm64")
        }
    )


def install() -> None:
    if os.getuid() != 0 or not shutil.which("apt-get"):
        raise ValueError("Run as root inside a Debian/Ubuntu build container")
    suite = os.environ.get("UBUNTU_SUITE", "noble")
    mirror = os.environ.get("MIRROR", "http://ports.ubuntu.com/ubuntu-ports")
    # Validate overrides before changing package-manager state.
    if not re.fullmatch(r"[a-z][-a-z0-9]*", suite) or not re.fullmatch(
        r"https?://[A-Za-z0-9._/-]+", mirror
    ):
        raise ValueError("Invalid UBUNTU_SUITE or MIRROR")
    os.environ["DEBIAN_FRONTEND"] = "noninteractive"
    architectures = subprocess.run(
        ["dpkg", "--print-foreign-architectures"], capture_output=True, text=True, check=True
    ).stdout.split()
    if "arm64" not in architectures:
        subprocess.run(["dpkg", "--add-architecture", "arm64"], check=True)
    configure_sources(Path("/etc/apt"), suite, mirror)
    run_checked_with_retry(
        ["apt-get", "-o", "Acquire::Retries=3", "update", "-y", "--quiet"], retry_backoff_seconds=10
    )
    installer = os.environ.get(
        "INSTALL_DEPS",
        str(Path(__file__).resolve().parents[2] / "tools/setup/install_dependencies"),
    )
    packages = subprocess.run(
        [
            sys.executable,
            installer,
            "--print-packages",
            "--platform",
            "debian",
            "--category",
            "cross_arm64",
        ],
        capture_output=True,
        text=True,
        check=True,
    ).stdout.split()
    if not packages:
        raise ValueError("install_dependencies returned no cross_arm64 packages")
    run_checked_with_retry(
        [
            "apt-get",
            "-o",
            "Acquire::Retries=3",
            "install",
            "-y",
            "--quiet",
            "--no-install-recommends",
            "gcc-aarch64-linux-gnu",
            "g++-aarch64-linux-gnu",
            "pkgconf",
        ],
        retry_backoff_seconds=10,
    )
    wanted = [f"{package}:arm64" for package in packages]
    if (
        subprocess.run(
            ["apt-cache", "show", "gstreamer1.0-qt6:arm64"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        ).returncode
        == 0
    ):
        wanted.append("gstreamer1.0-qt6:arm64")
    output = subprocess.run(
        [
            "apt-cache",
            "depends",
            "--recurse",
            "--no-recommends",
            "--no-suggests",
            "--no-conflicts",
            "--no-breaks",
            "--no-replaces",
            "--no-enhances",
            *wanted,
        ],
        capture_output=True,
        text=True,
        check=True,
    ).stdout
    closure = arm64_closure(output)
    if not closure:
        raise ValueError("Empty arm64 dependency closure")
    sysroot = Path(os.environ.get("SYSROOT", "/opt/sysroot"))
    # Download/extract instead of installing :arm64 -dev packages: those may
    # otherwise replace host Python with python3:arm64.
    with tempfile.TemporaryDirectory(prefix="qgc-sysroot-") as directory:
        run_checked_with_retry(
            ["apt-get", "download", *closure], cwd=directory, retry_backoff_seconds=10
        )
        archives = sorted(Path(directory).glob("*.deb"))
        if not archives:
            raise ValueError("apt-get download produced no .deb files")
        sysroot.mkdir(parents=True, exist_ok=True)
        for archive in archives:
            subprocess.run(["dpkg-deb", "-x", str(archive), str(sysroot)], check=True)
    print(f"Sysroot ready at {sysroot} ({len(closure)} arm64 packages)")


if __name__ == "__main__":
    try:
        install()
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Sysroot setup failed: {error}", file=sys.stderr)
        raise SystemExit(1) from error
