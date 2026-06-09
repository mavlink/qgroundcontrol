#!/usr/bin/env python3
"""Detect drift between install_dependencies/_debian.py:JUST_VERSION and tools/uv.lock:rust-just.

Two independent install paths deliver `just`: the upstream-binary fallback in
install_dependencies/_debian.py (Debian/Ubuntu < 24.04) and rust-just from PyPI
(dev/CI venv via uv). They can silently skew, leaving local-vs-CI behaviour
different.

The Debian fallback pins an exact version (JUST_VERSION) while uv.lock resolves
to whatever satisfies the rust-just>=1.30 floor. The drift check requires the
exact apt-fallback version be >= the uv.lock version (so devs upgrading via
either path get at least the floor we test against in CI).
"""

from __future__ import annotations

import re
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parents[2]
# JUST_VERSION lives in install_dependencies/_debian.py since the package split.
INSTALL_DEPS = REPO_ROOT / "tools" / "setup" / "install_dependencies" / "_debian.py"
UV_LOCK = REPO_ROOT / "tools" / "uv.lock"

_JUST_VERSION_RE = re.compile(r'^JUST_VERSION\s*=\s*"([\d.]+)"', re.MULTILINE)
_RUST_JUST_BLOCK_RE = re.compile(
    r'\[\[package\]\]\s*\n'
    r'name\s*=\s*"rust-just"\s*\n'
    r'version\s*=\s*"([\d.]+)"',
    re.MULTILINE,
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
