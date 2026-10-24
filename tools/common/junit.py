"""CTest JUnit XML parsing: test cases, outcomes, durations, and failure excerpts."""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import TYPE_CHECKING

from .format import clip
from .xml import xml_parse

if TYPE_CHECKING:
    from os import PathLike
    from xml.etree.ElementTree import Element

__all__ = ["JUnitCase", "failure_excerpt", "parse_junit"]

# First line of a QtTest, sanitizer, or fatal-message failure; indented lines that follow it
# (Actual/Expected/Loc, stack frames) belong to it.
_FAILURE_START = re.compile(
    r"^(?:FAIL!|XPASS|QFATAL|QCRITICAL)\s*:|(?:ERROR|SUMMARY): \w*Sanitizer|runtime error:"
)
# CTest's failure message when a test only exited non-zero; the output says more.
_GENERIC_FAILURE_MESSAGES = {"", "failed"}
_EXCERPT_MAX_LINES = 12


@dataclass(frozen=True)
class JUnitCase:
    name: str
    suite: str
    outcome: str
    seconds: float
    excerpt: tuple[str, ...] = ()


def _case_outcome(case: Element) -> str:
    if case.find("failure") is not None or case.find("error") is not None:
        return "failed"
    if case.get("status") == "fail":
        return "failed"
    if case.find("skipped") is not None or case.get("status") in {"notrun", "disabled"}:
        return "skipped"
    return "passed"


def failure_excerpt(message: str, output: str) -> tuple[str, ...]:
    """Return the lines that explain a test failure: its failure reports, else the output tail."""
    lines = output.splitlines()
    picked: list[str] = []
    index = 0
    while index < len(lines) and len(picked) < _EXCERPT_MAX_LINES:
        if not _FAILURE_START.search(lines[index]):
            index += 1
            continue
        picked.append(lines[index].rstrip())
        index += 1
        while (
            index < len(lines)
            and lines[index][:1].isspace()
            and lines[index].strip()
            and len(picked) < _EXCERPT_MAX_LINES
        ):
            picked.append(lines[index].rstrip())
            index += 1
    if picked:
        return tuple(picked)
    tail = [line.rstrip() for line in lines if line.strip()][-6:]
    head = [] if message.strip().lower() in _GENERIC_FAILURE_MESSAGES else [message.strip()]
    return tuple(head + tail)


def parse_junit(path: str | PathLike[str], suite: str = "") -> tuple[list[JUnitCase], float | None]:
    """Return the test cases of a CTest JUnit file and its wall time (the testsuite's time).

    Test names are clipped (``common.format.clip``) because they come from pull request code.
    Raises ``OSError`` or ``common.xml.XMLParseError`` for an unreadable report.
    """
    root = xml_parse(path).getroot()
    if root is None:
        return [], None
    cases: list[JUnitCase] = []
    for case in root.iter("testcase"):
        outcome = _case_outcome(case)
        excerpt: tuple[str, ...] = ()
        if outcome == "failed":
            failure = case.find("failure")
            if failure is None:
                failure = case.find("error")
            message = failure.get("message", "") if failure is not None else ""
            excerpt = failure_excerpt(message, case.findtext("system-out") or "")
        try:
            seconds = float(case.get("time", "0"))
        except ValueError:
            seconds = 0.0
        name = clip(case.get("name") or case.get("classname") or "<unnamed>")
        cases.append(JUnitCase(name, suite, outcome, seconds, excerpt))
    try:
        wall = float(root.get("time", "")) if root.tag == "testsuite" else None
    except ValueError:
        wall = None
    return cases, wall
