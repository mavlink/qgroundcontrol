"""Guard the pinned SHA256 digests in the mold/ccache CI installer helpers.

Each helper hard-codes per-arch SHA256 digests bound to one release version, which
is the single source of truth (no longer mirrored in build-config.json). These
tests catch a malformed or missing digest before a runner hits an opaque checksum
mismatch mid-install.
"""

from __future__ import annotations

from ccache_helper import MACOS_BINARY_SHA256, WINDOWS_BINARY_SHA256
from mold_helper import PINNED_RELEASE


def test_mold_sha_dict_covers_both_arches() -> None:
    assert set(PINNED_RELEASE.sha256) == {"x86_64", "aarch64"}
    for arch, sha in PINNED_RELEASE.sha256.items():
        assert len(sha) == 64, f"PINNED_RELEASE.sha256[{arch}] not a sha256 hex digest"
        int(sha, 16)


def test_ccache_windows_sha_dict_covers_both_arches() -> None:
    assert set(WINDOWS_BINARY_SHA256) == {"x86_64", "aarch64"}
    for arch, sha in WINDOWS_BINARY_SHA256.items():
        assert len(sha) == 64, f"WINDOWS_BINARY_SHA256[{arch}] not a sha256 hex digest"


def test_ccache_macos_sha_is_hex_digest() -> None:
    assert len(MACOS_BINARY_SHA256) == 64
    int(MACOS_BINARY_SHA256, 16)
