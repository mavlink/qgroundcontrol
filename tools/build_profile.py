#!/usr/bin/env python3
"""Summarize Ninja and Clang time-trace build hotspots."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import _bootstrap as _bootstrap
from qgc_tools.build_profile import (
    build_json,
    build_report,
    find_time_traces,
    parse_ninja_log,
    summarize_ninja_log,
    write_snapshot,
)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Report Ninja and Clang time-trace build hotspots.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""Examples:
  %(prog)s
  %(prog)s --build-dir build-debug --limit 20
  %(prog)s --trace-dir build --json

Enable Clang traces with CMake option QGC_TIME_TRACE=ON, then rebuild.
""",
    )
    parser.add_argument(
        "-B",
        "--build-dir",
        default=Path("build"),
        type=Path,
        help="Build directory containing .ninja_log (default: build)",
    )
    parser.add_argument(
        "--ninja-log",
        type=Path,
        help="Explicit .ninja_log path (default: <build-dir>/.ninja_log)",
    )
    parser.add_argument(
        "--trace-dir",
        type=Path,
        help="Directory to scan for Clang -ftime-trace JSON files (default: build dir)",
    )
    parser.add_argument(
        "--limit", type=int, default=15, help="Rows to show per section (default: 15)"
    )
    parser.add_argument("--json", action="store_true", help="Emit machine-readable JSON")
    parser.add_argument(
        "--output-dir", type=Path, help="Write a Ninja-only CI snapshot (no trace scan)"
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    build_dir = args.build_dir.resolve()
    ninja_log = (args.ninja_log or build_dir / ".ninja_log").resolve()
    trace_dir = (args.trace_dir or build_dir).resolve()
    limit = max(1, args.limit)

    if args.output_dir:
        if ninja_log.is_file():
            print(write_snapshot(build_dir, args.output_dir))
        else:
            print(f"No Ninja log at {ninja_log}; this generator has no Ninja timing data.")
        return 0

    edges = parse_ninja_log(ninja_log)
    summary = summarize_ninja_log(edges, limit=limit)
    traces = find_time_traces(trace_dir)

    if args.json:
        print(json.dumps(build_json(summary, traces, limit=limit), indent=2))
    else:
        print(build_report(summary, traces, limit=limit))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
