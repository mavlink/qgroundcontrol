#!/usr/bin/env python3
"""Tests for tools/coverage.py."""

from __future__ import annotations

from coverage import build_step_summary


def test_build_step_summary_includes_metrics() -> None:
    markdown = build_step_summary("lines: 42.0% (42 out of 100)\nbranches: 10.0% (1 out of 10)\n", "report-only")
    assert "| Lines | 42.0% (42 out of 100) |" in markdown
    assert "| Branches | 10.0% (1 out of 10) |" in markdown
