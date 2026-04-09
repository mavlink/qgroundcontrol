#!/usr/bin/env python3
"""Tests for tools/run_tests.py."""

from __future__ import annotations

from pathlib import Path
from unittest.mock import patch

from run_tests import QtTestRunner


class TestQtTestRunner:
    def test_detect_platform_linux(self) -> None:
        runner = QtTestRunner(Path("/tmp/build"))
        with patch("run_tests.platform.system", return_value="Linux"):
            assert runner.detect_platform() == "linux"

    def test_detect_platform_darwin(self) -> None:
        runner = QtTestRunner(Path("/tmp/build"))
        with patch("run_tests.platform.system", return_value="Darwin"):
            assert runner.detect_platform() == "macos"

    def test_detect_platform_windows(self) -> None:
        runner = QtTestRunner(Path("/tmp/build"))
        with patch("run_tests.platform.system", return_value="Windows"):
            assert runner.detect_platform() == "windows"

    def test_find_binary_direct(self, tmp_path: Path) -> None:
        build = tmp_path / "build"
        build.mkdir()
        binary = build / "QGroundControl"
        binary.touch(mode=0o755)
        runner = QtTestRunner(build)
        with patch.object(runner, "detect_platform", return_value="linux"), \
             patch.object(runner, "_run_find_script", return_value=None):
            result = runner.find_binary()
        assert result is not None
        assert result.name == "QGroundControl"

    def test_find_binary_in_build_type_dir(self, tmp_path: Path) -> None:
        build = tmp_path / "build"
        debug_dir = build / "Debug"
        debug_dir.mkdir(parents=True)
        binary = debug_dir / "QGroundControl"
        binary.touch(mode=0o755)
        runner = QtTestRunner(build)
        with patch.object(runner, "detect_platform", return_value="linux"), \
             patch.object(runner, "_run_find_script", return_value=None):
            result = runner.find_binary("Debug")
        assert result is not None
        assert "Debug" in str(result)

    def test_find_binary_not_found(self, tmp_path: Path) -> None:
        build = tmp_path / "build"
        build.mkdir()
        runner = QtTestRunner(build)
        with patch.object(runner, "detect_platform", return_value="linux"), \
             patch.object(runner, "_run_find_script", return_value=None):
            result = runner.find_binary()
        assert result is None

    def test_needs_virtual_display_headless(self) -> None:
        runner = QtTestRunner(Path("/tmp/build"), headless=True)
        assert runner.needs_virtual_display() is False

    def test_needs_virtual_display_linux_no_display(self) -> None:
        runner = QtTestRunner(Path("/tmp/build"))
        with patch.object(runner, "detect_platform", return_value="linux"), \
             patch.dict("os.environ", {}, clear=True):
            assert runner.needs_virtual_display() is True

    def test_needs_virtual_display_macos(self) -> None:
        runner = QtTestRunner(Path("/tmp/build"))
        with patch.object(runner, "detect_platform", return_value="macos"):
            assert runner.needs_virtual_display() is False
