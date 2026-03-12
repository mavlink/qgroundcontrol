#!/usr/bin/env python3
"""Prepare pre-commit result artifacts for CI."""

from __future__ import annotations

import argparse
import json
import shutil
import sys
from pathlib import Path


def write_results_json(
    output_dir: Path,
    *,
    exit_code: str,
    passed: str,
    failed: str,
    skipped: str,
    run_url: str,
) -> Path:
    """Write the summary JSON artifact."""
    output_dir.mkdir(parents=True, exist_ok=True)
    output_path = output_dir / "pre-commit-results.json"
    output_path.write_text(
        json.dumps(
            {
                "exit_code": exit_code,
                "passed": passed,
                "failed": failed,
                "skipped": skipped,
                "run_url": run_url,
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )
    return output_path


def copy_output_file(source: Path | None, output_dir: Path) -> None:
    """Copy the text output file into the artifact directory if it exists."""
    if source is None or not source.exists():
        return
    shutil.copy2(source, output_dir / source.name)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(description="Prepare pre-commit result artifacts.")
    parser.add_argument("--output-dir", default="pre-commit-results")
    parser.add_argument("--exit-code", default="1")
    parser.add_argument("--passed", default="0")
    parser.add_argument("--failed", default="0")
    parser.add_argument("--skipped", default="0")
    parser.add_argument("--run-url", default="")
    parser.add_argument("--output-file", default="")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Generate the artifact directory for pre-commit results."""
    args = parse_args(argv)
    output_dir = Path(args.output_dir)
    write_results_json(
        output_dir,
        exit_code=args.exit_code,
        passed=args.passed,
        failed=args.failed,
        skipped=args.skipped,
        run_url=args.run_url,
    )
    copy_output_file(Path(args.output_file) if args.output_file else None, output_dir)
    return 0


if __name__ == "__main__":
    sys.exit(main())
