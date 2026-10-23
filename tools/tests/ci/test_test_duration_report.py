from __future__ import annotations

from ci.reporting.test_duration_report import analyze_test_durations, main


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

    assert report["slow_warnings"] == [("Suite::slow", 10.0)]
    assert report["total_tests"] == 2
    assert len(report["top_slowest"]) == 2
    assert report["top_slowest"][0]["test"] == "Suite::slow"


def test_test_duration_report_main_missing_junit(tmp_path, gh_summary):
    result = main(["--junit-path", str(tmp_path / "missing.xml")])
    assert result == 0
    assert "WARNING" in gh_summary.read_text(encoding="utf-8")


def test_main_summarizes_and_warns_without_writing_files(tmp_path, monkeypatch, capsys, gh_summary):
    junit = tmp_path / "junit.xml"
    junit.write_text(
        '<testsuite><testcase classname="Suite" name="slow" time="90.0" /></testsuite>',
        encoding="utf-8",
    )
    monkeypatch.chdir(tmp_path)

    assert main(["--junit-path", str(junit)]) == 0

    assert "`Suite::slow`" in gh_summary.read_text(encoding="utf-8")
    assert "Slow test (>=60.0s): Suite::slow took 90.000s" in capsys.readouterr().out
    assert sorted(path.name for path in tmp_path.iterdir()) == ["gh_summary", "junit.xml"]
