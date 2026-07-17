"""Validate the pinned mold and ccache installer digests."""

from __future__ import annotations

from ccache_helper import MACOS_BINARY_SHA256, WINDOWS_BINARY_SHA256
from mold_helper import PINNED_RELEASE


def test_installer_digests_cover_supported_architectures_and_are_sha256() -> None:
    digest_sets = (
        ("mold", PINNED_RELEASE.sha256, {"x86_64", "aarch64"}),
        ("ccache-windows", WINDOWS_BINARY_SHA256, {"x86_64", "aarch64"}),
        ("ccache-macos", {"universal": MACOS_BINARY_SHA256}, {"universal"}),
    )
    for name, digests, expected_arches in digest_sets:
        assert set(digests) == expected_arches, name
        for digest in digests.values():
            assert len(digest) == 64, name
            int(digest, 16)
