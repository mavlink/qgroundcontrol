#!/usr/bin/env python3
"""Contracts for mirrored GStreamer artifact selection."""

from __future__ import annotations

import pytest
from mirror_gstreamer import PLATFORMS, Artifact, artifacts_for, resolve_platforms


def test_artifact_sets_match_each_supported_platform() -> None:
    version = "1.28.3"
    expected = {
        "android": [f"gstreamer-1.0-android-universal-{version}.tar.xz"],
        "ios": [f"gstreamer-1.0-devel-{version}-ios-universal.pkg"],
        "macos": [
            f"gstreamer-1.0-{version}-universal.pkg",
            f"gstreamer-1.0-devel-{version}-universal.pkg",
        ],
        "windows": [
            f"gstreamer-1.0-msvc-arm64-{version}.exe",
            f"gstreamer-1.0-msvc-x86_64-{version}.exe",
        ],
    }
    for platform, names in expected.items():
        artifacts = artifacts_for(platform, version)
        assert sorted(artifact.filename for artifact in artifacts) == names
        assert {artifact.s3_dir for artifact in artifacts} == {platform}

    android = artifacts_for("android", version)[0]
    assert android.s3_key() == f"dependencies/gstreamer/android/{android.filename}"
    assert artifacts_for("ios", version)[0].url.startswith(
        "https://gstreamer.freedesktop.org/data/pkg/ios/"
    )


def test_artifact_selection_rejects_unsupported_inputs() -> None:
    with pytest.raises(ValueError, match="requires version"):
        artifacts_for("windows", "1.26.5")
    with pytest.raises(ValueError, match="Unknown platform"):
        artifacts_for("plugin", "1.28.3")


def test_platform_resolution_and_s3_key() -> None:
    assert resolve_platforms("") == list(PLATFORMS)
    assert resolve_platforms("all") == list(PLATFORMS)
    assert resolve_platforms("windows, android") == ["windows", "android"]
    with pytest.raises(ValueError, match="Unknown platform"):
        resolve_platforms("android,plugin")

    artifact = Artifact("https://example.test/pkg/file.tar.xz", "android")
    assert artifact.s3_key() == "dependencies/gstreamer/android/file.tar.xz"
