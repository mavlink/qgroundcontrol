"""Tests for gstreamer_archive.py."""

from __future__ import annotations

import os
from pathlib import Path
from unittest.mock import patch

import pytest

from gstreamer_archive import ArchiveResult, GStreamerArchiver


class TestGStreamerArchiver:
    """Tests for GStreamerArchiver class."""

    def test_valid_platforms(self):
        """Test all valid platforms are accepted."""
        for platform in ["linux", "macos", "windows", "android", "ios"]:
            archiver = GStreamerArchiver(platform=platform, arch="x86_64", version="1.24.0")
            assert archiver.platform == platform

    def test_invalid_platform(self):
        """Test invalid platform raises ValueError."""
        with pytest.raises(ValueError, match="Unknown platform"):
            GStreamerArchiver(platform="freebsd", arch="x86_64", version="1.24.0")

    def test_default_prefix_linux(self):
        """Test default prefix for Linux."""
        with patch.dict(os.environ, {"RUNNER_TEMP": "/tmp/runner"}):
            archiver = GStreamerArchiver(platform="linux", arch="x86_64", version="1.24.0")
            assert archiver.prefix == Path("/tmp/runner/gstreamer")

    def test_default_prefix_windows(self):
        """Test default prefix for Windows."""
        archiver = GStreamerArchiver(platform="windows", arch="x86_64", version="1.24.0")
        assert "gstreamer" in str(archiver.prefix).lower()

    def test_default_prefix_from_env(self):
        """Test prefix from GSTREAMER_PREFIX environment variable."""
        with patch.dict(os.environ, {"GSTREAMER_PREFIX": "/custom/gstreamer"}):
            archiver = GStreamerArchiver(platform="linux", arch="x86_64", version="1.24.0")
            assert archiver.prefix == Path("/custom/gstreamer")

    def test_normalize_arch_windows(self):
        """Test architecture normalization for Windows."""
        archiver = GStreamerArchiver(platform="windows", arch="x86_64", version="1.24.0")
        assert archiver._normalize_arch() == "x64"

    def test_normalize_arch_linux(self):
        """Test architecture normalization for Linux (no change)."""
        archiver = GStreamerArchiver(platform="linux", arch="x86_64", version="1.24.0")
        assert archiver._normalize_arch() == "x86_64"

    def test_get_archive_name_linux(self):
        """Test archive name generation for Linux."""
        archiver = GStreamerArchiver(platform="linux", arch="x86_64", version="1.24.0")
        assert archiver.get_archive_name() == "gstreamer-1.0-linux-x86_64-1.24.0"

    def test_get_archive_name_windows(self):
        """Test archive name generation for Windows."""
        archiver = GStreamerArchiver(platform="windows", arch="x86_64", version="1.24.0")
        assert archiver.get_archive_name() == "gstreamer-1.0-msvc-x64-1.24.0"

    def test_get_archive_name_macos(self):
        """Test archive name generation for macOS."""
        archiver = GStreamerArchiver(platform="macos", arch="arm64", version="1.24.0")
        assert archiver.get_archive_name() == "gstreamer-1.0-macos-arm64-1.24.0"

    def test_get_archive_name_ios_device(self):
        """Test archive name generation for iOS device."""
        archiver = GStreamerArchiver(platform="ios", arch="arm64", version="1.24.0")
        assert archiver.get_archive_name() == "gstreamer-1.0-ios-arm64-1.24.0"

    def test_get_archive_name_ios_simulator(self):
        """Test archive name generation for iOS simulator."""
        archiver = GStreamerArchiver(platform="ios", arch="arm64", version="1.24.0", simulator=True)
        assert archiver.get_archive_name() == "gstreamer-1.0-ios-arm64-simulator-1.24.0"

    def test_get_extension_windows(self):
        """Test file extension for Windows (zip)."""
        archiver = GStreamerArchiver(platform="windows", arch="x86_64", version="1.24.0")
        assert archiver._get_extension() == "zip"

    def test_get_extension_linux(self):
        """Test file extension for Linux (tar.xz)."""
        archiver = GStreamerArchiver(platform="linux", arch="x86_64", version="1.24.0")
        assert archiver._get_extension() == "tar.xz"

    def test_custom_output_dir(self):
        """Test custom output directory."""
        custom_dir = Path("/custom/output")
        archiver = GStreamerArchiver(
            platform="linux",
            arch="x86_64",
            version="1.24.0",
            output_dir=custom_dir,
        )
        assert archiver.output_dir == custom_dir

    def test_custom_prefix(self):
        """Test custom prefix."""
        custom_prefix = Path("/opt/gstreamer")
        archiver = GStreamerArchiver(
            platform="linux",
            arch="x86_64",
            version="1.24.0",
            prefix=custom_prefix,
        )
        assert archiver.prefix == custom_prefix


class TestArchiveResult:
    """Tests for ArchiveResult dataclass."""

    def test_archive_result_creation(self):
        """Test ArchiveResult creation."""
        result = ArchiveResult(
            name="gstreamer-1.0-linux-x86_64-1.24.0",
            path=Path("/tmp/archive.tar.xz"),
            extension="tar.xz",
        )
        assert result.name == "gstreamer-1.0-linux-x86_64-1.24.0"
        assert result.path == Path("/tmp/archive.tar.xz")
        assert result.extension == "tar.xz"
