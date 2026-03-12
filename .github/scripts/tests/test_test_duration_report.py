from __future__ import annotations

import json

from test_duration_report import analyze_test_durations, main


def _write_junit(path, xml: str) -> None:
    path.write_text(xml, encoding="utf-8")


def test_analyze_test_durations_detects_regression(tmp_path):
    junit = tmp_path / "junit.xml"
    baseline = tmp_path / "baseline.json"
    _write_junit(
        junit,
        """<testsuite>
        <testcase classname="Suite" name="fast" time="1.0" />
        <testcase classname="Suite" name="slow" time="10.0" />
        </testsuite>""",
    )
    baseline.write_text(json.dumps({"tests": {"Suite::slow": {"seconds": 2.0}}}), encoding="utf-8")

    report = analyze_test_durations(
        junit,
        baseline,
        top_n=5,
        slow_threshold=5.0,
        regression_factor=1.5,
        min_delta=5.0,
    )

    assert report["slow_count"] == 1
    assert report["regression_count"] == 1


def test_test_duration_report_main_missing_junit(tmp_path):
    summary = tmp_path / "summary.md"
    output = tmp_path / "output.txt"
    result = main(
        [
            "--junit-path",
            str(tmp_path / "missing.xml"),
            "--github-step-summary",
            str(summary),
            "--github-output",
            str(output),
        ]
    )
    assert result == 0
    assert "regression_count=0" in output.read_text(encoding="utf-8")
    assert "WARNING" in summary.read_text(encoding="utf-8")
