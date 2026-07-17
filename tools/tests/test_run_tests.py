#!/usr/bin/env python3
"""Contracts for locating and launching Qt test binaries."""

from __future__ import annotations

from pathlib import Path
from typing import TYPE_CHECKING

from run_tests import QtTestRunner

if TYPE_CHECKING:
    import pytest


def test_platform_detection_delegates_to_shared_helper(monkeypatch: pytest.MonkeyPatch) -> None:
    runner = QtTestRunner(Path("/tmp/build"))
    for platform, expected in (
        ("linux", "linux"),
        ("macos", "macos"),
        ("windows", "windows"),
        ("other", "linux"),
    ):
        monkeypatch.setattr("run_tests.current_platform", lambda platform=platform: platform)
        assert runner.detect_platform() == expected


def test_binary_lookup_checks_direct_and_build_type_paths(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    build = tmp_path / "build"
    build.mkdir()
    runner = QtTestRunner(build)
    monkeypatch.setattr(runner, "detect_platform", lambda: "linux")
    assert runner.find_binary() is None

    direct = build / "QGroundControl"
    direct.touch(mode=0o755)
    assert runner.find_binary() == direct
    direct.unlink()

    debug = build / "Debug" / "QGroundControl"
    debug.parent.mkdir()
    debug.touch(mode=0o755)
    assert runner.find_binary("Debug") == debug


def test_virtual_display_is_linux_only_without_display(monkeypatch: pytest.MonkeyPatch) -> None:
    assert QtTestRunner(Path("/tmp/build"), headless=True).needs_virtual_display() is False
    runner = QtTestRunner(Path("/tmp/build"))
    monkeypatch.delenv("DISPLAY", raising=False)
    monkeypatch.setattr(runner, "detect_platform", lambda: "linux")
    assert runner.needs_virtual_display() is True
    monkeypatch.setattr(runner, "detect_platform", lambda: "macos")
    assert runner.needs_virtual_display() is False
