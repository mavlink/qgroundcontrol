"""Tests for verify_coverage_thresholds.py."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

import pytest
from ci.reporting.verify_coverage_thresholds import main

if TYPE_CHECKING:
    from pathlib import Path


def _write_summary(
    path: Path,
    line: tuple[int, int, float],
    branch: tuple[int, int, float] = (100, 50, 50.0),
) -> Path:
    """Write a gcovr summary from (total, covered, percent) line and branch metrics."""
    keys = ("total", "covered", "percent")
    summary: dict[str, object] = {"gcovr/summary_format_version": "0.6", "files": []}
    summary |= {f"line_{key}": value for key, value in zip(keys, line, strict=True)}
    summary |= {f"branch_{key}": value for key, value in zip(keys, branch, strict=True)}
    path.write_text(json.dumps(summary), encoding="utf-8")
    return path


def _run(*arguments: str) -> int:
    return main(list(arguments))


def test_missing_file_returns_zero(tmp_path: Path) -> None:
    assert _run("--coverage-summary", str(tmp_path / "missing.json")) == 0


@pytest.mark.parametrize(
    "line,branch,expected",
    [
        ((100, 80, 80.0), (100, 60, 60.0), 0),
        ((100, 10, 10.0), (100, 50, 50.0), 1),
        ((100, 80, 80.0), (100, 10, 10.0), 1),
        ((100, 30, 30.0), (100, 20, 20.0), 0),
        # gcovr rounds 29.96% up to 30.0%; the threshold still compares exact counts.
        ((10000, 2996, 30.0), (100, 20, 20.0), 1),
        # A report without branches passes the branch threshold, as gcovr's own check does.
        ((100, 80, 80.0), (0, 0, 0.0), 0),
    ],
)
def test_thresholds(tmp_path: Path, line, branch, expected) -> None:
    summary = _write_summary(tmp_path / "coverage-summary.json", line, branch)
    arguments = ["--line-threshold", "30", "--branch-threshold", "20"]
    assert _run("--coverage-summary", str(summary), *arguments) == expected


def test_zero_lines_reports_coverage_output_tail(tmp_path: Path, capsys) -> None:
    summary = _write_summary(tmp_path / "coverage-summary.json", (0, 0, 0.0), (0, 0, 0.0))
    (tmp_path / "coverage-output.txt").write_text("gcovr: no coverage data\n", encoding="utf-8")
    assert _run("--coverage-summary", str(summary)) == 1
    output = capsys.readouterr().out
    assert "contains 0 lines" in output
    assert "gcovr: no coverage data" in output


@pytest.mark.parametrize("content", ["{", "[]", '{"line_percent": 50.0}'])
def test_invalid_summary_fails(tmp_path: Path, content: str) -> None:
    summary = tmp_path / "coverage-summary.json"
    summary.write_text(content, encoding="utf-8")
    assert _run("--coverage-summary", str(summary)) == 1


def _write_build_dir(path: Path, line_percent: float, branch_percent: float, cache: str) -> None:
    _write_summary(
        path / "coverage-summary.json",
        (1000, round(line_percent * 10), line_percent),
        (1000, round(branch_percent * 10), branch_percent),
    )
    (path / "CMakeCache.txt").write_text(cache, encoding="utf-8")


def test_build_dir_reads_thresholds_from_cmake_cache(tmp_path: Path, capsys) -> None:
    _write_build_dir(
        tmp_path,
        50.0,
        30.0,
        "QGC_COVERAGE_LINE_THRESHOLD:STRING=60\nQGC_COVERAGE_BRANCH_THRESHOLD:STRING=25\n",
    )
    assert _run("--build-dir", str(tmp_path)) == 1
    output = capsys.readouterr().out
    assert "Line coverage: 50.0% (threshold: 60.0%)" in output
    assert "threshold: 25.0%" in output


def test_build_dir_falls_back_to_default_thresholds(tmp_path: Path, capsys) -> None:
    _write_build_dir(tmp_path, 31.0, 21.0, "CMAKE_BUILD_TYPE:STRING=Debug\n")
    assert _run("--build-dir", str(tmp_path)) == 0
    output = capsys.readouterr().out
    assert "threshold: 30.0%" in output
    assert "threshold: 20.0%" in output


def test_explicit_thresholds_override_cmake_cache(tmp_path: Path) -> None:
    _write_build_dir(
        tmp_path,
        50.0,
        30.0,
        "QGC_COVERAGE_LINE_THRESHOLD:STRING=60\nQGC_COVERAGE_BRANCH_THRESHOLD:STRING=25\n",
    )
    arguments = ["--line-threshold", "40", "--branch-threshold", "20"]
    assert _run("--build-dir", str(tmp_path), *arguments) == 0
