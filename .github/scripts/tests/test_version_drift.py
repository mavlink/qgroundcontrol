"""Detect drift between pinned tool versions in helper scripts and .github/build-config.json.

The helpers hard-code SHA256 digests tied to one release; the cache actions pass
the version from build-config.json into the installers. If those drift, runners
hit a checksum mismatch deep in the install step — these tests surface it at
lint/CI time instead.
"""

from __future__ import annotations

import json

from _helpers import REPO_ROOT
from ccache_helper import (
    MACOS_BINARY_SHA256,
    PINNED_BINARY_VERSION,
    WINDOWS_BINARY_SHA256,
)
from mold_helper import PINNED_RELEASE

BUILD_CONFIG = REPO_ROOT / ".github" / "build-config.json"


def _config() -> dict:
    return json.loads(BUILD_CONFIG.read_text(encoding="utf-8"))


def test_mold_pinned_version_matches_build_config() -> None:
    config = _config()
    assert config["mold_version"] == PINNED_RELEASE.version, (
        f"build-config.json mold_version ({config['mold_version']}) drifted from "
        f"mold_helper.PINNED_RELEASE.version ({PINNED_RELEASE.version}). Refresh the "
        f"per-arch SHA256 digests in .github/scripts/mold_helper.py for the new "
        f"version, then update PINNED_RELEASE.version."
    )


def test_mold_sha_dict_covers_both_arches() -> None:
    assert set(PINNED_RELEASE.sha256) == {"x86_64", "aarch64"}
    for arch, sha in PINNED_RELEASE.sha256.items():
        assert len(sha) == 64, f"PINNED_RELEASE.sha256[{arch}] not a sha256 hex digest"
        int(sha, 16)


def test_ccache_pinned_version_matches_build_config() -> None:
    config = _config()
    assert config["ccache_version"] == PINNED_BINARY_VERSION, (
        f"build-config.json ccache_version ({config['ccache_version']}) drifted "
        f"from ccache_helper.PINNED_BINARY_VERSION ({PINNED_BINARY_VERSION}). "
        f"Refresh WINDOWS_BINARY_SHA256 and MACOS_BINARY_SHA256 in "
        f".github/scripts/ccache_helper.py for the new version, then update "
        f"PINNED_BINARY_VERSION."
    )


def test_ccache_windows_sha_dict_covers_both_arches() -> None:
    assert set(WINDOWS_BINARY_SHA256) == {"x86_64", "aarch64"}
    for arch, sha in WINDOWS_BINARY_SHA256.items():
        assert len(sha) == 64, f"WINDOWS_BINARY_SHA256[{arch}] not a sha256 hex digest"


def test_ccache_macos_sha_is_hex_digest() -> None:
    assert len(MACOS_BINARY_SHA256) == 64
    int(MACOS_BINARY_SHA256, 16)
