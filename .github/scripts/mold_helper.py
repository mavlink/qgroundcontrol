#!/usr/bin/env python3
"""
Mold linker helper for CI: download and install a pinned, SHA256-verified mold binary (Linux).

mold publishes no signatures or checksums, so the digests below are computed from the release
tarballs and pinned here; test_mold_version_drift.py keeps them in sync with
.github/build-config.json. Mirrors ccache_helper.py's pinned-download model.

Example:
    mold_helper.py install --arch x86_64
"""

from __future__ import annotations

import argparse
import hashlib
import platform
import sys
import tarfile
import tempfile
from dataclasses import dataclass
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.io import require_tar_data_filter
from common.net import download_with_retry
from common.proc import run_captured
from common.tool_version import probe_version

_MOLD_RELEASE_URL = "https://github.com/rui314/mold/releases/download"


@dataclass(frozen=True)
class MoldRelease:
    """A pinned mold release: version coupled to its per-arch tarball digests."""

    version: str
    sha256: dict[str, str]  # keyed by arch (x86_64, aarch64)


PINNED_RELEASE = MoldRelease(
    version="2.41.0",
    sha256={
        "x86_64": "a3696680d99e692970590a178bc3a33d78d60d1c6dc9db7a11b557b02b751f5d",
        "aarch64": "946de2774b06a71346bd59b55fddba610b65b8d93c3a4a1559cc84e103472710",
    },
)

ARCH_MAP = {"x86_64": "x86_64", "amd64": "x86_64", "aarch64": "aarch64", "arm64": "aarch64"}


def detect_arch() -> str:
    """Auto-detect CPU architecture, normalizing to mold's release naming."""
    return ARCH_MAP.get(platform.machine().lower(), "x86_64")


def install(version: str, arch: str, prefix: Path) -> Path:
    """Download the SHA256-verified mold tarball and install the binary under ``prefix``/bin."""
    if version != PINNED_RELEASE.version:
        raise RuntimeError(
            f"mold installer pinned to {PINNED_RELEASE.version} but got {version}. "
            f"Update PINNED_RELEASE in mold_helper.py and .github/build-config.json together."
        )
    sha256 = PINNED_RELEASE.sha256[arch]
    stem = f"mold-{version}-{arch}-linux"
    archive_name = f"{stem}.tar.gz"
    url = f"{_MOLD_RELEASE_URL}/v{version}/{archive_name}"

    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        archive = tmp_path / archive_name
        download_with_retry(url, archive)
        actual = hashlib.sha256(archive.read_bytes()).hexdigest()
        if actual != sha256:
            raise RuntimeError(f"SHA256 mismatch for {archive_name}: {actual} != {sha256}")
        require_tar_data_filter()
        with tarfile.open(archive, "r:gz") as tar:
            tar.extractall(tmp_path, filter="data")
        src = tmp_path / stem / "bin" / "mold"
        if not src.exists():
            raise FileNotFoundError(f"mold binary not found in archive: {src}")
        bin_dir = prefix / "bin"
        dest = bin_dir / "mold"
        res = run_captured(["sudo", "install", "-D", "-m", "0755", str(src), str(dest)])
        if res.returncode != 0:
            raise RuntimeError(f"Failed to install mold to {dest}: {res.stderr}")
        res = run_captured(["sudo", "ln", "-sf", "mold", str(bin_dir / "ld.mold")])
        if res.returncode != 0:
            raise RuntimeError(f"Failed to create ld.mold symlink: {res.stderr}")
    return dest


def is_installed(version: str) -> bool:
    """Check if the requested mold version is already the one on PATH."""
    installed = probe_version("mold")
    if installed is None:
        return False
    expected = tuple(int(n) for n in version.split("."))
    compare_len = min(len(installed), len(expected))
    return installed[:compare_len] == expected[:compare_len]


def main(argv: list[str] | None = None) -> int:
    """CLI entry point."""
    parser = argparse.ArgumentParser(description="Install a pinned mold binary (Linux)")
    sub = parser.add_subparsers(dest="command")
    inst = sub.add_parser("install", help="Download and install a pinned mold binary")
    inst.add_argument("--version", default=PINNED_RELEASE.version)
    inst.add_argument("--arch", choices=["x86_64", "aarch64"], default=None)
    inst.add_argument("--prefix", type=Path, default=Path("/usr/local"))
    args = parser.parse_args(argv)

    if args.command == "install":
        if platform.system() != "Linux":
            print("Warning: mold binary install only supported on Linux", file=sys.stderr)
            return 0
        arch = args.arch or detect_arch()
        if is_installed(args.version):
            print(f"mold {args.version} already installed")
            return 0
        dest = install(args.version, arch, args.prefix)
        result = run_captured([str(dest), "--version"])
        if result.stdout:
            print(result.stdout.strip())
        return 0

    print("Error: a subcommand is required (install)", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
