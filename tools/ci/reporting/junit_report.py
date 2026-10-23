#!/usr/bin/env python3
"""Summarize CTest JUnit results in the job summary and annotate failed and slow tests.

The section lists counts, each failure with its first report lines, and the slowest tests.
Failed tests become error annotations; with --slow-warnings, tests over the threshold become
warnings. Also the JUnit parser that the build-results comment uses.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from xml.etree.ElementTree import Element

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.format import format_count, format_duration
from common.gh_actions import env_default, env_flag, gh_error, gh_warning, write_step_summary
from common.markdown import code_block, code_span, md_table, more_items, summary_section
from common.xml import XMLParseError, xml_parse

# Test names come from pull request code; clip them wherever they are shown.
MAX_NAME_CHARS = 120
# GitHub shows at most 10 error annotations per step.
MAX_ANNOTATIONS = 10
MAX_LISTED_FAILURES = 20
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


def clip(value: Any, limit: int = MAX_NAME_CHARS) -> str:
    """Return *value* on one line, shortened to *limit* characters."""
    text = " ".join(str(value).split())
    return text if len(text) <= limit else f"{text[: limit - 1]}…"


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


def parse_junit(path: Path, suite: str = "") -> tuple[list[JUnitCase], float | None]:
    """Return the test cases of a CTest JUnit file and its wall time (the testsuite's time)."""
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


def count(cases: list[JUnitCase], outcome: str) -> int:
    return sum(case.outcome == outcome for case in cases)


def build_summary(
    title: str, cases: list[JUnitCase], wall: float | None, *, top_n: int, junit: str
) -> str:
    """Render the job-summary section for one JUnit report."""
    failed = [case for case in cases if case.outcome == "failed"]
    skipped = [case for case in cases if case.outcome == "skipped"]
    ran = sorted(
        (case for case in cases if case.outcome != "skipped"),
        key=lambda case: case.seconds,
        reverse=True,
    )
    slowest = ran[0] if ran else None
    facts = [
        ("Result", f"{len(failed)} failed" if failed else "passed"),
        (
            "Tests",
            f"{format_count(count(cases, 'passed'))} passed, {format_count(len(failed))} failed, "
            f"{format_count(len(skipped))} skipped",
        ),
        ("Time", format_duration(wall if wall is not None else sum(c.seconds for c in cases))),
        (
            "Slowest",
            f"{code_span(slowest.name)} ({format_duration(slowest.seconds)})" if slowest else "",
        ),
        ("JUnit", code_span(junit)),
    ]
    body = ""
    if failed:
        lines = [
            f"- {code_span(case.name)}: "
            f"{code_span(clip(case.excerpt[0], 200)) if case.excerpt else 'failed'}"
            for case in failed[:MAX_LISTED_FAILURES]
        ]
        if len(failed) > MAX_LISTED_FAILURES:
            lines.append(f"- {more_items(len(failed) - MAX_LISTED_FAILURES)}")
        body = "**Failed tests**\n\n" + "\n".join(lines)
    output = "\n\n".join(
        f"**{code_span(case.name)}**\n\n{code_block(chr(10).join(case.excerpt) or '(no output)', 'text')}"
        for case in failed[:MAX_LISTED_FAILURES]
    )
    slow_table = md_table(
        ["Test", "Time"],
        [[code_span(case.name), format_duration(case.seconds)] for case in ran[:top_n]],
        align=["left", "right"],
        markdown=True,
    )
    skipped_list = "\n".join(f"- {code_span(case.name)}" for case in skipped[:50])
    if len(skipped) > 50:
        skipped_list += f"\n- {more_items(len(skipped) - 50)}"
    return summary_section(
        title,
        facts,
        body=body,
        collapsed=[
            ("Failure output", output),
            (f"Slowest tests (top {min(top_n, len(ran))})", slow_table if ran else ""),
            (f"Skipped tests ({len(skipped)})", skipped_list),
        ],
    )


def annotate(cases: list[JUnitCase], *, slow_threshold: float | None) -> None:
    """Annotate failed tests as errors and, with a threshold, slow tests as warnings."""
    failed = [case for case in cases if case.outcome == "failed"]
    for case in failed[:MAX_ANNOTATIONS]:
        gh_error("\n".join(case.excerpt[:6]) or "failed", title=f"Test failed: {case.name}")
    if slow_threshold is None:
        return
    for case in sorted(cases, key=lambda case: case.seconds, reverse=True):
        if case.seconds < slow_threshold:
            break
        gh_warning(f"Slow test (>={slow_threshold:.1f}s): {case.name} took {case.seconds:.3f}s")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse arguments; defaults come from the run-unit-tests action's INPUT_* environment."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--junit-path", default=env_default("INPUT_JUNIT_PATH"))
    parser.add_argument("--title", default=env_default("INPUT_TITLE", "Tests"))
    parser.add_argument("--top-n", type=int, default=10)
    parser.add_argument(
        "--slow-warnings",
        action="store_true",
        default=env_flag("INPUT_SLOW_WARNINGS"),
        help="Annotate tests slower than --slow-threshold-seconds as warnings",
    )
    parser.add_argument("--slow-threshold-seconds", type=float, default=60.0)
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    if not args.junit_path:
        gh_error("junit_report.py needs --junit-path (or INPUT_JUNIT_PATH)")
        return 2
    junit = Path(args.junit_path)
    try:
        cases, wall = parse_junit(junit)
    except (OSError, XMLParseError) as exc:
        message = f"Cannot read JUnit report {junit}: {exc}"
        gh_warning(message)
        write_step_summary(summary_section(args.title, [("Result", "no results")], body=message))
        return 0
    write_step_summary(build_summary(args.title, cases, wall, top_n=args.top_n, junit=junit.name))
    annotate(cases, slow_threshold=args.slow_threshold_seconds if args.slow_warnings else None)
    return 0


if __name__ == "__main__":
    sys.exit(main())
