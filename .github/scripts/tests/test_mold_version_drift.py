"""Detect drift between mold_helper.PINNED_RELEASE and .github/build-config.json.

mold_helper.py hard-codes per-arch SHA256 digests tied to one mold release. The
cache action passes the version from build-config.json into the installer. If
those drift, runners hit a checksum mismatch deep in the install step — this
test surfaces it at lint/CI time instead.
"""

from __future__ import annotations

import json
from pathlib import Path

from mold_helper import PINNED_RELEASE

REPO_ROOT = Path(__file__).resolve().parents[3]
BUILD_CONFIG = REPO_ROOT / ".github" / "build-config.json"


def test_pinned_version_matches_build_config() -> None:
    config = json.loads(BUILD_CONFIG.read_text(encoding="utf-8"))
    assert config["mold_version"] == PINNED_RELEASE.version, (
        f"build-config.json mold_version ({config['mold_version']}) drifted from "
        f"mold_helper.PINNED_RELEASE.version ({PINNED_RELEASE.version}). Refresh the "
        f"per-arch SHA256 digests in .github/scripts/mold_helper.py for the new "
        f"version, then update PINNED_RELEASE.version."
    )


def test_sha_dict_covers_both_arches() -> None:
    assert set(PINNED_RELEASE.sha256) == {"x86_64", "aarch64"}
    for arch, sha in PINNED_RELEASE.sha256.items():
        assert len(sha) == 64, f"PINNED_RELEASE.sha256[{arch}] not a sha256 hex digest"
        int(sha, 16)
