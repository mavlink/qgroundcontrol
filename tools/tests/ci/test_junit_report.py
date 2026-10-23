"""Tests for junit_report.py."""

from __future__ import annotations

from typing import TYPE_CHECKING
from xml.sax.saxutils import escape, quoteattr

import pytest
from ci.reporting import junit_report as mod
from ci.reporting.junit_report import failure_excerpt, parse_junit

if TYPE_CHECKING:
    from pathlib import Path

    from syrupy.assertion import SnapshotAssertion

QT_FAILURE = """********* Start testing of MissionControllerTest *********
PASS   : MissionControllerTest::initTestCase()
FAIL!  : MissionControllerTest::_testGimbalRecalc() Compared values are not the same
   Actual   (visualItems->count()): 3
   Expected (4)                     : 4
   Loc: [src/MissionManager/MissionControllerTest.cc(412)]
PASS   : MissionControllerTest::cleanupTestCase()
Totals: 2 passed, 1 failed, 0 skipped, 0 blacklisted, 4120ms
"""
ASAN_FAILURE = """PASS   : VehicleLinkManagerTest::initTestCase()
==4242==ERROR: AddressSanitizer: heap-use-after-free on address 0x602000001234
    #0 0x55d1 in LinkInterface::_freeMavlinkChannel() src/Comms/LinkInterface.cc:88
    #1 0x55d2 in VehicleLinkManager::_removeLink() src/Vehicle/VehicleLinkManager.cc:201

SUMMARY: AddressSanitizer: heap-use-after-free src/Comms/LinkInterface.cc:88
"""
TIMEOUT_OUTPUT = """PASS   : FTPManagerTest::initTestCase()
QDEBUG : FTPManagerTest::_testLsList() Waiting for burst read
QDEBUG : FTPManagerTest::_testLsList() Waiting for burst read
"""


def junit_xml(cases: list[tuple[str, str, float, str, str]], wall: float) -> str:
    """CTest --output-junit XML; cases are (name, outcome, seconds, message, output)."""
    rows = []
    for name, outcome, seconds, message, output in cases:
        status = {"passed": "run", "failed": "fail", "skipped": "notrun"}[outcome]
        child = {
            "passed": "",
            "failed": f"<failure message={quoteattr(message)}/>",
            "skipped": '<skipped message="Disabled"/>',
        }[outcome]
        rows.append(
            f"<testcase name={quoteattr(name)} classname={quoteattr(name)} "
            f'time="{seconds}" status="{status}">'
            f"{child}<system-out>{escape(output)}</system-out></testcase>"
        )
    failures = sum(case[1] == "failed" for case in cases)
    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        f'<testsuite name="Linux-c++" tests="{len(cases)}" failures="{failures}" time="{wall}">\n'
        + "\n".join(rows)
        + "\n</testsuite>\n"
    )


def test_failure_excerpt_prefers_test_reports_over_output_tail() -> None:
    assert failure_excerpt("Failed", QT_FAILURE) == (
        "FAIL!  : MissionControllerTest::_testGimbalRecalc() Compared values are not the same",
        "   Actual   (visualItems->count()): 3",
        "   Expected (4)                     : 4",
        "   Loc: [src/MissionManager/MissionControllerTest.cc(412)]",
    )
    assert failure_excerpt("Failed", ASAN_FAILURE)[0].startswith("==4242==ERROR: AddressSanitizer")
    assert failure_excerpt("Failed", "a\n\nb\n") == ("a", "b")
    assert failure_excerpt("Timeout", "") == ("Timeout",)


def test_parse_junit_classifies_ctest_statuses(tmp_path: Path) -> None:
    path = tmp_path / "junit.xml"
    path.write_text(
        junit_xml(
            [
                ("Ok", "passed", 1.5, "", ""),
                ("Bad", "failed", 2.0, "Failed", "boom"),
                ("Off", "skipped", 0, "", ""),
            ],
            9,
        ),
        encoding="utf-8",
    )
    cases, wall = parse_junit(path, "suite")
    assert [(case.name, case.outcome, case.seconds) for case in cases] == [
        ("Ok", "passed", 1.5),
        ("Bad", "failed", 2.0),
        ("Off", "skipped", 0.0),
    ]
    assert cases[1].excerpt == ("boom",)
    assert wall == 9.0


def _write(tmp_path: Path, cases: list[tuple[str, str, float, str, str]], wall: float = 83) -> Path:
    path = tmp_path / "junit-results-linux-coverage-unit.xml"
    path.write_text(junit_xml(cases, wall), encoding="utf-8")
    return path


CASES = [
    ("MissionControllerTest", "failed", 95.2, "Failed", QT_FAILURE),
    ("FTPManagerTest", "failed", 61.5, "Timeout", TIMEOUT_OUTPUT),
    ("ParameterManagerTest", "passed", 48.7, "", ""),
    ("VehicleLinkManagerTest", "passed", 30.0, "", ""),
    ("SerialLinkTest", "skipped", 0, "", ""),
]


def test_summary_matches_snapshot(tmp_path: Path, snapshot: SnapshotAssertion) -> None:
    cases, wall = parse_junit(_write(tmp_path, CASES))
    summary = mod.build_summary(
        "Unit Tests (linux-coverage)", cases, wall, top_n=3, junit="junit.xml"
    )
    assert summary == snapshot


def test_main_writes_summary_and_annotates_failures_and_slow_tests(
    tmp_path: Path,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.setenv("INPUT_JUNIT_PATH", str(_write(tmp_path, CASES)))
    monkeypatch.setenv("INPUT_TITLE", "Unit Tests (macos)")
    monkeypatch.setenv("INPUT_SLOW_WARNINGS", "true")
    assert mod.main([]) == 0
    summary = gh_summary.read_text(encoding="utf-8")
    assert summary.startswith("### Unit Tests (macos)\n\n| Item | Value |")
    assert "| Result | 2 failed |" in summary
    assert "| Tests | 2 passed, 2 failed, 1 skipped |" in summary
    out = capsys.readouterr().out
    assert (
        "::error title=Test failed%3A MissionControllerTest::FAIL!  : "
        "MissionControllerTest::_testGimbalRecalc() Compared values are not the same%0A"
    ) in out
    assert "::error title=Test failed%3A FTPManagerTest::Timeout%0APASS" in out
    assert "::warning::Slow test (>=60.0s): MissionControllerTest took 95.200s" in out
    assert "::warning::Slow test (>=60.0s): FTPManagerTest took 61.500s" in out
    assert "ParameterManagerTest took" not in out


def test_slow_warnings_are_opt_in(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    path = _write(tmp_path, [("Slow", "passed", 90.0, "", "")])
    assert mod.main(["--junit-path", str(path)]) == 0
    assert "Slow test" not in capsys.readouterr().out


def test_failures_are_capped_in_annotations_and_the_list(
    tmp_path: Path, gh_summary: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    path = _write(tmp_path, [(f"T{i:02d}", "failed", 1.0, "Failed", QT_FAILURE) for i in range(25)])
    assert mod.main(["--junit-path", str(path)]) == 0
    assert capsys.readouterr().out.count("::error ") == mod.MAX_ANNOTATIONS
    assert "- …and 5 more" in gh_summary.read_text(encoding="utf-8")


@pytest.mark.parametrize("content", [None, "<testsuite"])
def test_missing_or_unreadable_junit_warns(
    tmp_path: Path, gh_summary: Path, capsys: pytest.CaptureFixture[str], content: str | None
) -> None:
    path = tmp_path / "junit.xml"
    if content is not None:
        path.write_text(content, encoding="utf-8")
    assert mod.main(["--junit-path", str(path), "--title", "Integration Tests"]) == 0
    assert "::warning::Cannot read JUnit report" in capsys.readouterr().out
    summary = gh_summary.read_text(encoding="utf-8")
    assert summary.startswith(
        "### Integration Tests\n\n| Item | Value |\n| --- | --- |\n| Result | no results |"
    )


def test_junit_path_is_required(capsys: pytest.CaptureFixture[str]) -> None:
    assert mod.main([]) == 2
    assert "needs --junit-path" in capsys.readouterr().out
