#!/usr/bin/env python3
"""Retry an Android build after a known intermittent Qt deployment-settings
JSON truncation. Exits non-zero if the original failure was unrelated, so
non-retriable errors surface fast.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import gh_error, gh_warning
from common.proc import run_tee

LOG_NAME = "qgc-build.log"
RETRY_LOG_NAME = "qgc-build-retry.log"
SETTINGS_NAME = "android-QGroundControl-deployment-settings.json"
_TRUNCATION_RE = re.compile(
    r"Invalid json file: .*android-QGroundControl-deployment-settings\.json\. "
    r"Reason: unterminated object"
)


def detect_truncation(log_path: Path) -> bool:
    """Return True if the build log shows the deployment-settings truncation."""
    with log_path.open("r", encoding="utf-8", errors="replace") as fh:
        for line in fh:
            if _TRUNCATION_RE.search(line):
                return True
    return False


def retry_build(build_dir: Path, build_type: str, retry_log: Path) -> int:
    """Re-run cmake --build with --parallel 1, tee output to retry_log."""
    cmd = [
        "cmake",
        "--build",
        str(build_dir),
        "--target",
        "all",
        "--config",
        build_type,
        "--parallel",
        "1",
    ]
    return run_tee(cmd, retry_log)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--build-type", required=True)
    args = parser.parse_args(argv)

    log_path = args.build_dir / LOG_NAME

    if not log_path.is_file():
        gh_error(f"Build failed and '{log_path}' is missing.")
        return 1

    if not detect_truncation(log_path):
        gh_error("Initial build failed for a non-retriable reason; skipping retry.")
        return 1

    gh_warning(
        "Detected truncated android deployment settings JSON. Retrying build with --parallel 1."
    )
    (args.build_dir / SETTINGS_NAME).unlink(missing_ok=True)
    return retry_build(args.build_dir, args.build_type, args.build_dir / RETRY_LOG_NAME)


if __name__ == "__main__":
    raise SystemExit(main())
