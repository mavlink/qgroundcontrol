#!/usr/bin/env python3
"""Tests for run-arducopter-sitl.py."""

import subprocess
import sys
from pathlib import Path
from unittest.mock import MagicMock, call, patch

import pytest

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent / "simulation"))


class TestDockerDetection:
    """Tests for Docker availability detection."""

    def test_has_docker_when_present(self) -> None:
        """Verify Docker detection when available."""
        from run_arducopter_sitl import has_docker

        with patch("shutil.which", return_value="/usr/bin/docker"):
            assert has_docker() is True

    def test_has_docker_when_absent(self) -> None:
        """Verify Docker detection when not available."""
        from run_arducopter_sitl import has_docker

        with patch("shutil.which", return_value=None):
            assert has_docker() is False


class TestImageManagement:
    """Tests for Docker image management."""

    def test_image_exists_true(self) -> None:
        """Verify image existence check returns True when image exists."""
        from run_arducopter_sitl import image_exists

        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            assert image_exists("test-image") is True
            mock_run.assert_called_once()

    def test_image_exists_false(self) -> None:
        """Verify image existence check returns False when missing."""
        from run_arducopter_sitl import image_exists

        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=1)
            assert image_exists("test-image") is False

    def test_build_image(self) -> None:
        """Verify image build command."""
        from run_arducopter_sitl import build_image

        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            build_image("test-image", "Copter-4.5.6")
            mock_run.assert_called_once()
            args = mock_run.call_args[0][0]
            assert "docker" in args
            assert "build" in args


class TestContainerManagement:
    """Tests for Docker container management."""

    def test_stop_container(self) -> None:
        """Verify container stop command."""
        from run_arducopter_sitl import stop_container

        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            stop_container("test-container")
            mock_run.assert_called_once()
            args = mock_run.call_args[0][0]
            assert "docker" in args
            assert "rm" in args
            assert "-f" in args

    def test_container_is_running_true(self) -> None:
        """Verify running container detection."""
        from run_arducopter_sitl import container_is_running

        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0, stdout="Up 5 seconds\n")
            assert container_is_running("test-container") is True

    def test_container_is_running_false(self) -> None:
        """Verify stopped container detection."""
        from run_arducopter_sitl import container_is_running

        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0, stdout="")
            assert container_is_running("test-container") is False


class TestSITLConfiguration:
    """Tests for SITL configuration."""

    def test_default_config(self) -> None:
        """Verify default SITL configuration."""
        from run_arducopter_sitl import SITLConfig

        config = SITLConfig()
        assert config.copter_version == "Copter-4.5.6"
        assert config.image_name == "ardupilot-sitl-4.5.6"
        assert config.container_name == "arducopter-sitl"
        assert config.port == 5760
        assert config.with_latency is False

    def test_config_with_latency(self) -> None:
        """Verify latency configuration."""
        from run_arducopter_sitl import SITLConfig

        config = SITLConfig(with_latency=True)
        assert config.with_latency is True
        assert config.latency_ms == 50  # One-way for 100ms RTT

    def test_home_location(self) -> None:
        """Verify default home location."""
        from run_arducopter_sitl import SITLConfig

        config = SITLConfig()
        assert "42.3898" in config.home_location
        assert "-71.1476" in config.home_location


class TestRunCommands:
    """Tests for SITL run commands."""

    def test_get_run_command_basic(self) -> None:
        """Verify basic run command generation."""
        from run_arducopter_sitl import SITLConfig, get_run_command

        config = SITLConfig()
        cmd = get_run_command(config)
        assert "docker" in cmd
        assert "run" in cmd
        assert "-d" in cmd
        assert "-p" in cmd
        assert "5760:5760" in cmd

    def test_get_run_command_with_latency(self) -> None:
        """Verify latency run command includes tc setup."""
        from run_arducopter_sitl import SITLConfig, get_run_command

        config = SITLConfig(with_latency=True)
        cmd = get_run_command(config)
        assert "--cap-add=NET_ADMIN" in cmd
        assert "tc" in " ".join(cmd)

    def test_get_arducopter_args(self) -> None:
        """Verify ArduCopter argument generation."""
        from run_arducopter_sitl import SITLConfig, get_arducopter_args

        config = SITLConfig()
        args = get_arducopter_args(config)
        assert "-S" in args
        assert "--model" in args
        assert "--speedup" in args
        assert "--defaults" in args
        assert "--home" in args
        assert "--serial0" in args


class TestArgumentParsing:
    """Tests for CLI argument parsing."""

    def test_default_no_latency(self) -> None:
        """Verify default is no latency."""
        from run_arducopter_sitl import parse_args

        args = parse_args([])
        assert args.with_latency is False

    def test_with_latency_flag(self) -> None:
        """Verify --with-latency flag."""
        from run_arducopter_sitl import parse_args

        args = parse_args(["--with-latency"])
        assert args.with_latency is True

    def test_stop_flag(self) -> None:
        """Verify --stop flag."""
        from run_arducopter_sitl import parse_args

        args = parse_args(["--stop"])
        assert args.stop is True

    def test_logs_flag(self) -> None:
        """Verify --logs flag."""
        from run_arducopter_sitl import parse_args

        args = parse_args(["--logs"])
        assert args.logs is True

    def test_custom_port(self) -> None:
        """Verify custom port."""
        from run_arducopter_sitl import parse_args

        args = parse_args(["--port", "5761"])
        assert args.port == 5761


class TestContainerLogs:
    """Tests for container log retrieval."""

    def test_get_logs(self) -> None:
        """Verify log retrieval command."""
        from run_arducopter_sitl import get_container_logs

        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0, stdout="log output", stderr="")
            logs = get_container_logs("test-container", lines=20)
            assert logs == "log output"
            mock_run.assert_called_once()
            args = mock_run.call_args[0][0]
            assert "docker" in args
            assert "logs" in args
