#!/usr/bin/env python3
"""Install verified Linux Grype assets into the scan action's runner tool cache."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.build_config import load_build_config
from common.gh_actions import write_github_output
from common.net import fetch_release_member
from common.platform import host_arch, is_linux


def install(tool_cache: Path, arch: str, pin: dict[str, Any]) -> Path:
    """Install build-config.json's ``tools.grype`` pin: repository, version and per-arch digests."""
    version = pin["version"]
    asset_arch, node_arch = {
        "x86_64": ("amd64", "x64"),
        "aarch64": ("arm64", "arm64"),
    }[arch]
    directory = tool_cache / "grype" / version / node_arch
    binary = directory / "grype"
    complete = directory.with_name(f"{node_arch}.complete")
    if binary.is_file() and complete.is_file():
        return binary

    filename = f"grype_{version}_linux_{asset_arch}.tar.gz"
    url = f"https://github.com/{pin['repository']}/releases/download/v{version}/{filename}"
    with tempfile.TemporaryDirectory(prefix="qgc-grype-") as temporary:
        source = fetch_release_member(url, pin["sha256"][arch], "grype", Path(temporary))
        source.chmod(0o755)
        subprocess.run([str(source), "version"], check=True, timeout=30)
        directory.mkdir(parents=True, exist_ok=True)
        complete.unlink(missing_ok=True)
        shutil.copy2(source, binary)
        # @actions/tool-cache.find requires this marker next to the arch directory.
        # Publish only after download, digest verification, extraction and execution succeed.
        complete.touch()
    return binary


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool-cache", type=Path, default=os.environ.get("RUNNER_TOOL_CACHE"))
    args = parser.parse_args(argv)
    if not is_linux():
        parser.error("Grype bootstrap supports Linux runners only")
    if args.tool_cache is None:
        parser.error("--tool-cache or RUNNER_TOOL_CACHE is required")
    pin = load_build_config()["tools"]["grype"]
    binary = install(args.tool_cache, host_arch(), pin)
    write_github_output({"version": pin["version"], "cmd": str(binary)})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
