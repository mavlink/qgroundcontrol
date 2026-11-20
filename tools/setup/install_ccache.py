#!/usr/bin/env python3
"""
Install the ccache release that .github/build-config.json pins (`tools.ccache`) on Linux, as CI's
runners use it, after verifying the release archive's SHA-256. The devcontainer image installs it
to /usr/local/bin, ahead of the distribution's older ccache on PATH.

Usage:
    install_ccache.py [--prefix /usr/local]
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import _bootstrap as _bootstrap
from common.build_config import pinned_release
from common.logging import log_error, log_ok
from common.net import fetch_release_member
from common.platform import host_arch, is_linux


def install(prefix: Path) -> Path:
    """Install the pinned ccache as <prefix>/bin/ccache and return its path."""
    ccache = pinned_release("ccache")
    stem = f"ccache-{ccache.version}-linux-{host_arch()}-glibc"
    destination = prefix / "bin" / "ccache"
    with tempfile.TemporaryDirectory() as staging:
        source = fetch_release_member(
            f"{ccache.url}/{stem}.tar.xz",
            ccache.sha256[f"linux-{host_arch()}-glibc"],
            f"{stem}/ccache",
            Path(staging),
        )
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
    destination.chmod(0o755)
    version = subprocess.run(
        [str(destination), "--version"], capture_output=True, text=True, check=True
    ).stdout
    if f"ccache version {ccache.version}" not in version:
        raise RuntimeError(
            f"{destination} reports {version.splitlines()[:1]}, not {ccache.version}"
        )
    return destination


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--prefix", type=Path, default=Path("/usr/local"))
    args = parser.parse_args(argv)
    if not is_linux():
        log_error("install_ccache.py installs the Linux release only")
        return 1
    try:
        destination = install(args.prefix)
    except (OSError, KeyError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        log_error(f"Installing ccache failed: {error}")
        return 1
    log_ok(f"Installed {destination}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
