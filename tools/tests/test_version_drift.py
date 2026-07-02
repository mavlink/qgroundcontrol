#!/usr/bin/env python3
"""Drift guards for the tools venv.

uv.lock vs pyproject.toml: a dep added without regenerating the lockfile makes
CI's `uv sync --frozen` fail with a confusing error — fail locally instead via
`uv lock --locked`, which exits non-zero when the lockfile would change.

JUST_VERSION vs rust-just: two independent install paths deliver `just` — the
upstream-binary fallback in install_dependencies/_debian.py (Debian/Ubuntu
< 24.04) and rust-just from PyPI (dev/CI venv via uv). The apt-fallback pin
(JUST_VERSION) must be >= the uv.lock rust-just version so devs on either path
get at least the floor CI tests against.
"""

from __future__ import annotations

import re
import shutil
import subprocess

import pytest

from ._helpers import TOOLS_DIR

INSTALL_DEPS = TOOLS_DIR / "setup" / "install_dependencies" / "_debian.py"
UV_LOCK = TOOLS_DIR / "uv.lock"

_JUST_VERSION_RE = re.compile(r'^JUST_VERSION\s*=\s*"([\d.]+)"', re.MULTILINE)
_RUST_JUST_BLOCK_RE = re.compile(
    r"\[\[package\]\]\s*\n" r'name\s*=\s*"rust-just"\s*\n' r'version\s*=\s*"([\d.]+)"',
    re.MULTILINE,
)


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


def _version_tuple(v: str) -> tuple[int, ...]:
    return tuple(int(part) for part in v.split("."))


def _parse_just_version() -> str:
    text = INSTALL_DEPS.read_text(encoding="utf-8")
    match = _JUST_VERSION_RE.search(text)
    assert match, "JUST_VERSION constant not found in install_dependencies/_debian.py"
    return match.group(1)


def _parse_rust_just_version() -> str:
    text = UV_LOCK.read_text(encoding="utf-8")
    match = _RUST_JUST_BLOCK_RE.search(text)
    assert match, "rust-just package not found in tools/uv.lock"
    return match.group(1)


def test_just_versions_in_sync() -> None:
    if not INSTALL_DEPS.exists() or not UV_LOCK.exists():
        pytest.skip("install_dependencies/_debian.py or uv.lock not in checkout")

    apt_fallback = _parse_just_version()
    rust_just = _parse_rust_just_version()

    if _version_tuple(apt_fallback) < _version_tuple(rust_just):
        pytest.fail(
            f"install_dependencies/_debian.py JUST_VERSION ({apt_fallback}) is older than "
            f"tools/uv.lock rust-just ({rust_just}). Devs hitting the Debian apt-fallback "
            f"path get an older binary than CI/dev-venv users. "
            f"Update JUST_VERSION in tools/setup/install_dependencies/_debian.py to {rust_just} or later."
        )
