"""Tests for shared gcovr JSON summary parsing."""

from __future__ import annotations

import json

import pytest
from common.coverage_summary import CoverageSummary, CoverageSummaryError, read_gcovr_summary

# Project totals as gcovr 8.6 `--json-summary` writes them (per-file entries omitted).
GCOVR_SUMMARY = {
    "root": ".",
    "gcovr/summary_format_version": "0.6",
    "files": [],
    "line_total": 10,
    "line_covered": 6,
    "line_percent": 60.0,
    "function_total": 3,
    "function_covered": 2,
    "function_percent": 66.7,
    "branch_total": 4,
    "branch_covered": 1,
    "branch_percent": 25.0,
}


def test_reads_project_totals(tmp_path) -> None:
    report = tmp_path / "coverage-summary.json"
    report.write_text(json.dumps(GCOVR_SUMMARY), encoding="utf-8")
    assert read_gcovr_summary(report) == CoverageSummary(60.0, 10, 6, 25.0, 4, 1, 66.7, 3, 2)


def test_function_counts_are_optional(tmp_path) -> None:
    report = tmp_path / "coverage-summary.json"
    without = {k: v for k, v in GCOVR_SUMMARY.items() if not k.startswith("function_")}
    report.write_text(json.dumps(without), encoding="utf-8")
    assert read_gcovr_summary(report) == CoverageSummary(60.0, 10, 6, 25.0, 4, 1)


def test_integer_percentages_are_floats(tmp_path) -> None:
    report = tmp_path / "coverage-summary.json"
    report.write_text(json.dumps({**GCOVR_SUMMARY, "line_percent": 100}), encoding="utf-8")
    assert read_gcovr_summary(report).line_percent == 100.0


@pytest.mark.parametrize(
    "content",
    [
        "{",
        "[]",
        json.dumps({key: value for key, value in GCOVR_SUMMARY.items() if key != "branch_total"}),
        json.dumps({**GCOVR_SUMMARY, "line_total": "10"}),
        json.dumps({**GCOVR_SUMMARY, "line_total": True}),
        json.dumps({**GCOVR_SUMMARY, "line_covered": -1}),
        json.dumps({**GCOVR_SUMMARY, "line_covered": 11}),
        json.dumps({**GCOVR_SUMMARY, "function_covered": 4}),
        json.dumps({**GCOVR_SUMMARY, "branch_percent": 100.1}),
        json.dumps({**GCOVR_SUMMARY, "line_percent": None}),
        '{"line_percent": NaN}',
    ],
)
def test_rejects_invalid_summaries(tmp_path, content: str) -> None:
    report = tmp_path / "coverage-summary.json"
    report.write_text(content, encoding="utf-8")
    with pytest.raises(CoverageSummaryError):
        read_gcovr_summary(report)


def test_missing_file_is_a_summary_error(tmp_path) -> None:
    with pytest.raises(CoverageSummaryError, match="failed to parse"):
        read_gcovr_summary(tmp_path / "missing.json")


def _file(filename: str, lines: tuple[int, int], branches: tuple[int, int]) -> dict[str, object]:
    return {
        "filename": filename,
        "line_total": lines[1],
        "line_covered": lines[0],
        "line_percent": 0.0,
        "function_total": 1,
        "function_covered": 1,
        "function_percent": 100.0,
        "branch_total": branches[1],
        "branch_covered": branches[0],
        "branch_percent": 0.0,
    }


def test_path_prefix_sums_only_the_files_under_it(tmp_path) -> None:
    files = [
        _file("src/Vehicle/Vehicle.cc", (2, 3), (1, 4)),
        _file("src/Comms/LinkManager.cc", (1, 1), (0, 0)),
        # Test code is nearly always covered and would inflate the application's percentages.
        _file("test/Vehicle/VehicleTest.cc", (50, 50), (5, 5)),
    ]
    report = tmp_path / "coverage-summary.json"
    report.write_text(json.dumps({**GCOVR_SUMMARY, "files": files}), encoding="utf-8")
    assert read_gcovr_summary(report, path_prefix="src/") == CoverageSummary(
        75.0, 4, 3, 25.0, 4, 1, 100.0, 2, 2
    )


def test_path_prefix_rounds_like_gcovr(tmp_path) -> None:
    report = tmp_path / "coverage-summary.json"
    files = [_file("src/a.cc", (999, 1000), (0, 0))]
    report.write_text(json.dumps({**GCOVR_SUMMARY, "files": files}), encoding="utf-8")
    summary = read_gcovr_summary(report, path_prefix="src/")
    # 99.9% rounds to 100.0, which gcovr reserves for full coverage.
    assert (summary.line_percent, summary.branch_percent) == (99.9, 0.0)


@pytest.mark.parametrize(
    "files",
    [None, [_file("src/a.cc", (-1, 2), (0, 0))], [_file("src/a.cc", (3, 2), (0, 0))]],
)
def test_path_prefix_rejects_invalid_files(tmp_path, files) -> None:
    report = tmp_path / "coverage-summary.json"
    report.write_text(json.dumps({**GCOVR_SUMMARY, "files": files}), encoding="utf-8")
    with pytest.raises(CoverageSummaryError):
        read_gcovr_summary(report, path_prefix="src/")
