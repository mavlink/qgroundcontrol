#!/usr/bin/env python3
"""Emit the linux.yml debug-validation matrix as a JSON 'include' list.

All events run coverage + sanitizers. Sanitizers (~20 min) finish well before
the overall PR long pole (Android/Windows builds at 40-50 min), so including
them on PRs adds no wall-clock time and catches UB before it reaches master.
"""

from __future__ import annotations

import argparse
import json
import sys

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import write_github_output

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


def build_matrix() -> list[dict[str, str | int]]:
    """Return the matrix include-list."""
    return [COVERAGE_JOB, SANITIZER_JOB]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args(argv)

    include = build_matrix()
    serialized = json.dumps(include, separators=(",", ":"))
    write_github_output({"include": serialized})
    print(f"include={serialized}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
