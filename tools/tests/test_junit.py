"""Tests for tools/common/junit.py."""

from __future__ import annotations

from typing import TYPE_CHECKING
from xml.sax.saxutils import escape, quoteattr

import pytest
from common.junit import failure_excerpt, parse_junit
from common.xml import XMLParseError

if TYPE_CHECKING:
    from pathlib import Path

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


def test_parse_junit_clips_long_names_and_reads_errors(tmp_path: Path) -> None:
    path = tmp_path / "junit.xml"
    path.write_text(
        '<testsuites><testsuite><testcase name="{}" time="x">'
        '<error message="Crashed"/></testcase></testsuite></testsuites>'.format("T" * 200),
        encoding="utf-8",
    )
    cases, wall = parse_junit(path)
    assert len(cases[0].name) == 120
    assert cases[0].name.endswith("…")
    assert (cases[0].outcome, cases[0].seconds, cases[0].excerpt) == ("failed", 0.0, ("Crashed",))
    assert wall is None


def test_parse_junit_rejects_entity_declarations(tmp_path: Path) -> None:
    path = tmp_path / "junit.xml"
    path.write_text(
        '<!DOCTYPE t [<!ENTITY x "y">]><testsuite><testcase name="&x;"/></testsuite>',
        encoding="utf-8",
    )
    with pytest.raises(XMLParseError):
        parse_junit(path)
