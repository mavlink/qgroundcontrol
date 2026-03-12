#!/usr/bin/env python3
"""Tests for resolve_gstreamer_config.py."""

from __future__ import annotations

from unittest.mock import patch

from resolve_gstreamer_config import resolve_version


def test_resolve_version_prefers_explicit_override() -> None:
    assert resolve_version("linux", "1.2.3") == "1.2.3"


def test_resolve_version_uses_platform_specific_key() -> None:
    with patch("resolve_gstreamer_config.get_build_config_value", return_value="9.9.9") as mock_get:
        assert resolve_version("windows", "") == "9.9.9"
    mock_get.assert_called_once()
