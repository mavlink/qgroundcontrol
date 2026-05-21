#!/usr/bin/env python3
"""Detect drift between tools/pyproject.toml and tools/uv.lock.

A dep added to pyproject.toml without regenerating uv.lock causes
`uv sync --frozen` (used in CI) to fail with a confusing error. Fail
locally instead via `uv lock --locked`, which exits non-zero when
the lockfile would change.
"""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_DIR = REPO_ROOT / "tools"


def test_uv_lock_in_sync_with_pyproject() -> None:
    if not (TOOLS_DIR / "uv.lock").exists():
        pytest.skip("tools/uv.lock not present")
    if not shutil.which("uv"):
        pytest.skip("uv not installed; CI must validate lockfile drift via its own uv install")

    result = subprocess.run(
        ["uv", "lock", "--locked", "--project", str(TOOLS_DIR)],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        pytest.fail(
            "tools/uv.lock is out of sync with tools/pyproject.toml.\n"
            "Run: (cd tools && uv lock) and commit the result.\n\n"
            f"uv stderr:\n{result.stderr}"
        )
