#!/usr/bin/env python3
"""Tests for tools/common/platform.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

from common.platform import current_platform, is_linux, is_macos, is_windows

if TYPE_CHECKING:
    import pytest


def test_is_windows(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.platform", "win32")
    assert is_windows() is True
    assert is_macos() is False
    assert is_linux() is False


def test_is_macos(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.platform", "darwin")
    assert is_macos() is True
    assert is_windows() is False
    assert is_linux() is False


def test_is_linux(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.platform", "linux")
    assert is_linux() is True
    assert is_windows() is False
    assert is_macos() is False


def test_is_linux_with_version_suffix(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.platform", "linux2")
    assert is_linux() is True


def test_current_platform_windows(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.platform", "win32")
    assert current_platform() == "windows"


def test_current_platform_macos(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.platform", "darwin")
    assert current_platform() == "macos"


def test_current_platform_linux(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.platform", "linux")
    assert current_platform() == "linux"


def test_current_platform_other(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.platform", "freebsd14")
    assert current_platform() == "other"
