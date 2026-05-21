#!/usr/bin/env python3
"""Tests for tools/common/format.py."""

from __future__ import annotations

from common.format import format_bytes, format_delta_bytes


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
    assert format_delta_bytes(2 * 1024 * 1024) == "+2.00 MB (increase)"


def test_format_delta_bytes_negative() -> None:
    assert format_delta_bytes(-3 * 1024 * 1024) == "-3.00 MB (decrease)"


def test_format_delta_bytes_zero() -> None:
    assert format_delta_bytes(0) == "No change"


def test_format_delta_bytes_small_positive() -> None:
    assert format_delta_bytes(1) == "+0.00 MB (increase)"
