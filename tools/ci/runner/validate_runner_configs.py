#!/usr/bin/env python3
"""Validate the runner image Packer template and the RunsOn configuration.

The packer-validate and runs-on-lint pre-commit hooks run this; pre-commit.yml runs them when their
files change. Each tool is the release pinned in build-config.json's tools section, downloaded once
per version into .cache/tools/ after its SHA-256 checks out.
"""

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
from common.net import download_file, fetch_release_member
from common.paths import REPO_ROOT
from common.platform import host_arch, is_linux, is_macos

RUNNER_IMAGES = REPO_ROOT / ".github/runner-images"
PACKER_TEMPLATE = "qgc-ubuntu24-x64.pkr.hcl"
# Validation checks the template, not a build: the variables a build supplies get placeholders.
PACKER_PLACEHOLDER_VARS = {
    "PKR_VAR_aws_region": "us-west-2",
    "PKR_VAR_subnet_id": "subnet-00000000000000000",
    "PKR_VAR_source_repository": "https://github.com/mavlink/qgroundcontrol.git",
    "PKR_VAR_source_ref": "0" * 40,
    "PKR_VAR_qt_version": "0.0.0",
    "PKR_VAR_qt_modules": "",
}
RUNS_ON_CONFIGS = (".github/runs-on.yml", ".github/runner-images/windows-warm-pool.example.yml")
# Release asset names spell architectures the Go way.
GO_ARCH = {"x86_64": "amd64", "aarch64": "arm64"}


def platform_key() -> str:
    """Return the build-config.json digest key for this host: ``<linux|darwin>-<arch>``."""
    if is_linux():
        system = "linux"
    elif is_macos():
        system = "darwin"
    else:
        raise SystemExit("The runner configuration hooks run on Linux and macOS")
    return f"{system}-{host_arch()}"


def tool_dir(name: str, version: str, key: str) -> Path:
    return REPO_ROOT / ".cache/tools" / name / version / key


def _publish(source: Path, binary: Path) -> Path:
    """Copy a verified download to *binary* atomically, so a partial copy never looks installed."""
    binary.parent.mkdir(parents=True, exist_ok=True)
    staging = binary.with_name(f".{binary.name}.partial")
    shutil.copy2(source, staging)
    staging.chmod(0o755)
    os.replace(staging, binary)
    return binary


def ensure_packer(pin: dict[str, Any], key: str) -> Path:
    """Return build-config.json's pinned Packer, downloading and verifying it if needed."""
    version = pin["version"]
    binary = tool_dir("packer", version, key) / "packer"
    if binary.is_file():
        return binary
    system, arch = key.split("-", 1)
    url = (
        f"https://releases.hashicorp.com/packer/{version}/"
        f"packer_{version}_{system}_{GO_ARCH[arch]}.zip"
    )
    with tempfile.TemporaryDirectory(prefix="qgc-packer-") as temporary:
        return _publish(
            fetch_release_member(url, pin["sha256"][key], "packer", Path(temporary)), binary
        )


def ensure_roc(pin: dict[str, Any], key: str) -> Path:
    """Return build-config.json's pinned RunsOn CLI, downloading and verifying it if needed."""
    version = pin["version"]
    binary = tool_dir("roc", version, key) / "roc"
    if binary.is_file():
        return binary
    system, arch = key.split("-", 1)
    url = (
        f"https://github.com/{pin['repository']}/releases/download/v{version}/"
        f"roc_v{version}_{system}_{GO_ARCH[arch]}"
    )
    with tempfile.TemporaryDirectory(prefix="qgc-roc-") as temporary:
        download = Path(temporary) / "roc"
        download_file(url, download, expected_sha256=pin["sha256"][key])
        return _publish(download, binary)


def validate_packer(packer: Path) -> int:
    """Check the template's formatting, install its plugins, and validate it; return the exit code."""
    env = {**os.environ, **PACKER_PLACEHOLDER_VARS}
    for command in (["fmt", "-check"], ["init"], ["validate"]):
        result = subprocess.run(
            [str(packer), *command, PACKER_TEMPLATE], cwd=RUNNER_IMAGES, env=env, check=False
        )
        if result.returncode != 0:
            return result.returncode
    return 0


def lint_runs_on(roc: Path) -> int:
    """Lint every RunsOn configuration; return the first failing exit code."""
    status = 0
    for config in RUNS_ON_CONFIGS:
        result = subprocess.run([str(roc), "lint", config], cwd=REPO_ROOT, check=False)
        status = status or result.returncode
    return status


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("check", choices=("packer", "runs-on"))
    args = parser.parse_args(argv)
    key = platform_key()
    tools = load_build_config()["tools"]
    if args.check == "packer":
        return validate_packer(ensure_packer(tools["packer"], key))
    return lint_runs_on(ensure_roc(tools["roc"], key))


if __name__ == "__main__":
    raise SystemExit(main())
