from __future__ import annotations

from typing import TYPE_CHECKING

from test_duration_report import analyze_test_durations, main

if TYPE_CHECKING:
    from pathlib import Path


def test_analyze_test_durations_reports_slow_tests(tmp_path):
    junit = tmp_path / "junit.xml"
    junit.write_text(
        """<testsuite>
        <testcase classname="Suite" name="fast" time="1.0" />
        <testcase classname="Suite" name="slow" time="10.0" />
        </testsuite>""",
        encoding="utf-8",
    )

    report = analyze_test_durations(junit, top_n=5, slow_threshold=5.0)

    assert report["slow_count"] == 1
    assert report["total_tests"] == 2
    assert len(report["top_slowest"]) == 2
    assert report["top_slowest"][0]["test"] == "Suite::slow"


def test_test_duration_report_main_missing_junit(tmp_path, monkeypatch, gh_output: Path):
    summary = tmp_path / "summary.md"
    monkeypatch.setenv("GITHUB_STEP_SUMMARY", str(summary))
    result = main(["--junit-path", str(tmp_path / "missing.xml")])
    assert result == 0
    assert "slow_count=0" in gh_output.read_text(encoding="utf-8")
    assert "WARNING" in summary.read_text(encoding="utf-8")
