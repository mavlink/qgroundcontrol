"""Run a command with bounded retries and linear backoff."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

_tools_dir = Path(__file__).resolve().parents[1]
if str(_tools_dir) not in sys.path:
    sys.path.insert(0, str(_tools_dir))

from _bootstrap import ensure_tools_dir  # noqa: E402

ensure_tools_dir(__file__)

from common.proc import run_checked_with_retry  # noqa: E402


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse retry limits and the command to execute."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--max-attempts", type=int, default=3)
    parser.add_argument("--retry-backoff-seconds", type=float, default=5.0)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    if args.command[:1] == ["--"]:
        args.command = args.command[1:]
    if not args.command:
        parser.error("a command is required after '--'")
    return args


def main(argv: list[str] | None = None) -> int:
    """Run the requested command and return its final exit status."""
    args = parse_args(argv)
    try:
        result = run_checked_with_retry(
            args.command,
            max_attempts=args.max_attempts,
            retry_backoff_seconds=args.retry_backoff_seconds,
        )
    except subprocess.CalledProcessError as error:
        return error.returncode
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
