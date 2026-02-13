#!/usr/bin/env python3
"""Generate combined build results report for PR comments.

Aggregates test results, coverage, and artifact sizes into markdown.

Usage:
    build_results.py --artifacts-dir artifacts/ --output comment.md
    build_results.py --artifacts-dir artifacts/ --baseline-coverage baseline.xml
    build_results.py --artifacts-dir artifacts/ --baseline-sizes sizes.json

Reads:
    artifacts/test-results-*/test-output.txt  - Test output logs
    artifacts/coverage-report/coverage.xml    - Coverage report
    pr-sizes.json                             - Current artifact sizes

Outputs:
    Markdown report with test results, coverage diff, and size comparison.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class ArchTestResults:
    """Test results for one architecture."""

    arch: str
    passed: int = 0
    failed: int = 0
    skipped: int = 0
    failures: list[str] = field(default_factory=list)


@dataclass
class CoverageInfo:
    """Coverage information."""

    line_rate: float = 0.0
    baseline_rate: float | None = None

    @property
    def percentage(self) -> str:
        return f"{self.line_rate * 100:.1f}%"

    @property
    def diff(self) -> str | None:
        if self.baseline_rate is None:
            return None
        delta = (self.line_rate - self.baseline_rate) * 100
        return f"{delta:+.1f}%"


@dataclass
class ArtifactSize:
    """Artifact size information."""

    name: str
    size_bytes: int
    size_human: str
    baseline_bytes: int | None = None

    @property
    def delta(self) -> str | None:
        if self.baseline_bytes is None:
            return None
        diff = self.size_bytes - self.baseline_bytes
        if diff == 0:
            return "No change"
        mb = diff / 1024 / 1024
        direction = "increase" if diff > 0 else "decrease"
        return f"{mb:+.2f} MB ({direction})"


def parse_test_results(artifacts_dir: Path) -> list[ArchTestResults]:
    """Parse test results from all architecture directories."""
    results = []

    for test_file in sorted(artifacts_dir.glob("test-results-*/test-output.txt")):
        # Extract architecture from directory name
        arch = test_file.parent.name.replace("test-results-", "")

        summary = ArchTestResults(arch=arch)

        try:
            content = test_file.read_text()

            # Count results
            for line in content.splitlines():
                if line.startswith("PASS"):
                    summary.passed += 1
                elif line.startswith("FAIL"):
                    summary.failed += 1
                    summary.failures.append(line)
                elif line.startswith("SKIP"):
                    summary.skipped += 1

            results.append(summary)
        except OSError:
            continue

    return results


def parse_coverage(coverage_file: Path) -> float | None:
    """Parse coverage XML and return line rate."""
    if not coverage_file.exists():
        return None

    try:
        tree = ET.parse(coverage_file)
        root = tree.getroot()
        return float(root.get("line-rate", 0))
    except (ET.ParseError, ValueError):
        return None


def get_coverage_info(
    artifacts_dir: Path, baseline_file: Path | None
) -> CoverageInfo | None:
    """Get coverage info with optional baseline comparison."""
    coverage_file = artifacts_dir / "coverage-report" / "coverage.xml"
    current_rate = parse_coverage(coverage_file)

    if current_rate is None:
        return None

    info = CoverageInfo(line_rate=current_rate)

    if baseline_file and baseline_file.exists():
        info.baseline_rate = parse_coverage(baseline_file)

    return info


def load_sizes(sizes_file: Path) -> dict[str, dict]:
    """Load artifact sizes from JSON file."""
    if not sizes_file.exists():
        return {}

    try:
        with open(sizes_file) as f:
            data = json.load(f)
            return {a["name"]: a for a in data.get("artifacts", [])}
    except (json.JSONDecodeError, KeyError):
        return {}


def get_artifact_sizes(
    pr_sizes_file: Path, baseline_file: Path | None
) -> list[ArtifactSize]:
    """Get artifact sizes with optional baseline comparison."""
    current = load_sizes(pr_sizes_file)
    baseline = load_sizes(baseline_file) if baseline_file else {}

    sizes = []
    for name, info in sorted(current.items()):
        size = ArtifactSize(
            name=name,
            size_bytes=info["size_bytes"],
            size_human=info["size_human"],
        )

        if name in baseline:
            size.baseline_bytes = baseline[name]["size_bytes"]

        sizes.append(size)

    return sizes


def generate_test_section(results: list[ArchTestResults]) -> str:
    """Generate markdown for test results section."""
    if not results:
        return ""

    lines = ["### Test Results", ""]

    total_passed = sum(r.passed for r in results)
    total_failed = sum(r.failed for r in results)
    total_skipped = sum(r.skipped for r in results)
    has_failures = any(r.failed > 0 for r in results)

    for r in results:
        if r.failed > 0:
            lines.append(
                f"**{r.arch}**: {r.passed} passed, {r.failed} failed, {r.skipped} skipped"
            )
            lines.append("")
            lines.append("<details><summary>Failed tests</summary>")
            lines.append("")
            lines.append("```")
            lines.extend(r.failures[:20])  # Limit to 20 failures
            lines.append("```")
            lines.append("</details>")
        else:
            lines.append(f"**{r.arch}**: {r.passed} passed, {r.skipped} skipped")
        lines.append("")

    if has_failures:
        lines.append(
            f"**Total: {total_passed} passed, {total_failed} failed, {total_skipped} skipped**"
        )
    else:
        lines.append(f"**Total: {total_passed} passed, {total_skipped} skipped**")
    lines.append("")

    return "\n".join(lines)


def generate_coverage_section(coverage: CoverageInfo | None) -> str:
    """Generate markdown for coverage section."""
    if coverage is None:
        return ""

    lines = ["### Code Coverage", ""]

    if coverage.baseline_rate is not None:
        lines.extend(
            [
                "| Coverage | Baseline | Change |",
                "|----------|----------|--------|",
                f"| {coverage.percentage} | {coverage.baseline_rate * 100:.1f}% | {coverage.diff} |",
            ]
        )
    else:
        lines.append(f"Coverage: **{coverage.percentage}**")
        lines.append("")
        lines.append("*No baseline available for comparison*")

    lines.append("")
    return "\n".join(lines)


def generate_sizes_section(sizes: list[ArtifactSize]) -> str:
    """Generate markdown for artifact sizes section."""
    if not sizes:
        return ""

    lines = ["### Artifact Sizes", ""]

    has_baseline = any(s.baseline_bytes is not None for s in sizes)

    if has_baseline:
        lines.extend(
            [
                "| Artifact | Size | Δ from master |",
                "|----------|------|---------------|",
            ]
        )
        for s in sizes:
            delta = s.delta or "New"
            lines.append(f"| {s.name} | {s.size_human} | {delta} |")
    else:
        lines.extend(
            [
                "| Artifact | Size |",
                "|----------|------|",
            ]
        )
        for s in sizes:
            lines.append(f"| {s.name} | {s.size_human} |")

    lines.append("")

    # Total delta
    if has_baseline:
        total_delta = sum(
            (s.size_bytes - s.baseline_bytes)
            for s in sizes
            if s.baseline_bytes is not None
        )
        if total_delta != 0:
            direction = "increased" if total_delta > 0 else "decreased"
            lines.append(
                f"**Total size {direction} by {abs(total_delta) / 1024 / 1024:.2f} MB**"
            )
    else:
        lines.append("*No baseline available for comparison*")

    lines.append("")
    return "\n".join(lines)


def generate_report(
    artifacts_dir: Path,
    pr_sizes_file: Path | None,
    baseline_coverage: Path | None,
    baseline_sizes: Path | None,
    builds_table: str | None = None,
    builds_summary: str | None = None,
) -> str:
    """Generate the complete markdown report."""
    sections = ["## Build Results", ""]

    # Platform status (passed through from workflow)
    if builds_table:
        sections.extend(["### Platform Status", "", builds_table])
        if builds_summary:
            sections.append(f"**{builds_summary}**")
        sections.append("")

    # Test results
    test_results = parse_test_results(artifacts_dir)
    if test_results:
        sections.append(generate_test_section(test_results))

    # Coverage
    coverage = get_coverage_info(artifacts_dir, baseline_coverage)
    if coverage:
        sections.append(generate_coverage_section(coverage))

    # Artifact sizes
    if pr_sizes_file and pr_sizes_file.exists():
        sizes = get_artifact_sizes(pr_sizes_file, baseline_sizes)
        if sizes:
            sections.append(generate_sizes_section(sizes))

    return "\n".join(sections)


def parse_args() -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Generate combined build results report",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )

    parser.add_argument(
        "--artifacts-dir",
        type=Path,
        default=Path("artifacts"),
        help="Directory containing build artifacts",
    )
    parser.add_argument(
        "--pr-sizes",
        type=Path,
        help="JSON file with current artifact sizes",
    )
    parser.add_argument(
        "--baseline-coverage",
        type=Path,
        help="Baseline coverage XML for comparison",
    )
    parser.add_argument(
        "--baseline-sizes",
        type=Path,
        help="Baseline sizes JSON for comparison",
    )
    parser.add_argument(
        "--builds-table",
        help="Markdown table of build status (from workflow)",
    )
    parser.add_argument(
        "--builds-summary",
        help="Build summary text (from workflow)",
    )
    parser.add_argument(
        "--output",
        "-o",
        type=Path,
        help="Output file (default: stdout)",
    )

    return parser.parse_args()


def main() -> int:
    """Main entry point."""
    args = parse_args()

    report = generate_report(
        artifacts_dir=args.artifacts_dir,
        pr_sizes_file=args.pr_sizes,
        baseline_coverage=args.baseline_coverage,
        baseline_sizes=args.baseline_sizes,
        builds_table=args.builds_table,
        builds_summary=args.builds_summary,
    )

    if args.output:
        args.output.write_text(report)
        print(f"Report written to {args.output}")
    else:
        print(report)

    return 0


if __name__ == "__main__":
    sys.exit(main())
