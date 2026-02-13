"""Tests for run_tests.py test runner."""

from __future__ import annotations

import os
import sys
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from run_tests import (
    TestResult,
    QtTestRunner,
    parse_args,
)


class TestRunTestsResult:
    """Tests for TestResult dataclass."""

    def test_default_values(self):
        """Test default TestResult values."""
        result = TestResult()
        assert result.passed == 0
        assert result.failed == 0
        assert result.skipped == 0
        assert result.duration == 0.0
        assert result.output == ""
        assert result.xml_path is None
        assert result.log_path is None
        assert result.exit_code == 0

    def test_with_values(self):
        """Test TestResult with values."""
        result = TestResult(
            passed=10,
            failed=2,
            skipped=3,
            duration=45.5,
            exit_code=1,
        )
        assert result.passed == 10
        assert result.failed == 2
        assert result.duration == 45.5


class TestParseArgs:
    """Tests for parse_args function."""

    def test_default_args(self):
        """Test default arguments."""
        args = parse_args([])
        assert args.build_dir == Path("build")
        assert args.timeout == 300
        assert args.verbose is False
        assert args.headless is False

    def test_build_dir(self):
        """Test --build-dir argument."""
        args = parse_args(["--build-dir", "build-release"])
        assert args.build_dir == Path("build-release")

    def test_timeout(self):
        """Test --timeout argument."""
        args = parse_args(["--timeout", "600"])
        assert args.timeout == 600

    def test_verbose(self):
        """Test --verbose flag."""
        args = parse_args(["--verbose"])
        assert args.verbose is True

    def test_headless(self):
        """Test --headless flag."""
        args = parse_args(["--headless"])
        assert args.headless is True


class TestQtTestRunner:
    """Tests for QtTestRunner class."""

    def test_init(self, tmp_path):
        """Test QtTestRunner initialization."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()

        runner = QtTestRunner(build_dir)

        assert runner.build_dir == build_dir.resolve()
        assert runner.timeout == 300
        assert runner.verbose is False

    def test_init_with_options(self, tmp_path):
        """Test QtTestRunner with options."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()

        runner = QtTestRunner(
            build_dir,
            timeout=600,
            verbose=True,
            headless=True,
        )

        assert runner.timeout == 600
        assert runner.verbose is True
        assert runner.headless is True


class TestQtTestRunnerDetectPlatform:
    """Tests for QtTestRunner.detect_platform method."""

    def test_detect_linux(self, tmp_path):
        """Test detecting Linux platform."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()

        runner = QtTestRunner(build_dir)

        with patch("platform.system", return_value="Linux"):
            assert runner.detect_platform() == "linux"

    def test_detect_macos(self, tmp_path):
        """Test detecting macOS platform."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()

        runner = QtTestRunner(build_dir)

        with patch("platform.system", return_value="Darwin"):
            assert runner.detect_platform() == "macos"

    def test_detect_windows(self, tmp_path):
        """Test detecting Windows platform."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()

        runner = QtTestRunner(build_dir)

        with patch("platform.system", return_value="Windows"):
            assert runner.detect_platform() == "windows"


class TestQtTestRunnerNeedsVirtualDisplay:
    """Tests for QtTestRunner.needs_virtual_display method."""

    def test_headless_no_display_needed(self, tmp_path):
        """Test headless mode doesn't need virtual display."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()

        runner = QtTestRunner(build_dir, headless=True)
        assert runner.needs_virtual_display() is False

    def test_linux_no_display_needs_virtual(self, tmp_path):
        """Test Linux without DISPLAY needs virtual display."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()

        runner = QtTestRunner(build_dir, headless=False)

        with patch.object(runner, "detect_platform", return_value="linux"):
            with patch.dict(os.environ, {}, clear=True):
                # Remove DISPLAY from environment
                os.environ.pop("DISPLAY", None)
                assert runner.needs_virtual_display() is True

    def test_linux_with_display(self, tmp_path):
        """Test Linux with DISPLAY doesn't need virtual display."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()

        runner = QtTestRunner(build_dir, headless=False)

        with patch.object(runner, "detect_platform", return_value="linux"):
            with patch.dict(os.environ, {"DISPLAY": ":0"}):
                assert runner.needs_virtual_display() is False


class TestQtTestRunnerFindBinary:
    """Tests for QtTestRunner.find_binary method."""

    def test_find_binary_direct_path(self, tmp_path):
        """Test finding binary in direct path."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        binary = build_dir / "QGroundControl"
        binary.write_text("#!/bin/bash")
        binary.chmod(0o755)

        runner = QtTestRunner(build_dir)

        with patch.object(runner, "detect_platform", return_value="linux"):
            found = runner.find_binary()

        assert found == binary

    def test_find_binary_in_build_type(self, tmp_path):
        """Test finding binary in build type subdirectory."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        debug_dir = build_dir / "Debug"
        debug_dir.mkdir()
        binary = debug_dir / "QGroundControl"
        binary.write_text("#!/bin/bash")
        binary.chmod(0o755)

        runner = QtTestRunner(build_dir)

        with patch.object(runner, "detect_platform", return_value="linux"):
            found = runner.find_binary()

        assert found == binary

    def test_find_binary_not_found(self, tmp_path):
        """Test when binary not found."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()

        runner = QtTestRunner(build_dir)

        with patch.object(runner, "detect_platform", return_value="linux"):
            found = runner.find_binary()

        assert found is None


class TestQtTestRunnerRunTests:
    """Tests for QtTestRunner.run_tests method."""

    def test_run_tests_no_binary(self, tmp_path):
        """Test run_tests when binary not found."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()

        runner = QtTestRunner(build_dir)

        with patch.object(runner, "find_binary", return_value=None):
            result = runner.run_tests()

        assert result.exit_code != 0

    def test_run_tests_success(self, tmp_path):
        """Test successful test run."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        binary = build_dir / "QGroundControl"
        binary.write_text("#!/bin/bash")
        binary.chmod(0o755)

        runner = QtTestRunner(build_dir)

        with patch.object(runner, "find_binary", return_value=binary):
            with patch("subprocess.run") as mock_run:
                mock_run.return_value = MagicMock(
                    returncode=0,
                    stdout="Totals: 100 passed, 0 failed, 0 skipped",
                    stderr="",
                )
                result = runner.run_tests()

        assert result.exit_code == 0


class TestQtTestRunnerBuildCommand:
    """Tests for QtTestRunner._build_command method."""

    def test_build_command_basic(self, tmp_path):
        """Test building basic command."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        binary = build_dir / "QGroundControl"
        binary.write_text("#!/bin/bash")
        binary.chmod(0o755)

        runner = QtTestRunner(build_dir)
        cmd = runner._build_command(binary, [])

        assert str(binary) in cmd

    def test_build_command_with_args(self, tmp_path):
        """Test building command with test arguments."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        binary = build_dir / "QGroundControl"
        binary.write_text("#!/bin/bash")
        binary.chmod(0o755)

        runner = QtTestRunner(build_dir)
        cmd = runner._build_command(binary, ["--unittest", "-v"])

        assert "--unittest" in cmd
        assert "-v" in cmd


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
