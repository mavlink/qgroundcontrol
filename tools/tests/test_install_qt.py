#!/usr/bin/env python3
"""Tests for tools/setup/install_qt.py."""

from __future__ import annotations

import pytest

from setup.install_qt import compute_cache_digest, resolve_android_qt_root, resolve_arch_dir, resolve_qt_root


class TestResolveArchDir:
    def test_linux_gcc_64(self) -> None:
        assert resolve_arch_dir("linux_gcc_64") == "gcc_64"

    def test_linux_arm64(self) -> None:
        assert resolve_arch_dir("linux_arm64") == "arm64"

    def test_win64_msvc2022_64(self) -> None:
        assert resolve_arch_dir("win64_msvc2022_64") == "msvc2022_64"

    def test_win64_msvc2022_arm64_cross_compiled(self) -> None:
        assert resolve_arch_dir("win64_msvc2022_arm64_cross_compiled") == "msvc2022_arm64"

    def test_clang_64_maps_to_macos(self) -> None:
        assert resolve_arch_dir("clang_64") == "macos"

    def test_android_arm64_v8a_unchanged(self) -> None:
        assert resolve_arch_dir("android_arm64_v8a") == "android_arm64_v8a"

    def test_ios_unchanged(self) -> None:
        assert resolve_arch_dir("ios") == "ios"


class TestComputeCacheDigest:
    def test_deterministic(self) -> None:
        a = compute_cache_digest("qtcharts qtlocation", "")
        b = compute_cache_digest("qtcharts qtlocation", "")
        assert a == b

    def test_different_modules_differ(self) -> None:
        a = compute_cache_digest("qtcharts", "")
        b = compute_cache_digest("qtlocation", "")
        assert a != b

    def test_archives_affect_digest(self) -> None:
        a = compute_cache_digest("qtcharts", "")
        b = compute_cache_digest("qtcharts", "icu")
        assert a != b

    def test_returns_hex_string(self) -> None:
        d = compute_cache_digest("", "")
        assert len(d) == 64
        assert all(c in "0123456789abcdef" for c in d)


class TestResolveQtRoot:
    def test_valid_path(self, tmp_path: "pytest.TempPathFactory") -> None:
        qt_root = tmp_path / "6.8.3" / "gcc_64"
        qt_root.mkdir(parents=True)
        result = resolve_qt_root(tmp_path, "6.8.3", "gcc_64")
        assert result == qt_root

    def test_missing_path_exits(self, tmp_path: "pytest.TempPathFactory") -> None:
        with pytest.raises(SystemExit):
            resolve_qt_root(tmp_path, "6.8.3", "gcc_64")


class TestResolveAndroidQtRoot:
    def test_arm64_preferred(self) -> None:
        roots = {"arm64": "/qt/arm64", "armv7": "/qt/armv7"}
        assert resolve_android_qt_root("arm64-v8a;armeabi-v7a", roots) == "/qt/arm64"

    def test_armv7_fallback(self) -> None:
        roots = {"arm64": "", "armv7": "/qt/armv7"}
        assert resolve_android_qt_root("arm64-v8a;armeabi-v7a", roots) == "/qt/armv7"

    def test_x86_64_only(self) -> None:
        roots = {"x86_64": "/qt/x86_64"}
        assert resolve_android_qt_root("x86_64", roots) == "/qt/x86_64"

    def test_x86_only(self) -> None:
        roots = {"x86": "/qt/x86"}
        assert resolve_android_qt_root("x86", roots) == "/qt/x86"

    def test_no_match_exits(self) -> None:
        with pytest.raises(SystemExit):
            resolve_android_qt_root("mips", {})

    def test_empty_root_skipped(self) -> None:
        roots = {"arm64": "", "x86_64": "/qt/x86_64"}
        assert resolve_android_qt_root("arm64-v8a;x86_64", roots) == "/qt/x86_64"

    def test_semicolon_parsing(self) -> None:
        roots = {"armv7": "/qt/armv7"}
        assert resolve_android_qt_root("armeabi-v7a", roots) == "/qt/armv7"
