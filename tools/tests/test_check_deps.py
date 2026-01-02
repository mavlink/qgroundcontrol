"""Tests for check_deps.py dependency checker."""

from __future__ import annotations

import json
import os
import sys
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from check_deps import (
    load_config,
    get_tool_version,
    SubmoduleStatus,
    ToolStatus,
    check_build_tools,
)


class TestSubmoduleStatus:
    """Tests for SubmoduleStatus dataclass."""

    def test_submodule_status_fields(self):
        """Test SubmoduleStatus field values."""
        status = SubmoduleStatus(
            path="libs/test",
            current_hash="abc123",
            branch="master",
            behind_count=0,
            up_to_date=True,
        )
        assert status.path == "libs/test"
        assert status.current_hash == "abc123"
        assert status.branch == "master"
        assert status.behind_count == 0
        assert status.up_to_date is True

    def test_outdated_submodule(self):
        """Test outdated submodule status."""
        status = SubmoduleStatus(
            path="libs/test",
            current_hash="abc123",
            branch="master",
            behind_count=5,
            up_to_date=False,
        )
        assert status.behind_count == 5
        assert status.up_to_date is False


class TestToolStatus:
    """Tests for ToolStatus dataclass."""

    def test_installed_tool(self):
        """Test installed tool status."""
        status = ToolStatus(name="cmake", installed=True, version="3.28.0")
        assert status.installed is True
        assert status.version == "3.28.0"

    def test_missing_tool(self):
        """Test missing tool status."""
        status = ToolStatus(name="ninja", installed=False)
        assert status.installed is False
        assert status.version == ""


class TestLoadConfig:
    """Tests for load_config function."""

    def test_load_valid_config(self, tmp_path):
        """Test loading valid build config."""
        config_dir = tmp_path / ".github"
        config_dir.mkdir()
        config_file = config_dir / "build-config.json"
        config_file.write_text(json.dumps({
            "qt_version": "6.8.0",
            "gstreamer_version": "1.24.0",
        }))

        config = load_config(tmp_path)
        assert config["qt_version"] == "6.8.0"
        assert config["gstreamer_version"] == "1.24.0"

    def test_load_missing_config(self, tmp_path):
        """Test loading missing config returns empty dict."""
        config = load_config(tmp_path)
        assert config == {}


class TestGetToolVersion:
    """Tests for get_tool_version function."""

    def test_cmake_version(self):
        """Test getting cmake version."""
        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(
                returncode=0,
                stdout="cmake version 3.28.0\n",
            )
            installed, version = get_tool_version("cmake")

        assert installed is True
        assert "3.28.0" in version

    def test_tool_not_found(self):
        """Test tool not found."""
        with patch("shutil.which", return_value=None):
            installed, version = get_tool_version("nonexistent-tool")

        assert installed is False
        assert version == ""

    def test_ninja_version(self):
        """Test getting ninja version."""
        with patch("shutil.which", return_value="/usr/bin/ninja"):
            with patch("subprocess.run") as mock_run:
                mock_run.return_value = MagicMock(
                    returncode=0,
                    stdout="1.11.1\n",
                )
                installed, version = get_tool_version("ninja")

        assert installed is True


class TestCheckBuildTools:
    """Tests for check_build_tools function."""

    def test_check_build_tools_returns_list(self):
        """Test check_build_tools returns list of ToolStatus."""
        with patch("check_deps.get_tool_version") as mock_get:
            mock_get.return_value = (True, "3.28.0")
            results = check_build_tools()

        assert isinstance(results, list)
        assert all(isinstance(r, ToolStatus) for r in results)

    def test_check_build_tools_includes_cmake(self):
        """Test check_build_tools includes cmake."""
        with patch("check_deps.get_tool_version") as mock_get:
            mock_get.return_value = (True, "3.28.0")
            results = check_build_tools()

        tool_names = [r.name for r in results]
        assert "cmake" in tool_names


class TestBuildConfigIntegration:
    """Integration tests for build config handling."""

    def test_full_config(self, tmp_path):
        """Test full build config parsing."""
        config_dir = tmp_path / ".github"
        config_dir.mkdir()
        config_file = config_dir / "build-config.json"
        config_file.write_text(json.dumps({
            "qt_version": "6.8.0",
            "gstreamer_version": "1.24.0",
            "ndk_version": "26.2.11394342",
            "xcode_version": "16.x",
        }))

        config = load_config(tmp_path)

        assert config["qt_version"] == "6.8.0"
        assert config["gstreamer_version"] == "1.24.0"
        assert config["ndk_version"] == "26.2.11394342"
        assert config["xcode_version"] == "16.x"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
