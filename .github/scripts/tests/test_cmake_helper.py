#!/usr/bin/env python3
"""Tests for cmake_helper.py."""

from __future__ import annotations

from typing import TYPE_CHECKING
from unittest.mock import patch

import pytest
from cmake_helper import detect_jobs, main, read_cache_var

if TYPE_CHECKING:
    from pathlib import Path


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
        with pytest.raises(SystemExit):
            detect_jobs("abc")

    def test_zero_exits(self) -> None:
        with pytest.raises(SystemExit):
            detect_jobs("0")

    def test_negative_exits(self) -> None:
        with pytest.raises(SystemExit):
            detect_jobs("-1")


_SAMPLE_CACHE = """\
# This is the CMakeCache file.
//Build type
CMAKE_BUILD_TYPE:STRING=Release
QGC_COVERAGE_LINE_THRESHOLD:STRING=42
QGC_COVERAGE_BRANCH_THRESHOLD:STRING=23
QGC_ENABLE_GST:BOOL=ON
//Comment
"""


def _write_cache(tmp_path: Path) -> Path:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text(_SAMPLE_CACHE)
    return cache


class TestReadCacheVar:
    def test_returns_string_value(self, tmp_path: Path) -> None:
        cache = _write_cache(tmp_path)
        assert read_cache_var(str(cache), "QGC_COVERAGE_LINE_THRESHOLD") == "42"

    def test_returns_bool_value(self, tmp_path: Path) -> None:
        cache = _write_cache(tmp_path)
        assert read_cache_var(str(cache), "QGC_ENABLE_GST") == "ON"

    def test_missing_var_returns_none(self, tmp_path: Path) -> None:
        cache = _write_cache(tmp_path)
        assert read_cache_var(str(cache), "ABSENT_VAR") is None

    def test_missing_file_returns_none(self, tmp_path: Path) -> None:
        assert read_cache_var(str(tmp_path / "nope.txt"), "X") is None

    def test_partial_name_does_not_match(self, tmp_path: Path) -> None:
        cache = _write_cache(tmp_path)
        assert read_cache_var(str(cache), "QGC_COVERAGE_LINE") is None


class TestCmdCacheVar:
    def test_main_prints_and_writes_output(
        self, tmp_path: Path, monkeypatch, capsys, gh_output: Path
    ) -> None:
        _write_cache(tmp_path)
        monkeypatch.setattr(
            "sys.argv",
            ["prog", "cache-var", "--build-dir", str(tmp_path), "--name", "CMAKE_BUILD_TYPE"],
        )
        main()
        assert capsys.readouterr().out.strip() == "Release"
        assert "cmake_build_type=Release" in gh_output.read_text()

    def test_main_uses_default_when_missing(
        self, tmp_path: Path, monkeypatch, capsys, gh_output: Path
    ) -> None:
        _write_cache(tmp_path)
        monkeypatch.setattr(
            "sys.argv",
            [
                "prog",
                "cache-var",
                "--build-dir",
                str(tmp_path),
                "--name",
                "ABSENT",
                "--default",
                "fallback",
                "--output-key",
                "value",
            ],
        )
        main()
        assert capsys.readouterr().out.strip() == "fallback"
        assert "value=fallback" in gh_output.read_text()

    def test_main_required_missing_exits(self, tmp_path: Path, monkeypatch) -> None:
        _write_cache(tmp_path)
        monkeypatch.setattr(
            "sys.argv",
            ["prog", "cache-var", "--build-dir", str(tmp_path), "--name", "ABSENT", "--required"],
        )
        with pytest.raises(SystemExit) as exc:
            main()
        assert exc.value.code == 1
