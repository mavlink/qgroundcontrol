#!/usr/bin/env python3
"""Verify coverage.xml meets line and branch coverage thresholds."""

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from xml_utils import xml_parse


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--coverage-xml", default="coverage.xml", help="Path to coverage.xml")
    parser.add_argument("--line-threshold", type=float, default=30.0)
    parser.add_argument("--branch-threshold", type=float, default=20.0)
    args = parser.parse_args()

    path = Path(args.coverage_xml)
    if not path.exists():
        print("::warning::coverage.xml not found, skipping threshold check")
        return 0

    root = xml_parse(str(path)).getroot()
    if root is None:
        print("::error::coverage.xml has no root element")
        return 1
    cov = root
    lines_valid = int(cov.get("lines-valid", 0))
    lines_covered = int(cov.get("lines-covered", 0))
    line_rate = float(cov.get("line-rate", 0)) * 100
    branch_rate = float(cov.get("branch-rate", 0)) * 100

    if lines_valid == 0:
        print(
            f"::error::Coverage report contains 0 lines — "
            f"lines-covered={lines_covered}, line-rate={line_rate:.2f}%, branch-rate={branch_rate:.2f}%"
        )
        output = path.parent / "coverage-output.txt"
        if output.exists():
            print("::group::coverage-output tail")
            for line in output.read_text(encoding="utf-8", errors="replace").splitlines()[-40:]:
                print(line)
            print("::endgroup::")
        return 1

    print(f"Line coverage: {line_rate:.1f}% (threshold: {args.line_threshold}%)")
    print(f"Branch coverage: {branch_rate:.1f}% (threshold: {args.branch_threshold}%)")

    failed = False
    if line_rate < args.line_threshold:
        print(f"::error::Line coverage {line_rate:.1f}% is below {args.line_threshold}% threshold")
        failed = True
    if branch_rate < args.branch_threshold:
        print(f"::error::Branch coverage {branch_rate:.1f}% is below {args.branch_threshold}% threshold")
        failed = True

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
