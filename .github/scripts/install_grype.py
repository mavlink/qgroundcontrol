#!/usr/bin/env python3
"""Install verified Linux Grype assets into the scan action's runner tool cache."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import write_github_output
from common.io import extract_tar_data, sha256_file
from common.net import download_with_retry
from common.platform import host_arch, is_linux

VERSION = "0.110.0"
SHA256 = {
    "x86_64": "aaa98d27d2d7efd9317c6a1ad6d9b15f3e65bab320e7d03bde41e251387bb71c",
    "aarch64": "804041ee69f119022e3e866741a558eae6f2df372a5dc1a5376d456d16f8c931",
}


def install(tool_cache: Path, arch: str) -> Path:
    asset_arch, node_arch = {
        "x86_64": ("amd64", "x64"),
        "aarch64": ("arm64", "arm64"),
    }[arch]
    directory = tool_cache / "grype" / VERSION / node_arch
    binary = directory / "grype"
    complete = directory.with_name(f"{node_arch}.complete")
    if binary.is_file() and complete.is_file():
        return binary

    filename = f"grype_{VERSION}_linux_{asset_arch}.tar.gz"
    url = f"https://github.com/anchore/grype/releases/download/v{VERSION}/{filename}"
    with tempfile.TemporaryDirectory(prefix="qgc-grype-") as temporary:
        staging = Path(temporary)
        archive = staging / filename
        download_with_retry(url, archive, attempts=3, delay=10, timeout=60)
        if sha256_file(archive) != SHA256[arch]:
            raise RuntimeError(f"SHA256 mismatch for {filename}")
        extract_tar_data(archive, staging, mode="r:gz")
        source = staging / "grype"
        if source.is_symlink() or not source.is_file():
            raise RuntimeError(f"No regular grype binary in {filename}")
        source.chmod(0o755)
        subprocess.run([str(source), "version"], check=True, timeout=30)
        directory.mkdir(parents=True, exist_ok=True)
        complete.unlink(missing_ok=True)
        shutil.copy2(source, binary)
        # @actions/tool-cache.find requires this marker next to the arch directory.
        # Publish only after download, digest verification, extraction and execution succeed.
        complete.touch()
    return binary


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool-cache", type=Path, default=os.environ.get("RUNNER_TOOL_CACHE"))
    args = parser.parse_args()
    if not is_linux():
        parser.error("Grype bootstrap supports Linux runners only")
    if args.tool_cache is None:
        parser.error("--tool-cache or RUNNER_TOOL_CACHE is required")
    binary = install(args.tool_cache, host_arch())
    write_github_output({"version": VERSION, "cmd": str(binary)})


if __name__ == "__main__":
    main()
