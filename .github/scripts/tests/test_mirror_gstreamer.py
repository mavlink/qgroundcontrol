#!/usr/bin/env python3
"""Tests for mirror_gstreamer.py."""

from __future__ import annotations

import pytest
from mirror_gstreamer import PLATFORMS, Artifact, artifacts_for, resolve_platforms


def test_android_single_artifact() -> None:
    arts = artifacts_for("android", "1.28.3")
    assert len(arts) == 1
    assert arts[0].filename == "gstreamer-1.0-android-universal-1.28.3.tar.xz"
    assert arts[0].s3_key() == "dependencies/gstreamer/android/gstreamer-1.0-android-universal-1.28.3.tar.xz"


def test_macos_has_runtime_and_devel() -> None:
    names = sorted(a.filename for a in artifacts_for("macos", "1.28.3"))
    assert names == [
        "gstreamer-1.0-1.28.3-universal.pkg",
        "gstreamer-1.0-devel-1.28.3-universal.pkg",
    ]
    assert {a.s3_dir for a in artifacts_for("macos", "1.28.3")} == {"macos"}


def test_windows_has_both_arches() -> None:
    names = sorted(a.filename for a in artifacts_for("windows", "1.28.3"))
    assert names == [
        "gstreamer-1.0-msvc-arm64-1.28.3.exe",
        "gstreamer-1.0-msvc-x86_64-1.28.3.exe",
    ]


def test_windows_rejects_pre_1_28() -> None:
    with pytest.raises(ValueError, match="requires version"):
        artifacts_for("windows", "1.26.5")


def test_ios_devel_pkg() -> None:
    (art,) = artifacts_for("ios", "1.28.3")
    assert art.filename == "gstreamer-1.0-devel-1.28.3-ios-universal.pkg"
    assert art.url.startswith("https://gstreamer.freedesktop.org/data/pkg/ios/")


def test_unknown_platform_raises() -> None:
    with pytest.raises(ValueError, match="Unknown platform"):
        artifacts_for("plugin", "1.28.3")


def test_resolve_platforms_all() -> None:
    assert resolve_platforms("all") == list(PLATFORMS)
    assert resolve_platforms("") == list(PLATFORMS)


def test_resolve_platforms_subset_preserves_order() -> None:
    assert resolve_platforms("windows, android") == ["windows", "android"]


def test_resolve_platforms_rejects_unknown() -> None:
    with pytest.raises(ValueError, match="Unknown platform"):
        resolve_platforms("android,plugin")


def test_s3_key_uses_url_basename() -> None:
    art = Artifact("https://example/data/pkg/android/1.28.3/file-1.28.3.tar.xz", "android")
    assert art.s3_key() == "dependencies/gstreamer/android/file-1.28.3.tar.xz"
