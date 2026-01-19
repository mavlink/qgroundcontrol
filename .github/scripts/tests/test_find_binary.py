"""Tests for find_binary.py."""

from __future__ import annotations

import os
import sys
from pathlib import Path
from unittest.mock import patch

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from find_binary import BinaryInfo, detect_platform, find_binary, get_binary_name


class TestDetectPlatform:
    """Tests for detect_platform function."""

    def test_linux(self):
        """Test Linux detection."""
        with patch("sys.platform", "linux"):
            assert detect_platform() == "linux"

    def test_macos(self):
        """Test macOS detection."""
        with patch("sys.platform", "darwin"):
            assert detect_platform() == "macos"

    def test_windows(self):
        """Test Windows detection."""
        with patch("sys.platform", "win32"):
            assert detect_platform() == "windows"


class TestGetBinaryName:
    """Tests for get_binary_name function."""

    def test_linux_binary(self):
        """Test Linux binary name."""
        assert get_binary_name("linux") == "QGroundControl"

    def test_macos_binary(self):
        """Test macOS bundle name."""
        assert get_binary_name("macos") == "QGroundControl.app"

    def test_windows_binary(self):
        """Test Windows executable name."""
        assert get_binary_name("windows") == "QGroundControl.exe"


class TestBinaryInfo:
    """Tests for BinaryInfo dataclass."""

    def test_creation(self, tmp_path):
        """Test BinaryInfo creation."""
        info = BinaryInfo(
            path=tmp_path / "QGroundControl",
            name="QGroundControl",
            directory=tmp_path,
        )
        assert info.path == tmp_path / "QGroundControl"
        assert info.name == "QGroundControl"
        assert info.directory == tmp_path


class TestFindBinary:
    """Tests for find_binary function."""

    def test_find_single_config(self, tmp_path):
        """Test finding binary in single-config layout."""
        build_dir = tmp_path / "build"
        binary = build_dir / "QGroundControl"
        binary.parent.mkdir(parents=True)
        binary.write_text("binary")

        result = find_binary(build_dir, platform="linux")

        assert result is not None
        assert result.name == "QGroundControl"
        assert result.path.exists()

    def test_find_multi_config_release(self, tmp_path):
        """Test finding binary in multi-config Release layout."""
        build_dir = tmp_path / "build"
        binary = build_dir / "Release" / "QGroundControl"
        binary.parent.mkdir(parents=True)
        binary.write_text("binary")

        result = find_binary(build_dir, platform="linux")

        assert result is not None
        assert "Release" in str(result.path)

    def test_find_with_build_type_hint(self, tmp_path):
        """Test finding binary with build type hint."""
        build_dir = tmp_path / "build"

        # Create both Debug and Release
        debug_binary = build_dir / "Debug" / "QGroundControl"
        release_binary = build_dir / "Release" / "QGroundControl"
        debug_binary.parent.mkdir(parents=True)
        release_binary.parent.mkdir(parents=True)
        debug_binary.write_text("debug")
        release_binary.write_text("release")

        # Should prioritize Release when specified
        result = find_binary(build_dir, build_type="Release", platform="linux")

        assert result is not None
        assert "Release" in str(result.path)

    def test_find_windows_exe(self, tmp_path):
        """Test finding Windows executable."""
        build_dir = tmp_path / "build"
        binary = build_dir / "QGroundControl.exe"
        binary.parent.mkdir(parents=True)
        binary.write_text("binary")

        result = find_binary(build_dir, platform="windows")

        assert result is not None
        assert result.name == "QGroundControl.exe"

    def test_find_macos_app(self, tmp_path):
        """Test finding macOS .app bundle."""
        build_dir = tmp_path / "build"
        app_dir = build_dir / "QGroundControl.app"
        app_dir.mkdir(parents=True)

        result = find_binary(build_dir, platform="macos")

        assert result is not None
        assert result.name == "QGroundControl.app"

    def test_not_found_returns_none(self, tmp_path):
        """Test returns None when binary not found."""
        build_dir = tmp_path / "empty_build"
        build_dir.mkdir()

        result = find_binary(build_dir, platform="linux")

        assert result is None

    def test_recursive_search_fallback(self, tmp_path):
        """Test recursive search when standard paths don't exist."""
        build_dir = tmp_path / "build"
        # Put binary in a non-standard location
        binary = build_dir / "nested" / "dir" / "QGroundControl"
        binary.parent.mkdir(parents=True)
        binary.write_text("binary")

        result = find_binary(build_dir, platform="linux")

        assert result is not None
        assert result.path.exists()


class TestGitHubOutput:
    """Tests for GitHub Actions output."""

    def test_output_written(self, tmp_path):
        """Test GitHub output is written correctly."""
        from find_binary import output_github_actions

        output_file = tmp_path / "github_output"
        info = BinaryInfo(
            path=tmp_path / "QGroundControl",
            name="QGroundControl",
            directory=tmp_path,
        )

        with patch.dict(os.environ, {"GITHUB_OUTPUT": str(output_file)}):
            output_github_actions(info)

        content = output_file.read_text()
        assert "binary_path=" in content
        assert "binary_name=QGroundControl" in content
        assert "binary_dir=" in content
