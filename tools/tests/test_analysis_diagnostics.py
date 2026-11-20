"""Tests for tools/common/analysis_diagnostics.py."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

import pytest
from common.analysis_diagnostics import (
    AnalysisWarning,
    is_bug_class,
    load_changed_findings,
    load_unique_warnings,
)

if TYPE_CHECKING:
    from pathlib import Path


def _write(path: Path, tool: str, diagnostics: dict, changed: dict | None = None) -> None:
    report: dict = {"tool": tool, "shard": 1, "diagnostics": diagnostics}
    if changed is not None:
        report["changed"] = changed
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report), encoding="utf-8")


def test_warnings_are_unique_across_shards_and_skip_errors(tmp_path: Path) -> None:
    header = [10, 5, "warning", "modernize-use-nodiscard"]
    _write(
        tmp_path / "shard-1/clang-tidy-diagnostics.json",
        "clang-tidy",
        {"src/Vehicle/Vehicle.h": [header, [12, 1, "warning", "readability-identifier-naming"]]},
        {"src/Vehicle/Vehicle.h": [header]},
    )
    _write(
        tmp_path / "shard-2/clang-tidy-diagnostics.json",
        "clang-tidy",
        {
            "src/Vehicle/Vehicle.h": [header],
            "src/Vehicle/Vehicle.cc": [[9, 2, "error", "clang-diagnostic-error"], ["malformed"]],
        },
    )
    _write(tmp_path / "shard-1/clazy-diagnostics.json", "clazy", {})

    warnings, changed = load_unique_warnings(tmp_path)

    assert {tool: len(found) for tool, found in warnings.items()} == {"clang-tidy": 2, "clazy": 0}
    assert changed == {
        "clang-tidy": {
            AnalysisWarning("clang-tidy", "src/Vehicle/Vehicle.h", 10, 5, "modernize-use-nodiscard")
        }
    }


def test_unreadable_reports_are_logged_and_skipped(
    tmp_path: Path, caplog: pytest.LogCaptureFixture
) -> None:
    (tmp_path / "clazy-diagnostics.json").write_text("{", encoding="utf-8")
    _write(tmp_path / "other-diagnostics.json", "", {"a.cc": [[1, 1, "warning", "check"]]})
    (tmp_path / "list-diagnostics.json").write_text("[]", encoding="utf-8")

    warnings, changed = load_unique_warnings(tmp_path)

    # A report without a tool name is named after its file.
    assert {tool: len(found) for tool, found in warnings.items()} == {"other": 1}
    assert changed == {}
    assert "Failed to parse analysis diagnostics" in caplog.text


def test_changed_findings_skip_malformed_rows_and_keep_exact_paths(tmp_path: Path) -> None:
    # Reports come from pull request code; GitHub's file names must match exactly.
    long_path = "src/" + "deeply/nested  dir/" * 8 + "Long File.cc"
    _write(
        tmp_path / "clang-tidy-diagnostics.json",
        "clang-tidy",
        {},
        {long_path: [[1, 1, [], "check"], [2, 3, "error", "check", "message", "  2 | code"]]},
    )

    (finding,) = load_changed_findings(tmp_path)

    assert (finding.file, finding.line, finding.severity) == (long_path, 2, "error")


@pytest.mark.parametrize(
    ("tool", "severity", "check", "bug_class"),
    [
        ("clang-tidy", "warning", "bugprone-narrowing-conversions", True),
        ("clang-tidy", "warning", "clang-analyzer-core.CallAndMessage", True),
        # clang-tidy joins a check's enabled aliases.
        ("clang-tidy", "warning", "cert-err09-cpp,bugprone-throw-by-value", True),
        ("clang-tidy", "warning", "performance-unnecessary-value-param", False),
        ("clang-tidy", "warning", "modernize-use-nodiscard", False),
        ("clang-tidy", "warning", "not-bugprone-x", False),
        ("clazy", "warning", "-Wclazy-range-loop-detach", False),
        ("clazy", "warning", "bugprone-named-by-another-tool", False),
        ("clang-tidy", "error", "clang-diagnostic-error", True),
        ("clazy", "error", "-Wclazy-old-style-connect", True),
    ],
)
def test_bug_class_is_any_error_or_a_clang_tidy_bug_or_analyzer_warning(
    tool: str, severity: str, check: str, bug_class: bool
) -> None:
    finding = AnalysisWarning(tool, "src/a.cc", 1, 1, check, "message", severity)
    assert is_bug_class(finding) is bug_class
