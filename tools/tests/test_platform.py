#!/usr/bin/env python3
"""Tests for tools/common/platform.py."""

from __future__ import annotations

import pytest
from common.platform import (
    current_platform,
    host_arch,
    is_linux,
    is_macos,
    is_windows,
    normalize_arch,
)


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


def test_architecture_normalization() -> None:
    expected = {
        "x86_64": "x86_64",
        "amd64": "x86_64",
        "X64": "x86_64",
        "aarch64": "aarch64",
        "arm64": "aarch64",
        "ARM64": "aarch64",
    }
    for value, architecture in expected.items():
        assert normalize_arch(value) == architecture
    with pytest.raises(ValueError, match="Unsupported architecture"):
        normalize_arch("riscv64")


def test_host_arch_prefers_runner_then_machine(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("RUNNER_ARCH", "ARM64")
    monkeypatch.setattr("platform.machine", lambda: "x86_64")
    assert host_arch() == "aarch64"
    monkeypatch.delenv("RUNNER_ARCH")
    monkeypatch.setattr("platform.machine", lambda: "AMD64")
    assert host_arch() == "x86_64"
