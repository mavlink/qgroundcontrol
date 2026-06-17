#!/usr/bin/env python3
"""Emit the linux.yml debug-validation matrix as a JSON 'include' list.

PR builds run coverage only; non-PR (push, merge_group) adds sanitizers.
Sanitizer runs add 30-50min and rarely catch what coverage misses, so we
gate them on post-merge events to keep PR feedback fast.
"""

from __future__ import annotations

import argparse
import json
import os
import sys

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import parse_bool, write_github_output

COVERAGE_JOB: dict[str, str | int] = {
    "job_name": "Test + Coverage linux_gcc_64 Debug",
    "mode": "coverage",
    "timeout_minutes": 60,
    "configure_extra": "",
    "exclude_labels": "Flaky|Network",
}

SANITIZER_JOB: dict[str, str | int] = {
    "job_name": "Sanitizers linux_gcc_64 Debug (ASan+UBSan)",
    "mode": "sanitizers",
    "timeout_minutes": 120,
    # QGC_TEST_DETECT_LEAKS=ON opts in to LSan; the default suppresses it per-test
    # so that non-sanitizer runs don't fail on expected Qt object leaks.
    "configure_extra": "-DQGC_ENABLE_ASAN=ON -DQGC_ENABLE_UBSAN=ON -DQGC_TEST_DETECT_LEAKS=ON",
    "exclude_labels": "Flaky|Network|NoSanitizer",
}


def build_matrix(is_pr: bool) -> list[dict[str, str | int]]:
    """Return the matrix include-list for the given event type."""
    return [COVERAGE_JOB] if is_pr else [COVERAGE_JOB, SANITIZER_JOB]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--is-pr",
        default=os.environ.get("IS_PR", "0"),
        help="'1'/'true' for PR builds (default from $IS_PR)",
    )
    args = parser.parse_args(argv)

    include = build_matrix(parse_bool(args.is_pr))
    serialized = json.dumps(include, separators=(",", ":"))
    write_github_output({"include": serialized})
    print(f"include={serialized}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
