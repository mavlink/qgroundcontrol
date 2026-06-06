#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
"""Parse a qmltestrunner JUnit XML report and emit a JSON summary.

Reads the JUnit XML produced by `qmltestrunner -o report.xml,junitxml`
(or by `ctest --output-junit`) and writes a JSON object to stdout
containing test counts, per-case detail, and the 10 slowest cases.

Usage:
    parse-qmltestrunner-output.py <report.xml>
    parse-qmltestrunner-output.py --help

Exit codes:
    0 — JSON written to stdout
    1 — file not found, parse failure, or unsupported document shape;
        a JSON object {"error": "..."} is written to stdout.

Output schema:
    {
      "total": int,
      "passed": int,
      "failed": int,
      "skipped": int,
      "duration_ms": float,
      "cases": [
        {
          "name": str,
          "classname": str,
          "time_ms": float,
          "status": "passed" | "failed" | "skipped",
          "failure_message": str | null,
          "source": str | null   # "<path>.qml:<line>:<col>" if extractable
        },
        ...
      ],
      "slowest": [<top 10 cases by time_ms>]
    }
"""

from __future__ import annotations

import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import List, Dict, Optional, Any


SOURCE_RE = re.compile(
    # Three anchored alternatives so spaces in paths don't
    # bleed into surrounding prose:
    #   1. Windows drive-letter path: C:\Program Files\foo.qml
    #   2. Absolute Unix path:        /Users/First Last/foo.qml
    #   3. Relative path (no spaces): widgets/foo.qml
    r"((?:[A-Za-z]:\\[^\n:]*?|/[^\n:]*?|[\w./\\-]+)\.qml):(\d+)(?::(\d+))?"
)
USAGE = (
    "Usage: parse-qmltestrunner-output.py <report.xml>\n"
    "Reads a JUnit XML test report and writes a JSON summary to stdout.\n"
)


def _emit_error(msg: str) -> int:
    json.dump({"error": msg}, sys.stdout)
    sys.stdout.write("\n")
    return 1


def _parse_time_seconds(value: Optional[str]) -> float:
    """JUnit times are seconds (float). Missing → 0.0."""
    if not value:
        return 0.0
    try:
        return float(value)
    except ValueError:
        return 0.0


def _extract_source(text: str) -> Optional[str]:
    """Extract the first `<file>.qml:<line>[:<col>]` location from text.

    The regex captures `//foo.qml` from a `file:///foo.qml` URI;
    collapse the leading slashes. Windows/relative paths are untouched.
    """
    if not text:
        return None
    m = SOURCE_RE.search(text)
    if not m:
        return None
    file_, line, col = m.group(1), m.group(2), m.group(3)
    if file_.startswith("/"):
        file_ = "/" + file_.lstrip("/")
    return f"{file_}:{line}:{col}" if col else f"{file_}:{line}"


def _classify(testcase: ET.Element) -> Dict[str, Any]:
    """Return status + optional failure_message + source for a <testcase>."""
    failure = testcase.find("failure")
    error = testcase.find("error")
    skipped = testcase.find("skipped")

    if failure is not None or error is not None:
        node = failure if failure is not None else error
        msg = (node.get("message") or "").strip()
        body = (node.text or "").strip()
        combined = "\n".join(p for p in (msg, body) if p)
        return {
            "status": "failed",
            "failure_message": combined or None,
            "source": _extract_source(combined),
        }

    if skipped is not None:
        msg = (skipped.get("message") or "").strip() or (skipped.text or "").strip()
        return {
            "status": "skipped",
            "failure_message": msg or None,
            "source": None,
        }

    return {"status": "passed", "failure_message": None, "source": None}


def _walk_testcases(root: ET.Element) -> List[ET.Element]:
    """Find every <testcase> regardless of <testsuites>/<testsuite> nesting."""
    cases: List[ET.Element] = []
    if root.tag == "testcase":
        cases.append(root)
        return cases
    for testcase in root.iter("testcase"):
        cases.append(testcase)
    return cases


def parse_report(xml_path: Path) -> Dict[str, Any]:
    """Parse a JUnit XML file and return the summary dict."""
    if not xml_path.exists():
        raise FileNotFoundError(f"Report file not found: {xml_path}")

    try:
        tree = ET.parse(str(xml_path))
    except ET.ParseError as exc:
        raise ValueError(f"Failed to parse XML: {exc}") from exc

    root = tree.getroot()
    testcases = _walk_testcases(root)

    cases: List[Dict[str, Any]] = []
    duration_ms = 0.0

    for tc in testcases:
        time_ms = _parse_time_seconds(tc.get("time")) * 1000.0
        info = _classify(tc)
        case = {
            "name": tc.get("name", "") or "",
            "classname": tc.get("classname", "") or "",
            "time_ms": round(time_ms, 3),
            "status": info["status"],
            "failure_message": info["failure_message"],
            "source": info["source"],
        }
        cases.append(case)
        duration_ms += time_ms

    if not cases:
        # Empty document — surface as a soft error so the caller can
        # tell parse-success-no-tests apart from real success.
        raise ValueError(
            "No <testcase> elements found in report. The runner may "
            "have crashed before discovering tests, or the test "
            "directory contained no tst_*.qml files."
        )

    passed = sum(1 for c in cases if c["status"] == "passed")
    failed = sum(1 for c in cases if c["status"] == "failed")
    skipped = sum(1 for c in cases if c["status"] == "skipped")

    slowest = sorted(cases, key=lambda c: c["time_ms"], reverse=True)[:10]

    return {
        "total": len(cases),
        "passed": passed,
        "failed": failed,
        "skipped": skipped,
        "duration_ms": round(duration_ms, 3),
        "cases": cases,
        "slowest": slowest,
    }


def main(argv: List[str]) -> int:
    if len(argv) == 1 or argv[1] in ("-h", "--help"):
        sys.stdout.write(USAGE)
        return 0 if len(argv) > 1 else 1

    if len(argv) != 2:
        return _emit_error("Expected exactly one argument: <report.xml>")

    path = Path(argv[1])

    try:
        summary = parse_report(path)
    except FileNotFoundError as exc:
        return _emit_error(str(exc))
    except ValueError as exc:
        return _emit_error(str(exc))

    json.dump(summary, sys.stdout, indent=2)
    sys.stdout.write("\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
