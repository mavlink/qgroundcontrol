"""Run commands and detect host resources for the GStreamer builders."""

from __future__ import annotations

import os
import platform
import subprocess
from typing import TYPE_CHECKING

from common.logging import log_info

if TYPE_CHECKING:
    from pathlib import Path


def run_cmd(
    cmd: list, cwd: Path | None = None, check: bool = True, env: dict | None = None
) -> subprocess.CompletedProcess:
    """Run a command with optional working directory and environment."""
    merged_env = {**os.environ, **(env or {})}
    log_info(f"Running: {' '.join(str(c) for c in cmd)}")
    return subprocess.run(cmd, cwd=cwd, check=check, env=merged_env)


def detect_jobs(override: int | None = None) -> int:
    """Detect number of parallel jobs to use."""
    if override:
        return override
    return os.cpu_count() or 4


def detect_host_arch() -> str:
    """Detect the host architecture."""
    machine = platform.machine().lower()
    if machine in ("x86_64", "amd64"):
        return "x86_64"
    if machine in ("aarch64", "arm64"):
        return "arm64"
    if machine.startswith("arm"):
        return "armv7"
    return machine
