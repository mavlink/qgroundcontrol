"""Drift guards for the tools venv.

uv.lock vs pyproject.toml: a dep added without regenerating the lockfile makes
CI's `uv sync --frozen` fail with a confusing error — fail locally instead via
`uv lock --locked`, which exits non-zero when the lockfile would change.

just binary vs rust-just: two independent install paths deliver `just` — the
upstream-binary fallback in install_dependencies/_debian.py (Debian/Ubuntu
< 24.04) and rust-just from PyPI (dev/CI venv via uv). _debian derives its
version from uv.lock's rust-just at runtime so the two can't drift; this
guards that the derivation still resolves against the real lockfile.
"""

from __future__ import annotations

import shutil
import subprocess

import pytest
from common.io import read_toml

from ._helpers import TOOLS_DIR

UV_LOCK = TOOLS_DIR / "uv.lock"


def test_uv_lock_in_sync_with_pyproject() -> None:
    if not UV_LOCK.exists():
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


def _lock_version(package: str) -> str:
    """Independently parse *package*'s pin from uv.lock (not via the helper under test)."""
    return next(
        entry["version"] for entry in read_toml(UV_LOCK)["package"] if entry["name"] == package
    )


def _derived_versions() -> dict[str, str]:
    """The upstream binary fallback uses the same lock as the Python build profile."""
    from setup.install_dependencies._debian import _just_version

    return {"rust-just": _just_version()}


@pytest.mark.parametrize("package", ["rust-just"])
def test_fallback_version_derives_from_uv_lock(package: str) -> None:
    if not UV_LOCK.exists():
        pytest.skip("tools/uv.lock not in checkout")

    derived = _derived_versions()[package]
    pinned = _lock_version(package)

    assert derived == pinned, (
        f"{package}: setup script resolved {derived!r} but tools/uv.lock pins {pinned!r}. "
        f"common.tool_version.uv_lock_version is broken or the constant's fallback is "
        f"shadowing the lock, so the fallback-install path would ship a stale binary."
    )
