"""Tests for install_ccache.py."""

from __future__ import annotations

import os
import tempfile
from pathlib import Path
from unittest.mock import patch

import pytest

import sys
sys.path.insert(0, str(Path(__file__).parent.parent))

from install_ccache import CcacheConfig, CcacheInstaller


class TestCcacheInstaller:
    """Tests for CcacheInstaller class."""

    def test_validate_version_valid(self):
        """Test valid version formats."""
        assert CcacheInstaller.validate_version("4.12.2")
        assert CcacheInstaller.validate_version("4.12")
        assert CcacheInstaller.validate_version("5.0.0")

    def test_validate_version_invalid(self):
        """Test invalid version formats."""
        assert not CcacheInstaller.validate_version("4.12.2.1")
        assert not CcacheInstaller.validate_version("v4.12.2")
        assert not CcacheInstaller.validate_version("latest")
        assert not CcacheInstaller.validate_version("")

    def test_detect_arch_x86_64(self):
        """Test x86_64 architecture detection."""
        with patch("platform.machine", return_value="x86_64"):
            assert CcacheInstaller.detect_arch() == "x86_64"

    def test_detect_arch_amd64(self):
        """Test amd64 (alias) architecture detection."""
        with patch("platform.machine", return_value="amd64"):
            assert CcacheInstaller.detect_arch() == "x86_64"

    def test_detect_arch_arm64(self):
        """Test arm64 architecture detection."""
        with patch("platform.machine", return_value="arm64"):
            assert CcacheInstaller.detect_arch() == "aarch64"

    def test_detect_arch_aarch64(self):
        """Test aarch64 architecture detection."""
        with patch("platform.machine", return_value="aarch64"):
            assert CcacheInstaller.detect_arch() == "aarch64"

    def test_default_prefix_from_env(self):
        """Test prefix from CCACHE_PREFIX environment variable."""
        with patch.dict(os.environ, {"CCACHE_PREFIX": "/custom/path"}):
            assert CcacheInstaller._default_prefix() == Path("/custom/path")

    def test_default_prefix_fallback(self):
        """Test default prefix fallback to /usr/local."""
        with patch.dict(os.environ, {}, clear=True):
            os.environ.pop("CCACHE_PREFIX", None)
            assert CcacheInstaller._default_prefix() == Path("/usr/local")

    def test_read_max_size_from_config(self):
        """Test reading max_size from ccache.conf."""
        with tempfile.NamedTemporaryFile(mode="w", suffix=".conf", delete=False) as f:
            f.write("max_size = 5G\n")
            f.write("compiler_check = content\n")
            f.flush()
            config_path = Path(f.name)

        try:
            installer = CcacheInstaller(config_path=config_path)
            assert installer.max_size == "5G"
        finally:
            config_path.unlink()

    def test_read_max_size_missing_file(self):
        """Test max_size default when config file is missing."""
        installer = CcacheInstaller(config_path=Path("/nonexistent/path.conf"))
        assert installer.max_size == CcacheInstaller.DEFAULT_MAX_SIZE

    def test_get_config(self):
        """Test get_config returns correct named tuple."""
        installer = CcacheInstaller(version="4.10", arch="aarch64")
        config = installer.get_config()

        assert isinstance(config, CcacheConfig)
        assert config.version == "4.10"
        assert config.arch == "aarch64"
        assert config.max_size == CcacheInstaller.DEFAULT_MAX_SIZE

    def test_installer_with_custom_prefix(self):
        """Test installer with custom prefix."""
        custom_prefix = Path("/opt/ccache")
        installer = CcacheInstaller(prefix=custom_prefix)
        assert installer.prefix == custom_prefix


class TestCcacheConfig:
    """Tests for CcacheConfig named tuple."""

    def test_config_creation(self):
        """Test CcacheConfig creation."""
        config = CcacheConfig(version="4.12.2", arch="x86_64", max_size="2G")
        assert config.version == "4.12.2"
        assert config.arch == "x86_64"
        assert config.max_size == "2G"

    def test_config_immutable(self):
        """Test CcacheConfig is immutable."""
        config = CcacheConfig(version="4.12.2", arch="x86_64", max_size="2G")
        with pytest.raises(AttributeError):
            config.version = "5.0.0"
