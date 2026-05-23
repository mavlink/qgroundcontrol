#!/usr/bin/env python3
"""Tests for resolve_gstreamer_config.py."""

from __future__ import annotations

from resolve_gstreamer_config import resolve_version


def test_resolve_version_prefers_explicit_override() -> None:
    assert resolve_version("1.2.3", "9.9.9", "0.0.0") == "1.2.3"


def test_resolve_version_uses_platform_default_when_no_override() -> None:
    assert resolve_version("", "9.9.9", "0.0.0") == "9.9.9"


def test_resolve_version_falls_back_to_generic_when_no_platform_default() -> None:
    assert resolve_version("", "", "0.0.0") == "0.0.0"
