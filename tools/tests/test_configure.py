"""Tests for configure.py."""

from __future__ import annotations

import os
import sys
from pathlib import Path
from unittest.mock import patch

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from configure import BuildConfig, find_qt_cmake, parse_version


class TestParseVersion:
    """Tests for parse_version function."""

    def test_parse_standard_version(self, tmp_path):
        """Test parsing standard Qt version path."""
        path = tmp_path / "Qt" / "6.8.0" / "gcc_64" / "bin" / "qt-cmake"
        version = parse_version(path)
        assert version == (6, 8, 0)

    def test_parse_double_digit_version(self, tmp_path):
        """Test parsing version with double-digit minor."""
        path = tmp_path / "Qt" / "6.10.1" / "gcc_64" / "bin" / "qt-cmake"
        version = parse_version(path)
        assert version == (6, 10, 1)

    def test_parse_no_version(self, tmp_path):
        """Test path with no version returns zeros."""
        path = tmp_path / "usr" / "lib" / "qt6" / "bin" / "qt-cmake"
        version = parse_version(path)
        assert version == (0, 0, 0)


class TestFindQtCmake:
    """Tests for find_qt_cmake function."""

    def test_explicit_qt_root(self, tmp_path):
        """Test finding qt-cmake with explicit qt_root."""
        qt_root = tmp_path / "Qt" / "6.8.0" / "gcc_64"
        qt_cmake = qt_root / "bin" / "qt-cmake"
        qt_cmake.parent.mkdir(parents=True)
        qt_cmake.write_text("#!/bin/bash\n")
        qt_cmake.chmod(0o755)

        result = find_qt_cmake(qt_root)
        assert result == qt_cmake

    def test_env_qt_root_dir(self, tmp_path):
        """Test finding qt-cmake from QT_ROOT_DIR env var."""
        qt_root = tmp_path / "Qt" / "6.8.0" / "gcc_64"
        qt_cmake = qt_root / "bin" / "qt-cmake"
        qt_cmake.parent.mkdir(parents=True)
        qt_cmake.write_text("#!/bin/bash\n")
        qt_cmake.chmod(0o755)

        with patch.dict(os.environ, {"QT_ROOT_DIR": str(qt_root)}):
            result = find_qt_cmake()
            assert result == qt_cmake

    def test_not_found_returns_none(self, tmp_path):
        """Test returns None when qt-cmake not found."""
        with patch.dict(os.environ, {"HOME": str(tmp_path)}, clear=False):
            # Clear QT_ROOT_DIR if set
            env = os.environ.copy()
            env.pop("QT_ROOT_DIR", None)
            with patch.dict(os.environ, env, clear=True):
                result = find_qt_cmake()
                # May return None or fall through to system cmake
                assert result is None or result == Path("/usr/lib/qt6/bin/qt-cmake")


class TestBuildConfig:
    """Tests for BuildConfig dataclass."""

    def test_default_values(self):
        """Test default configuration values."""
        config = BuildConfig()
        assert config.build_dir == Path("build")
        assert config.build_type == "Debug"
        assert config.generator == "Ninja"
        assert config.testing is False
        assert config.coverage is False
        assert config.unity_build is False

    def test_custom_values(self):
        """Test custom configuration values."""
        config = BuildConfig(
            build_dir=Path("build-release"),
            build_type="Release",
            testing=True,
            coverage=True,
        )
        assert config.build_dir == Path("build-release")
        assert config.build_type == "Release"
        assert config.testing is True
        assert config.coverage is True


class TestConfigure:
    """Tests for configure function."""

    def test_cmake_args_debug(self, tmp_path):
        """Test CMake arguments for Debug build."""
        from configure import configure

        config = BuildConfig(source_dir=tmp_path, build_dir=tmp_path / "build")

        with patch("subprocess.run") as mock_run:
            mock_run.return_value.returncode = 0
            configure(config)

            args = mock_run.call_args[0][0]
            assert "-DCMAKE_BUILD_TYPE=Debug" in args
            assert "-DQGC_BUILD_TESTING=OFF" in args

    def test_cmake_args_testing(self, tmp_path):
        """Test CMake arguments with testing enabled."""
        from configure import configure

        config = BuildConfig(source_dir=tmp_path, build_dir=tmp_path / "build", testing=True)

        with patch("subprocess.run") as mock_run:
            mock_run.return_value.returncode = 0
            configure(config)

            args = mock_run.call_args[0][0]
            assert "-DQGC_BUILD_TESTING=ON" in args

    def test_cmake_args_unity_build(self, tmp_path):
        """Test CMake arguments with unity build enabled."""
        from configure import configure

        config = BuildConfig(
            source_dir=tmp_path,
            build_dir=tmp_path / "build",
            unity_build=True,
            unity_batch_size=32,
        )

        with patch("subprocess.run") as mock_run:
            mock_run.return_value.returncode = 0
            configure(config)

            args = mock_run.call_args[0][0]
            assert "-DCMAKE_UNITY_BUILD=ON" in args
            assert "-DCMAKE_UNITY_BUILD_BATCH_SIZE=32" in args

    def test_github_output(self, tmp_path):
        """Test GitHub Actions output is written."""
        from configure import configure

        config = BuildConfig(source_dir=tmp_path, build_dir=tmp_path / "build")
        github_output = tmp_path / "github_output"

        with patch("subprocess.run") as mock_run, \
             patch.dict(os.environ, {"GITHUB_OUTPUT": str(github_output)}):
            mock_run.return_value.returncode = 0
            configure(config)

            content = github_output.read_text()
            assert "build_dir=" in content
