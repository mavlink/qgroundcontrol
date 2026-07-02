#!/usr/bin/env python3
"""Analyze JUnit test durations and emit CI summaries."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    import xml.etree.ElementTree as ET

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh_warning, write_github_output, write_step_summary
from common.markdown import md_table
from xml_utils import xml_parse


def test_key(elem: ET.Element) -> str:
    """Return a stable identifier for a JUnit testcase element."""
    classname = elem.attrib.get("classname", "").strip()
    name = elem.attrib.get("name", "").strip()
    if classname and name:
        return f"{classname}::{name}"
    return name or classname or "<unnamed>"


def parse_time(value: str) -> float:
    """Parse testcase duration, defaulting invalid values to 0."""
    try:
        return float(value)
    except (ValueError, TypeError):
        return 0.0


def analyze_test_durations(
    junit_path: Path,
    *,
    top_n: int,
    slow_threshold: float,
) -> dict[str, Any]:
    """Return computed timing report data for a JUnit XML file."""
    tree = xml_parse(junit_path)
    root = tree.getroot()
    if root is None:
        raise ValueError(f"JUnit XML {junit_path} has no root element")
    cases = [
        (test_key(testcase), parse_time(testcase.attrib.get("time", "0")))
        for testcase in root.iter("testcase")
    ]

    total_seconds = sum(secs for _, secs in cases)
    slowest = sorted(cases, key=lambda item: item[1], reverse=True)
    slow_over_threshold = [(key, secs) for key, secs in slowest if secs >= slow_threshold]

    return {
        "junit_path": str(junit_path),
        "total_tests": len(cases),
        "total_seconds": total_seconds,
        "slow_threshold_seconds": slow_threshold,
        "slow_tests": [{"test": key, "seconds": secs} for key, secs in slow_over_threshold],
        "top_slowest": [{"test": key, "seconds": secs} for key, secs in slowest[:top_n]],
        "slow_count": len(slow_over_threshold),
        "slow_warnings": slow_over_threshold[:top_n],
    }


def build_summary(report: dict[str, Any], *, missing_junit: str = "") -> str:
    """Render a markdown summary for GitHub Step Summary."""
    lines = ["## Test Duration Report", ""]
    if missing_junit:
        lines.append(f"- WARNING: {missing_junit}")
        lines.append("")
        return "\n".join(lines)

    lines.extend(
        [
            f"- JUnit: `{report['junit_path']}`",
            f"- Total tests: **{report['total_tests']}**",
            f"- Total time: **{report['total_seconds']:.1f}s**",
            f"- Slow threshold: **{report['slow_threshold_seconds']:.1f}s**",
        ]
    )
    lines.append("")

    top_slowest = report["top_slowest"]
    if top_slowest:
        lines.append(f"### Top {len(top_slowest)} Slowest Tests")
        lines.append("")
        lines.append(
            md_table(
                ["Test", "Seconds"],
                [[f"`{item['test']}`", f"{item['seconds']:.3f}"] for item in top_slowest],
                align=["left", "right"],
            )
        )
        lines.append("")

    return "\n".join(lines)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(description="Analyze JUnit test durations.")
    parser.add_argument("--junit-path", required=True)
    parser.add_argument("--report-json-path", default="test-duration-report.json")
    parser.add_argument("--top-n", type=int, default=20)
    parser.add_argument("--slow-threshold-seconds", type=float, default=60.0)
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Run the report generation flow."""
    args = parse_args(argv)
    junit_path = Path(args.junit_path)
    report_json_path = Path(args.report_json_path)

    if not junit_path.exists():
        message = f"JUnit report not found at {junit_path}"
        gh_warning(message)
        write_step_summary(build_summary({}, missing_junit=message))
        write_github_output({"slow_count": "0"})
        return 0

    report = analyze_test_durations(
        junit_path,
        top_n=args.top_n,
        slow_threshold=args.slow_threshold_seconds,
    )
    report_json_path.parent.mkdir(parents=True, exist_ok=True)
    report_json_path.write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    write_step_summary(build_summary(report))
    write_github_output({"slow_count": str(report["slow_count"])})

    for key, secs in report["slow_warnings"]:
        gh_warning(f"Slow test (>={args.slow_threshold_seconds:.1f}s): {key} took {secs:.3f}s")

    return 0


if __name__ == "__main__":
    sys.exit(main())
