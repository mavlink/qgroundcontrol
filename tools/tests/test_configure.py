#!/usr/bin/env python3
"""Tests for tools/configure.py."""

from __future__ import annotations

from pathlib import Path
from unittest.mock import patch

from configure import BuildConfig, find_qt_cmake, parse_version


class TestParseVersion:
    def test_standard_version(self) -> None:
        path = Path("/home/user/Qt/6.8.0/gcc_64/bin/qt-cmake")
        assert parse_version(path) == (6, 8, 0)

    def test_double_digit_minor(self) -> None:
        path = Path("/home/user/Qt/6.10.1/gcc_64/bin/qt-cmake")
        assert parse_version(path) == (6, 10, 1)

    def test_no_version_returns_zeros(self) -> None:
        path = Path("/usr/bin/qt-cmake")
        assert parse_version(path) == (0, 0, 0)


class TestBuildConfig:
    def test_defaults(self) -> None:
        config = BuildConfig()
        assert config.build_type == "Debug"
        assert config.generator == "Ninja"
        assert config.testing is False
        assert config.coverage is False
        assert config.unity_build is False

    def test_custom_values(self) -> None:
        config = BuildConfig(
            build_type="Release",
            testing=True,
            unity_build=True,
            unity_batch_size=32,
        )
        assert config.build_type == "Release"
        assert config.testing is True
        assert config.unity_batch_size == 32


class TestFindQtCmake:
    def test_returns_none_when_not_found(self, tmp_path: Path) -> None:
        with patch("configure.Path.home", return_value=tmp_path), \
             patch.dict("os.environ", {}, clear=True):
            result = find_qt_cmake(tmp_path / "nonexistent")
        assert result is None

    def test_finds_explicit_path(self, tmp_path: Path) -> None:
        qt_cmake = tmp_path / "bin" / "qt-cmake"
        qt_cmake.parent.mkdir(parents=True)
        qt_cmake.touch(mode=0o755)
        result = find_qt_cmake(tmp_path)
        assert result is not None
        assert result.name == "qt-cmake"
