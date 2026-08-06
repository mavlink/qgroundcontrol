#!/usr/bin/env python3
"""Retry an Android build after a known intermittent Qt deployment-settings
JSON truncation. Exits non-zero if the original failure was unrelated, so
non-retriable errors surface fast.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

import cmake_helper
from common.gh_actions import gh_error, gh_warning

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

def clean_settings_json(path: Path) -> None:
    """Best-effort: validate JSON for logging, then remove the file."""
    if not path.exists():
        return
    try:
        with path.open("r", encoding="utf-8") as fh:
            json.load(fh)
    except (json.JSONDecodeError, OSError):
        pass
    path.unlink(missing_ok=True)

def retry_build(build_dir: Path, build_type: str, retry_log: Path) -> int:
    """Re-run cmake --build with --parallel 1, tee output to retry_log."""
    cmd = [
        "cmake", "--build", str(build_dir),
        "--target", "all",
        "--config", build_type,
        "--parallel", "1",
    ]
    return cmake_helper._run_with_tee(cmd, str(retry_log))

def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--build-type", required=True)
    parser.add_argument(
        "--log-name",
        default="qgc-build.log",
        help="Original build log filename inside build-dir",
    )
    parser.add_argument(
        "--settings-name",
        default="android-QGroundControl-deployment-settings.json",
    )
    parser.add_argument(
        "--retry-log-name",
        default="qgc-build-retry.log",
    )
    args = parser.parse_args(argv)

    log_path = args.build_dir / args.log_name
    settings_path = args.build_dir / args.settings_name
    retry_log = args.build_dir / args.retry_log_name

    if not log_path.is_file():
        gh_error(f"Build failed and '{log_path}' is missing.")
        return 1

    if not detect_truncation(log_path):
        gh_error("Initial build failed for a non-retriable reason; skipping retry.")
        return 1

    gh_warning(
        "Detected truncated android deployment settings JSON. "
        "Retrying build with --parallel 1."
    )
    clean_settings_json(settings_path)
    return retry_build(args.build_dir, args.build_type, retry_log)

if __name__ == "__main__":
    raise SystemExit(main())
