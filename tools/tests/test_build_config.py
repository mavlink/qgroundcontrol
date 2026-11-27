"""Tests for shared build-configuration primitives."""

from __future__ import annotations

import pytest
from common.build_config import lookup_dotted


def test_lookup_dotted_distinguishes_null_missing_and_values() -> None:
    config = {"qt": {"version": 611, "minimum_version": None}}

    assert lookup_dotted(config, "qt.version") == 611
    assert lookup_dotted(config, "qt.minimum_version", "fallback") is None
    assert lookup_dotted(config, "qt.missing", "fallback") == "fallback"
    with pytest.raises(KeyError, match=r"qt\.missing"):
        lookup_dotted(config, "qt.missing")
