#!/usr/bin/env python3
"""Tests for install_dependencies.py."""

import subprocess
import sys
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent / "setup"))


class TestPlatformDetection:
    """Tests for platform detection."""

    def test_detect_platform_linux(self) -> None:
        """Verify Linux platform detection."""
        from install_dependencies import detect_platform

        with patch("sys.platform", "linux"):
            with patch("pathlib.Path.exists", return_value=True):
                result = detect_platform()
                assert result in ("debian", "linux")

    def test_detect_platform_macos(self) -> None:
        """Verify macOS platform detection."""
        from install_dependencies import detect_platform

        with patch("sys.platform", "darwin"):
            assert detect_platform() == "macos"

    def test_detect_platform_unsupported(self) -> None:
        """Verify unsupported platform returns None."""
        from install_dependencies import detect_platform

        with patch("sys.platform", "win32"):
            assert detect_platform() is None


class TestDebianPackages:
    """Tests for Debian package handling."""

    def test_get_debian_packages(self) -> None:
        """Verify Debian package list is populated."""
        from install_dependencies import get_debian_packages

        packages = get_debian_packages()
        assert len(packages) > 0
        assert "build-essential" in packages
        assert "cmake" in packages
        assert "ninja-build" in packages

    def test_get_debian_packages_by_category(self) -> None:
        """Verify packages can be filtered by category."""
        from install_dependencies import DEBIAN_PACKAGES

        assert "core" in DEBIAN_PACKAGES
        assert "qt" in DEBIAN_PACKAGES
        assert "gstreamer" in DEBIAN_PACKAGES

    def test_debian_packages_no_duplicates(self) -> None:
        """Verify no duplicate packages."""
        from install_dependencies import get_debian_packages

        packages = get_debian_packages()
        assert len(packages) == len(set(packages))


class TestMacOSPackages:
    """Tests for macOS package handling."""

    def test_get_macos_packages(self) -> None:
        """Verify macOS package list is populated."""
        from install_dependencies import get_macos_packages

        packages = get_macos_packages()
        assert len(packages) > 0
        assert "cmake" in packages
        assert "ninja" in packages

    def test_macos_packages_no_duplicates(self) -> None:
        """Verify no duplicate packages."""
        from install_dependencies import get_macos_packages

        packages = get_macos_packages()
        assert len(packages) == len(set(packages))


class TestPackageManagerDetection:
    """Tests for package manager detection."""

    def test_has_apt(self) -> None:
        """Verify apt detection."""
        from install_dependencies import has_command

        with patch("shutil.which", return_value="/usr/bin/apt-get"):
            assert has_command("apt-get") is True

    def test_has_brew(self) -> None:
        """Verify brew detection."""
        from install_dependencies import has_command

        with patch("shutil.which", return_value="/usr/local/bin/brew"):
            assert has_command("brew") is True

    def test_command_not_found(self) -> None:
        """Verify missing command detection."""
        from install_dependencies import has_command

        with patch("shutil.which", return_value=None):
            assert has_command("nonexistent") is False


class TestInstallCommands:
    """Tests for install command generation."""

    def test_get_apt_install_command(self) -> None:
        """Verify apt install command generation."""
        from install_dependencies import get_apt_install_command

        cmd = get_apt_install_command(["pkg1", "pkg2"])
        assert "apt-get" in cmd
        assert "install" in cmd
        assert "-y" in cmd
        assert "pkg1" in cmd
        assert "pkg2" in cmd

    def test_get_brew_install_command(self) -> None:
        """Verify brew install command generation."""
        from install_dependencies import get_brew_install_command

        cmd = get_brew_install_command(["pkg1", "pkg2"])
        assert "brew" in cmd
        assert "install" in cmd
        assert "pkg1" in cmd
        assert "pkg2" in cmd


class TestOptionalPackages:
    """Tests for optional package handling."""

    def test_check_package_available_apt(self) -> None:
        """Verify apt package availability check."""
        from install_dependencies import check_apt_package_available

        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            assert check_apt_package_available("existing-pkg") is True

    def test_check_package_unavailable_apt(self) -> None:
        """Verify apt package unavailability check."""
        from install_dependencies import check_apt_package_available

        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=1)
            assert check_apt_package_available("nonexistent-pkg") is False


class TestArgumentParsing:
    """Tests for CLI argument parsing."""

    def test_default_args(self) -> None:
        """Verify default arguments."""
        from install_dependencies import parse_args

        args = parse_args([])
        assert args.dry_run is False
        assert args.platform is None

    def test_dry_run_flag(self) -> None:
        """Verify --dry-run flag."""
        from install_dependencies import parse_args

        args = parse_args(["--dry-run"])
        assert args.dry_run is True

    def test_platform_override(self) -> None:
        """Verify --platform override."""
        from install_dependencies import parse_args

        args = parse_args(["--platform", "debian"])
        assert args.platform == "debian"

    def test_list_packages_flag(self) -> None:
        """Verify --list flag."""
        from install_dependencies import parse_args

        args = parse_args(["--list"])
        assert args.list_packages is True

    def test_category_filter(self) -> None:
        """Verify --category filter."""
        from install_dependencies import parse_args

        args = parse_args(["--category", "core"])
        assert args.category == "core"


class TestConfigReading:
    """Tests for build config reading."""

    def test_get_gstreamer_version(self) -> None:
        """Verify GStreamer version reading from config."""
        from install_dependencies import get_config_value

        mock_config = {"gstreamer": {"macos_version": "1.24.0"}}
        with patch("install_dependencies.load_build_config", return_value=mock_config):
            version = get_config_value("gstreamer.macos_version")
            assert version == "1.24.0"

    def test_get_config_missing_key(self) -> None:
        """Verify missing config key returns None."""
        from install_dependencies import get_config_value

        mock_config = {}
        with patch("install_dependencies.load_build_config", return_value=mock_config):
            value = get_config_value("nonexistent.key")
            assert value is None


class TestGStreamerMacOS:
    """Tests for macOS GStreamer installation."""

    def test_get_gstreamer_urls(self) -> None:
        """Verify GStreamer download URL generation."""
        from install_dependencies import get_gstreamer_macos_urls

        urls = get_gstreamer_macos_urls("1.24.0")
        assert len(urls) == 2
        assert "1.24.0" in urls[0]
        assert "universal.pkg" in urls[0]
        assert "devel" in urls[1]


class TestPipxPackages:
    """Tests for pipx package handling."""

    def test_get_pipx_packages(self) -> None:
        """Verify pipx package list."""
        from install_dependencies import PIPX_PACKAGES

        assert "cmake" in PIPX_PACKAGES
        assert "ninja" in PIPX_PACKAGES
        assert "gcovr" in PIPX_PACKAGES
