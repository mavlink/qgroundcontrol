#!/usr/bin/env python3
"""Contracts for platform and architecture normalization."""

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


def test_platform_detection(monkeypatch: pytest.MonkeyPatch) -> None:
    cases = [
        ("win32", "windows", (True, False, False)),
        ("darwin", "macos", (False, True, False)),
        ("linux", "linux", (False, False, True)),
        ("linux2", "linux", (False, False, True)),
        ("freebsd14", "other", (False, False, False)),
    ]
    for platform, expected, flags in cases:
        monkeypatch.setattr("sys.platform", platform)
        assert current_platform() == expected
        assert (is_windows(), is_macos(), is_linux()) == flags


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
