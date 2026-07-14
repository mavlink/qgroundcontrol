#!/usr/bin/env python3
"""Tests for mold_helper.py."""

from __future__ import annotations

from unittest.mock import patch

import pytest
from mold_helper import detect_arch, is_installed


def test_detect_arch_normalizes_supported_values_and_rejects_unknown(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.delenv("RUNNER_ARCH", raising=False)
    for machine, expected in (("x86_64", "x86_64"), ("amd64", "x86_64"), ("arm64", "aarch64")):
        with patch("platform.machine", return_value=machine):
            assert detect_arch() == expected
    with patch("platform.machine", return_value="riscv64"), pytest.raises(ValueError):
        detect_arch()


def test_is_installed_uses_shared_prefix_comparison() -> None:
    with patch("mold_helper.probe_version", return_value=(2, 41)):
        assert is_installed("2.41.0") is True
