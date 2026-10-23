#!/usr/bin/env python3
"""Verify a gcovr JSON coverage summary meets line and branch coverage thresholds."""

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.cmake import read_cache_var
from common.coverage_summary import SUMMARY_FILENAME, CoverageSummaryError, read_gcovr_summary
from common.gh_actions import gh_error, gh_group, gh_warning

DEFAULT_THRESHOLDS = {
    "QGC_COVERAGE_LINE_THRESHOLD": 30.0,
    "QGC_COVERAGE_BRANCH_THRESHOLD": 20.0,
}


def _threshold(explicit: float | None, build_dir: Path | None, name: str) -> float:
    if explicit is not None:
        return explicit
    if build_dir is not None:
        value = read_cache_var(build_dir / "CMakeCache.txt", name)
        if value:
            return float(value)
    return DEFAULT_THRESHOLDS[name]


def _below(covered: int, total: int, threshold: float) -> bool:
    # Compare exact counts: gcovr's percentages are rounded to one decimal.
    return covered * 100 < threshold * total


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-dir",
        type=Path,
        help=f"CMake build directory: default {SUMMARY_FILENAME} location and threshold cache "
        "variables",
    )
    parser.add_argument(
        "--coverage-summary",
        help=f"Path to the gcovr JSON summary (default: <build-dir>/{SUMMARY_FILENAME})",
    )
    parser.add_argument("--line-threshold", type=float)
    parser.add_argument("--branch-threshold", type=float)
    args = parser.parse_args(argv)
    line_threshold = _threshold(args.line_threshold, args.build_dir, "QGC_COVERAGE_LINE_THRESHOLD")
    branch_threshold = _threshold(
        args.branch_threshold, args.build_dir, "QGC_COVERAGE_BRANCH_THRESHOLD"
    )

    path = Path(args.coverage_summary or (args.build_dir or Path()) / SUMMARY_FILENAME)
    if not path.exists():
        gh_warning(f"{path.name} not found, skipping threshold check")
        return 0

    try:
        summary = read_gcovr_summary(path)
    except CoverageSummaryError as error:
        gh_error(str(error))
        return 1

    if summary.line_total == 0:
        gh_error(
            "Coverage report contains 0 lines — "
            f"lines-covered={summary.line_covered}, branches={summary.branch_total}"
        )
        output = path.parent / "coverage-output.txt"
        if output.exists():
            with gh_group("coverage-output tail"):
                for line in output.read_text(encoding="utf-8", errors="replace").splitlines()[-40:]:
                    print(line)
        return 1

    line_rate = f"{summary.line_percent:.1f}%"
    branch_rate = f"{summary.branch_percent:.1f}%" if summary.branch_total else "N/A"
    print(f"Line coverage: {line_rate} (threshold: {line_threshold}%)")
    print(f"Branch coverage: {branch_rate} (threshold: {branch_threshold}%)")

    failed = False
    if _below(summary.line_covered, summary.line_total, line_threshold):
        gh_error(
            f"Line coverage {line_rate} ({summary.line_covered} of {summary.line_total}) "
            f"is below {line_threshold}% threshold"
        )
        failed = True
    if _below(summary.branch_covered, summary.branch_total, branch_threshold):
        gh_error(
            f"Branch coverage {branch_rate} ({summary.branch_covered} of {summary.branch_total}) "
            f"is below {branch_threshold}% threshold"
        )
        failed = True

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
