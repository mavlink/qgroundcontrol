#!/usr/bin/env python3
"""Tests for cmake_helper.py."""

from __future__ import annotations

import os
from unittest.mock import patch

from cmake_helper import detect_jobs


class TestDetectJobs:
    def test_explicit_value(self) -> None:
        assert detect_jobs("4") == 4

    def test_explicit_value_large(self) -> None:
        assert detect_jobs("16") == 16

    def test_auto_uses_cpu_count(self) -> None:
        with patch("os.cpu_count", return_value=8):
            assert detect_jobs("auto") == 8

    def test_auto_fallback_none(self) -> None:
        with patch("os.cpu_count", return_value=None):
            assert detect_jobs("auto") == 2

    def test_invalid_exits(self) -> None:
        import pytest
        with pytest.raises(SystemExit):
            detect_jobs("abc")

    def test_zero_exits(self) -> None:
        import pytest
        with pytest.raises(SystemExit):
            detect_jobs("0")

    def test_negative_exits(self) -> None:
        import pytest
        with pytest.raises(SystemExit):
            detect_jobs("-1")
