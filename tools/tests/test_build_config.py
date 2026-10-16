"""Tests for shared build-configuration primitives."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

import pytest
from common.build_config import get_gstreamer_version, lookup_dotted

if TYPE_CHECKING:
    from pathlib import Path


def test_lookup_dotted_distinguishes_null_missing_and_values() -> None:
    config = {"qt": {"version": 611, "minimum_version": None}}

    assert lookup_dotted(config, "qt.version") == 611
    assert lookup_dotted(config, "qt.minimum_version", "fallback") is None
    assert lookup_dotted(config, "qt.missing", "fallback") == "fallback"
    with pytest.raises(KeyError, match=r"qt\.missing"):
        lookup_dotted(config, "qt.missing")


@pytest.mark.parametrize(
    "platform,expected",
    [("android", "1.28.2"), ("linux", "1.28.1"), ("windows", "1.28.1")],
)
def test_gstreamer_version_falls_back_to_default(
    tmp_path: Path, platform: str, expected: str
) -> None:
    config = tmp_path / "build-config.json"
    config.write_text(
        json.dumps({"gstreamer": {"version": {"default": "1.28.1", "android": "1.28.2"}}})
    )

    assert get_gstreamer_version(platform, config_file=config) == expected
