#!/usr/bin/env python3
"""Generate coverage comment for PR from Cobertura XML."""

import argparse
import os
import sys
from typing import TypedDict

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.cobertura import CoberturaError, read_cobertura
from common.markdown import md_table


class CoverageData(TypedDict):
    line: float
    branch: float | None


def parse_coverage(xml_path: str) -> CoverageData | None:
    """Parse coverage.xml and return line/branch coverage percentages."""
    try:
        metrics = read_cobertura(xml_path)
        return {"line": metrics.line_percent, "branch": metrics.branch_percent}
    except CoberturaError as e:
        print(f"Error parsing {xml_path}: {e}", file=sys.stderr)
        return None


def coverage_badge(pct: float) -> str:
    """Return text label based on coverage percentage."""
    if pct >= 80:
        return "Good"
    elif pct >= 60:
        return "Fair"
    else:
        return "Low"


def delta_str(old: float | None, new: float) -> str:
    """Format coverage delta."""
    if old is None:
        return "N/A"
    delta = new - old
    if delta > 0:
        return f"+{delta:.2f}%"
    elif delta < 0:
        return f"{delta:.2f}%"
    else:
        return "No change"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate coverage comment for PR from Cobertura XML.",
    )
    parser.add_argument(
        "--coverage-xml",
        default=os.environ.get("COVERAGE_XML", "coverage.xml"),
        help="Path to coverage XML file (default: $COVERAGE_XML or coverage.xml)",
    )
    parser.add_argument(
        "--baseline-xml",
        default=os.environ.get("BASELINE_XML", ""),
        help="Path to baseline coverage XML file (default: $BASELINE_XML)",
    )
    parser.add_argument(
        "--output",
        default="coverage-comment.md",
        help="Output markdown file path (default: coverage-comment.md)",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    coverage_xml = args.coverage_xml
    baseline_xml = args.baseline_xml

    if not coverage_xml or not os.path.exists(coverage_xml):
        print("Coverage XML not found", file=sys.stderr)
        with open(args.output, "w", encoding="utf-8") as f:
            f.write("## Code Coverage Report\n\n*Coverage data not available*\n")
        return 0

    pr_cov = parse_coverage(coverage_xml)
    baseline_cov = None
    if baseline_xml and os.path.exists(baseline_xml):
        baseline_cov = parse_coverage(baseline_xml)

    lines = ["## Code Coverage Report", ""]

    if pr_cov:
        line_badge = coverage_badge(pr_cov["line"])
        pr_branch = pr_cov["branch"]

        if baseline_cov:
            line_delta = delta_str(baseline_cov["line"], pr_cov["line"])
            if pr_branch is None:
                branch_delta = "N/A"
            else:
                branch_delta = delta_str(baseline_cov.get("branch"), pr_branch)
        else:
            line_delta = "*No baseline*"
            branch_delta = "*No baseline*"

        rows = [[f"{line_badge} Lines", f"{pr_cov['line']:.2f}%", line_delta]]
        if pr_branch is not None:
            branch_badge = coverage_badge(pr_branch)
            rows.append([f"{branch_badge} Branches", f"{pr_branch:.2f}%", branch_delta])

        lines.append(md_table(["Metric", "Coverage", "\u0394 from master"], rows))
        lines.append("")

        if pr_cov["line"] < 60:
            lines.append("> **Warning:** Coverage is below 60%. Consider adding more tests.")
        elif pr_cov["line"] >= 80:
            lines.append("> Great coverage!")
    else:
        lines.append("*Coverage data not available*")

    lines.extend(
        [
            "",
            "<sub>View detailed coverage report in build artifacts or [Codecov](https://codecov.io).</sub>",
        ]
    )

    with open(args.output, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

    print("\n".join(lines))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
