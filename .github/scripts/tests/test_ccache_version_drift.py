"""Detect drift between PINNED_BINARY_VERSION and .github/build-config.json.

ccache_helper.py hard-codes SHA256 dicts (WINDOWS_BINARY_SHA256 and
MACOS_BINARY_SHA256) tied to one ccache release. The cache action passes the
version from build-config.json into the installer. If those drift, runners hit
a checksum mismatch deep in the install step — this test surfaces it at
lint/CI time instead.
"""

from __future__ import annotations

import json
from pathlib import Path

from ccache_helper import (
    MACOS_BINARY_SHA256,
    PINNED_BINARY_VERSION,
    WINDOWS_BINARY_SHA256,
)

REPO_ROOT = Path(__file__).resolve().parents[3]
BUILD_CONFIG = REPO_ROOT / ".github" / "build-config.json"


def test_pinned_version_matches_build_config() -> None:
    config = json.loads(BUILD_CONFIG.read_text(encoding="utf-8"))
    assert config["ccache_version"] == PINNED_BINARY_VERSION, (
        f"build-config.json ccache_version ({config['ccache_version']}) drifted "
        f"from ccache_helper.PINNED_BINARY_VERSION ({PINNED_BINARY_VERSION}). "
        f"Refresh WINDOWS_BINARY_SHA256 and MACOS_BINARY_SHA256 in "
        f".github/scripts/ccache_helper.py for the new version, then update "
        f"PINNED_BINARY_VERSION."
    )


def test_windows_sha_dict_covers_both_arches() -> None:
    assert set(WINDOWS_BINARY_SHA256) == {"x86_64", "aarch64"}
    for arch, sha in WINDOWS_BINARY_SHA256.items():
        assert len(sha) == 64, f"WINDOWS_BINARY_SHA256[{arch}] not a sha256 hex digest"


def test_macos_sha_is_hex_digest() -> None:
    assert len(MACOS_BINARY_SHA256) == 64
    int(MACOS_BINARY_SHA256, 16)
