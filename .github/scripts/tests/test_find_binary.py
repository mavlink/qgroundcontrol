"""Tests for find_binary.py."""

from __future__ import annotations

import os
from pathlib import Path
from unittest.mock import patch

from find_binary import BinaryInfo, detect_platform, find_binary, get_binary_name, output_github_actions


class TestDetectPlatform:
    def test_linux(self) -> None:
        with patch("sys.platform", "linux"):
            assert detect_platform() == "linux"

    def test_macos(self) -> None:
        with patch("sys.platform", "darwin"):
            assert detect_platform() == "macos"

    def test_windows(self) -> None:
        with patch("sys.platform", "win32"):
            assert detect_platform() == "windows"


class TestGetBinaryName:
    def test_linux_binary(self) -> None:
        assert get_binary_name("linux") == "QGroundControl"

    def test_macos_binary(self) -> None:
        assert get_binary_name("macos") == "QGroundControl.app"

    def test_windows_binary(self) -> None:
        assert get_binary_name("windows") == "QGroundControl.exe"


class TestFindBinary:
    def test_find_single_config(self, tmp_path: Path) -> None:
        build_dir = tmp_path / "build"
        binary = build_dir / "QGroundControl"
        binary.parent.mkdir(parents=True)
        binary.write_text("binary", encoding="utf-8")

        result = find_binary(build_dir, platform="linux")
        assert result is not None
        assert result.name == "QGroundControl"
        assert result.path.exists()

    def test_find_multi_config_release(self, tmp_path: Path) -> None:
        build_dir = tmp_path / "build"
        binary = build_dir / "Release" / "QGroundControl"
        binary.parent.mkdir(parents=True)
        binary.write_text("binary", encoding="utf-8")

        result = find_binary(build_dir, platform="linux")
        assert result is not None
        assert "Release" in str(result.path)

    def test_find_with_build_type_hint(self, tmp_path: Path) -> None:
        build_dir = tmp_path / "build"
        debug_binary = build_dir / "Debug" / "QGroundControl"
        release_binary = build_dir / "Release" / "QGroundControl"
        debug_binary.parent.mkdir(parents=True)
        release_binary.parent.mkdir(parents=True)
        debug_binary.write_text("debug", encoding="utf-8")
        release_binary.write_text("release", encoding="utf-8")

        result = find_binary(build_dir, build_type="Release", platform="linux")
        assert result is not None
        assert "Release" in str(result.path)

    def test_find_windows_exe(self, tmp_path: Path) -> None:
        build_dir = tmp_path / "build"
        binary = build_dir / "QGroundControl.exe"
        binary.parent.mkdir(parents=True)
        binary.write_text("binary", encoding="utf-8")

        result = find_binary(build_dir, platform="windows")
        assert result is not None
        assert result.name == "QGroundControl.exe"

    def test_find_macos_app(self, tmp_path: Path) -> None:
        build_dir = tmp_path / "build"
        app_dir = build_dir / "QGroundControl.app"
        app_dir.mkdir(parents=True)

        result = find_binary(build_dir, platform="macos")
        assert result is not None
        assert result.name == "QGroundControl.app"

    def test_not_found_returns_none(self, tmp_path: Path) -> None:
        build_dir = tmp_path / "empty_build"
        build_dir.mkdir()
        assert find_binary(build_dir, platform="linux") is None

    def test_recursive_search_fallback(self, tmp_path: Path) -> None:
        build_dir = tmp_path / "build"
        binary = build_dir / "nested" / "dir" / "QGroundControl"
        binary.parent.mkdir(parents=True)
        binary.write_text("binary", encoding="utf-8")

        result = find_binary(build_dir, platform="linux")
        assert result is not None
        assert result.path.exists()


class TestGitHubOutput:
    def test_output_written(self, tmp_path: Path) -> None:
        output_file = tmp_path / "github_output"
        info = BinaryInfo(
            path=tmp_path / "QGroundControl",
            name="QGroundControl",
            directory=tmp_path,
        )

        with patch.dict(os.environ, {"GITHUB_OUTPUT": str(output_file)}):
            output_github_actions(info)

        content = output_file.read_text(encoding="utf-8")
        assert "binary_path=" in content
        assert "binary_name=QGroundControl" in content
        assert "binary_dir=" in content
