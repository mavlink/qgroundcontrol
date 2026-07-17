#!/usr/bin/env python3
"""Contracts for shared byte formatters."""

from common.format import format_bytes, format_delta_bytes


def test_byte_formatting_boundaries() -> None:
    cases = [
        (512, "0.00 MB"),
        (1024**2, "1.00 MB"),
        (1023 * 1024**2, "1023.00 MB"),
        (1024**3, "1.00 GB"),
        (5 * 1024**3, "5.00 GB"),
        (1.5 * 1024**2, "1.50 MB"),
    ]
    for size, expected in cases:
        assert format_bytes(size) == expected


def test_byte_delta_formatting() -> None:
    cases = [
        (2 * 1024**2, "+2.00 MB (increase)"),
        (-3 * 1024**2, "-3.00 MB (decrease)"),
        (0, "No change"),
        (1, "+0.00 MB (increase)"),
    ]
    for delta, expected in cases:
        assert format_delta_bytes(delta) == expected
