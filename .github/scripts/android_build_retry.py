#!/usr/bin/env python3
"""Retry known-intermittent Android build failures.

The retry is deliberately limited to Qt deployment-settings truncation and
transient Gradle dependency-repository access failures. Unrelated build errors
remain fatal so a retry cannot hide a source or configuration regression.
"""

from __future__ import annotations

import argparse
import json
import re
import time
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh_error, gh_warning
from common.proc import run_tee

_TRUNCATION_RE = re.compile(
    r"Invalid json file: .*android-QGroundControl-deployment-settings\.json\. "
    r"Reason: unterminated object"
)
_GRADLE_REPOSITORY_RE = re.compile(
    r"https://(?:repo\.maven\.apache\.org|dl\.google\.com|plugins\.gradle\.org|jitpack\.io)/",
    re.IGNORECASE,
)
_TRANSIENT_NETWORK_RE = re.compile(
    r"(?:"
    r"Received status code (?:403|408|429|5\d\d)\b"
    r"|(?:connect|read) timed out"
    r"|connection (?:reset|refused)"
    r"|temporary failure in name resolution"
    r"|remote host terminated the handshake"
    r")",
    re.IGNORECASE,
)
_REPOSITORY_RETRY_DELAY_SECONDS = 15


def _read_log(log_path: Path) -> str:
    return log_path.read_text(encoding="utf-8", errors="replace")


def detect_truncation(log_path: Path) -> bool:
    """Return True if the build log shows the deployment-settings truncation."""
    return _TRUNCATION_RE.search(_read_log(log_path)) is not None


def detect_gradle_repository_failure(log_path: Path) -> bool:
    """Return True for transient access failures from Android dependency repositories."""
    log_text = _read_log(log_path)
    return bool(_GRADLE_REPOSITORY_RE.search(log_text) and _TRANSIENT_NETWORK_RE.search(log_text))


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

    truncation = detect_truncation(log_path)
    repository_failure = detect_gradle_repository_failure(log_path)
    if not truncation and not repository_failure:
        gh_error("Initial build failed for a non-retriable reason; skipping retry.")
        return 1

    if truncation:
        gh_warning(
            "Detected truncated Android deployment settings JSON. Retrying build with --parallel 1."
        )
        clean_settings_json(settings_path)
    else:
        gh_warning(
            "Detected a transient Gradle dependency-repository access failure. "
            f"Retrying build in {_REPOSITORY_RETRY_DELAY_SECONDS} seconds."
        )
        time.sleep(_REPOSITORY_RETRY_DELAY_SECONDS)

    return retry_build(args.build_dir, args.build_type, retry_log)


if __name__ == "__main__":
    raise SystemExit(main())
