"""Drift guard for the tools venv.

uv.lock vs pyproject.toml: a dep added without regenerating the lockfile makes
CI's `uv sync --frozen` fail with a confusing error — fail locally instead via
`uv lock --locked`, which exits non-zero when the lockfile would change.
"""

from __future__ import annotations

import shutil
import subprocess

import pytest

from ._helpers import TOOLS_DIR


def test_uv_lock_in_sync_with_pyproject() -> None:
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
