#!/usr/bin/env python3
"""Summarize Ninja and Clang time-trace build hotspots.

build-profile.yml also installs the pinned ClangBuildAnalyzer with --install-clang-build-analyzer
and writes its job summary with --step-summary.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path

import _bootstrap as _bootstrap
from common.build_config import load_build_config
from common.cmake import read_cache_dict
from common.gh_actions import write_step_summary
from common.io import write_json
from common.markdown import code_block, code_span, summary_section
from common.net import download_file
from common.platform import host_arch, is_linux
from common.proc import run_captured
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
  %(prog)s --json-out report.json > report.txt

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
        "--json-out", type=Path, help="Also write the JSON report to this file (one scan)"
    )
    parser.add_argument(
        "--output-dir", type=Path, help="Write a Ninja-only CI snapshot (no trace scan)"
    )
    parser.add_argument(
        "--step-summary",
        action="store_true",
        help="Also add a Build profile section with the text report to the GitHub Actions job "
        "summary",
    )
    parser.add_argument(
        "--extra-report",
        action="append",
        default=[],
        metavar="TITLE=FILE",
        help="With --step-summary, also collapse FILE's text under TITLE (repeatable)",
    )
    parser.add_argument(
        "--install-clang-build-analyzer",
        type=Path,
        metavar="PATH",
        help="Only download build-config.json's SHA-256-pinned ClangBuildAnalyzer (Linux x86_64) "
        "to PATH",
    )
    args = parser.parse_args(argv)
    if args.output_dir and (args.json or args.json_out or args.step_summary):
        parser.error(
            "--output-dir writes report.md and report.json; omit --json/--json-out/--step-summary"
        )
    if args.extra_report and not args.step_summary:
        parser.error("--extra-report needs --step-summary")
    args.extra_reports = []
    for spec in args.extra_report:
        title, separator, path = spec.partition("=")
        if not (title and separator and path):
            parser.error(f"--extra-report expects TITLE=FILE, got {spec!r}")
        args.extra_reports.append((title, Path(path)))
    if args.install_clang_build_analyzer and (not is_linux() or host_arch() != "x86_64"):
        parser.error("The pinned ClangBuildAnalyzer release is a Linux x86_64 binary")
    return args


CLANG_BUILD_ANALYZER_ASSET = "ClangBuildAnalyzer-linux"


def install_clang_build_analyzer(binary: Path) -> Path:
    """Download the pinned ClangBuildAnalyzer to *binary*, verify its SHA-256, and make it executable."""
    pin = load_build_config()["tools"]["clang_build_analyzer"]
    url = (
        f"https://github.com/{pin['repository']}/releases/download/v{pin['version']}/"
        f"{CLANG_BUILD_ANALYZER_ASSET}"
    )
    binary.parent.mkdir(parents=True, exist_ok=True)
    download_file(url, binary, expected_sha256=pin["sha256"]["linux-x86_64"])
    binary.chmod(0o755)
    return binary


def _compiler_version(compiler: str) -> str:
    if not compiler:
        return ""
    result = run_captured([compiler, "--version"], timeout=30)
    return result.stdout.splitlines()[0] if result.returncode == 0 and result.stdout else ""


def step_summary(build_dir: Path, report: str, extra_reports: list[tuple[str, str]]) -> str:
    """Build profile job-summary section: build type, commit, compiler, then collapsed reports."""
    cache = read_cache_dict(build_dir / "CMakeCache.txt")
    commit = os.environ.get("GITHUB_SHA", "")
    compiler = _compiler_version(cache.get("CMAKE_CXX_COMPILER", ""))
    return summary_section(
        "Build profile",
        [
            ("Build type", cache.get("CMAKE_BUILD_TYPE", "")),
            ("Commit", code_span(commit) if commit else ""),
            ("Compiler", code_span(compiler) if compiler else ""),
        ],
        collapsed=[
            ("Ninja and per-TU hotspots (tools/build_profile.py)", code_block(report)),
            *((title, code_block(text)) for title, text in extra_reports),
        ],
    )


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    if args.install_clang_build_analyzer:
        print(f"Installed {install_clang_build_analyzer(args.install_clang_build_analyzer)}")
        return 0
    build_dir = args.build_dir.resolve()
    ninja_log = (args.ninja_log or build_dir / ".ninja_log").resolve()
    trace_dir = (args.trace_dir or build_dir).resolve()
    limit = max(1, args.limit)

    if args.output_dir:
        if ninja_log.is_file():
            print(write_snapshot(build_dir, args.output_dir, ninja_log=ninja_log, limit=limit))
        else:
            print(f"No Ninja log at {ninja_log}; this generator has no Ninja timing data.")
        return 0

    edges = parse_ninja_log(ninja_log)
    summary = summarize_ninja_log(edges, limit=limit)
    traces = find_time_traces(trace_dir)

    payload = build_json(summary, traces, limit=limit)
    if args.json_out:
        write_json(args.json_out, payload)
    report = build_report(summary, traces, limit=limit)
    if args.json:
        print(json.dumps(payload, indent=2))
    else:
        print(report)
    if args.step_summary:
        extra_reports = [
            (title, path.read_text(encoding="utf-8", errors="replace"))
            for title, path in args.extra_reports
        ]
        write_step_summary(step_summary(build_dir, report, extra_reports))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
