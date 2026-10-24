"""Tests for tools/common/format.py."""

from __future__ import annotations

import pytest
from common.format import (
    clip,
    format_bytes,
    format_count,
    format_delta_bytes,
    format_duration,
    format_percent,
    format_ratio,
)


def test_format_bytes_under_one_mb() -> None:
    assert format_bytes(512) == "0.00 MB"


def test_format_bytes_one_mb() -> None:
    assert format_bytes(1024 * 1024) == "1.00 MB"


def test_format_bytes_just_under_gb_boundary() -> None:
    assert format_bytes(1023 * 1024 * 1024) == "1023.00 MB"


def test_format_bytes_one_gb() -> None:
    assert format_bytes(1024 * 1024 * 1024) == "1.00 GB"


def test_format_bytes_multi_gb() -> None:
    assert format_bytes(5 * 1024 * 1024 * 1024) == "5.00 GB"


def test_format_bytes_accepts_float() -> None:
    assert format_bytes(1024 * 1024 * 1.5) == "1.50 MB"


def test_format_delta_bytes_positive() -> None:
    assert format_delta_bytes(2 * 1024 * 1024) == "+2.00 MB"


def test_format_delta_bytes_negative() -> None:
    assert format_delta_bytes(-3 * 1024 * 1024) == "-3.00 MB"


def test_format_delta_bytes_zero() -> None:
    assert format_delta_bytes(0) == "unchanged"


def test_format_delta_bytes_small_positive() -> None:
    assert format_delta_bytes(1) == "+0.00 MB"


def test_format_count_groups_thousands() -> None:
    assert format_count(1234567) == "1,234,567"


@pytest.mark.parametrize(
    ("seconds", "expected"),
    [
        (None, "n/a"),
        (-1, "n/a"),
        (0.42, "0.4s"),
        (9.94, "9.9s"),
        (42.4, "42s"),
        (59.6, "1m 00s"),
        (185, "3m 05s"),
        (3725, "1h 02m"),
    ],
)
def test_format_duration(seconds: float | None, expected: str) -> None:
    assert format_duration(seconds) == expected


def test_format_percent_and_ratio() -> None:
    assert format_percent(93.456) == "93.5%"
    assert format_percent(5, digits=2) == "5.00%"
    assert format_ratio(1234, 1300) == "1,234 of 1,300 (94.9%)"
    assert format_ratio(0, 0) == "0 of 0"


def test_clip_joins_lines_and_shortens() -> None:
    assert clip("a\n  b\tc") == "a b c"
    assert clip("x" * 120) == "x" * 120
    assert clip("x" * 121) == "x" * 119 + "…"
    assert clip(12345, limit=3) == "12…"
