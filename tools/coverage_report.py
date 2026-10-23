#!/usr/bin/env python3
"""Generate code coverage reports for QGroundControl."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import webbrowser
from pathlib import Path

import _bootstrap as _bootstrap
from common.coverage_summary import SUMMARY_FILENAME, CoverageSummaryError, read_gcovr_summary
from common.file_traversal import find_repo_root
from common.format import format_count, format_percent
from common.gh_actions import gh_error, is_ci, write_step_summary
from common.logging import log_error, log_info, log_ok
from common.markdown import code_span, summary_section
from common.proc import require_tools, run_captured
from qgc_tools.cmake_configure import CMakeConfig, configure

COVERAGE_PRESET = "Linux-coverage"
DEFAULT_BUILD_DIR = Path("build") / COVERAGE_PRESET
# Matches CI: instrumented integration tests run slower; at one per CPU MissionControllerTest times out.
INTEGRATION_JOBS = 2


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(description="Build and report test coverage.")
    parser.add_argument(
        "-r", "--report", action="store_true", help="Generate a report from existing coverage data"
    )
    parser.add_argument(
        "-o", "--open", dest="open_report", action="store_true", help="Open HTML report"
    )
    parser.add_argument("-c", "--clean", action="store_true", help="Clean coverage data")
    parser.add_argument("--log-file", default="", help="Write coverage target output to a file")
    parser.add_argument(
        "--step-summary",
        action="store_true",
        help="Append a coverage summary to GITHUB_STEP_SUMMARY if available",
    )
    parser.add_argument(
        "-b",
        "--build-dir",
        default=str(DEFAULT_BUILD_DIR),
        help=f"Coverage build directory (default: {DEFAULT_BUILD_DIR.as_posix()})",
    )
    return parser.parse_args(argv)


def check_dependencies() -> None:
    """Ensure required tooling is installed."""
    require_tools("gcovr", hint="Install with: python tools/setup/install_python.py coverage")


def clean_coverage(build_dir: Path) -> None:
    """Remove coverage build artifacts."""
    log_info("Cleaning coverage data...")
    shutil.rmtree(build_dir, ignore_errors=True)
    log_ok("Coverage data cleaned")


def configure_build(repo_root: Path, build_dir: Path) -> None:
    """Configure the coverage build directory."""
    cache_path = build_dir / "CMakeCache.txt"
    if cache_path.exists():
        cache_text = cache_path.read_text(encoding="utf-8", errors="ignore")
        if "QGC_ENABLE_COVERAGE:BOOL=ON" in cache_text:
            log_info("Using existing coverage build configuration")
            return

    log_info("Configuring build with coverage...")
    return_code = configure(
        CMakeConfig(source_dir=repo_root, build_dir=build_dir, preset=COVERAGE_PRESET)
    )
    if return_code:
        raise subprocess.CalledProcessError(return_code, ["cmake", "--preset", COVERAGE_PRESET])
    log_ok("Build configured")


def build_project(build_dir: Path) -> None:
    """Build the coverage target."""
    log_info("Building project...")
    subprocess.run(["cmake", "--build", str(build_dir), "--parallel"], check=True, text=True)
    log_ok("Build complete")


def run_tests(repo_root: Path, build_dir: Path) -> None:
    """Run the Unit, then the Integration phase like CI; fail if either phase fails.

    The coverage test preset supplies the excluded labels; each phase selects its label.
    """
    failed: list[str] = []
    for label, jobs in (("Unit", os.cpu_count() or 1), ("Integration", INTEGRATION_JOBS)):
        log_info(f"Running {label} tests...")
        result = subprocess.run(
            [
                "ctest",
                "--preset",
                COVERAGE_PRESET,
                "--test-dir",
                str(build_dir),
                "--timeout",
                "300",
                "--label-regex",
                label,
                "--parallel",
                str(jobs),
            ],
            cwd=repo_root,
            check=False,
            text=True,
        )
        if result.returncode:
            failed.append(label)
    if failed:
        raise RuntimeError(f"{' and '.join(failed)} tests failed")


def build_step_summary(summary_json: Path) -> str:
    """Build the job summary's Coverage section from the gcovr JSON summary."""
    try:
        summary = read_gcovr_summary(summary_json)
    except CoverageSummaryError as exc:
        return summary_section("Coverage", body=f"Coverage data not available: {exc}")

    def metric(covered: int, total: int) -> str:
        if not total:
            return "N/A"
        return f"{format_percent(100.0 * covered / total)} ({format_count(covered)} of {format_count(total)})"

    return summary_section(
        "Coverage",
        [
            ("Lines", metric(summary.line_covered, summary.line_total)),
            (
                "Functions",
                metric(summary.function_covered, summary.function_total)
                if summary.function_total
                else "",
            ),
            ("Branches", metric(summary.branch_covered, summary.branch_total)),
            (
                "Report",
                f"{code_span(summary_json.name)}, `coverage.xml`, and HTML in the `coverage-report` artifact",
            ),
        ],
    )


def generate_report(build_dir: Path, *, log_file: Path | None = None) -> None:
    """Generate XML, JSON summary and HTML coverage reports from existing coverage data.

    Fails when the tests produced no .gcda counters, which would otherwise report zero coverage.
    """
    gcda_count = sum(1 for _ in build_dir.rglob("*.gcda"))
    log_info(f"Detected {gcda_count} .gcda files")
    if not gcda_count:
        raise RuntimeError("No .gcda files were produced by test execution")
    log_info("Generating coverage report...")
    result = run_captured(
        ["cmake", "--build", str(build_dir), "--target", "coverage-report"],
        check=False,
    )
    if result.stdout:
        print(result.stdout, end="")
    if result.stderr:
        print(result.stderr, end="", file=sys.stderr)
    if log_file is not None:
        log_file.write_text((result.stdout or "") + (result.stderr or ""), encoding="utf-8")
    if result.returncode != 0:
        raise subprocess.CalledProcessError(result.returncode, result.args)
    print()
    log_ok("Coverage report generated")
    log_info(f"  XML:  {build_dir / 'coverage.xml'}")
    log_info(f"  JSON: {build_dir / SUMMARY_FILENAME}")
    log_info(f"  HTML: {build_dir / 'coverage.html'}")


def open_report(build_dir: Path) -> None:
    """Open the generated HTML coverage report."""
    report = build_dir / "coverage.html"
    if not report.exists():
        raise FileNotFoundError(f"Report not found: {report}")

    log_info("Opening coverage report...")
    if not webbrowser.open(report.resolve().as_uri()):
        raise RuntimeError(f"Could not find a browser opener. Report at: {report}")


def main(argv: list[str] | None = None) -> int:
    """Run the requested coverage workflow."""
    args = parse_args(argv)
    repo_root = find_repo_root(Path(__file__))
    build_dir = Path(args.build_dir)
    if not build_dir.is_absolute():
        build_dir = repo_root / build_dir
    log_file = Path(args.log_file) if args.log_file else None
    if log_file is not None and not log_file.is_absolute():
        log_file = repo_root / log_file

    try:
        if args.clean:
            clean_coverage(build_dir)
            return 0

        check_dependencies()
        if not args.report:
            configure_build(repo_root, build_dir)
            build_project(build_dir)
            run_tests(repo_root, build_dir)
        generate_report(build_dir, log_file=log_file)

        if args.open_report:
            open_report(build_dir)
        if args.step_summary:
            write_step_summary(build_step_summary(build_dir / SUMMARY_FILENAME))
        return 0
    except (RuntimeError, FileNotFoundError, subprocess.CalledProcessError) as exc:
        (gh_error if is_ci() else log_error)(str(exc))
        return 1


if __name__ == "__main__":
    sys.exit(main())
