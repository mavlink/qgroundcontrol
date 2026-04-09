#!/usr/bin/env python3
"""Generate code coverage reports for QGroundControl."""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

from _bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.logging import log_error, log_info, log_ok

LINE_COVERAGE_RE = re.compile(r"lines:\s*(.+)")
BRANCH_COVERAGE_RE = re.compile(r"branches:\s*(.+)")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(description="Build and report test coverage.")
    parser.add_argument(
        "--mode",
        choices=["full", "report-only"],
        default="full",
        help="Coverage generation mode (default: full)",
    )
    parser.add_argument("-r", "--report", action="store_true", help="Generate report only")
    parser.add_argument("-o", "--open", dest="open_report", action="store_true", help="Open HTML report")
    parser.add_argument("-c", "--clean", action="store_true", help="Clean coverage data")
    parser.add_argument("--xml", action="store_true", help="Generate coverage report and highlight XML output path")
    parser.add_argument("--log-file", default="", help="Write coverage target output to a file")
    parser.add_argument(
        "--step-summary",
        action="store_true",
        help="Append a coverage summary to GITHUB_STEP_SUMMARY if available",
    )
    parser.add_argument(
        "-b",
        "--build-dir",
        default="build-coverage",
        help="Coverage build directory (default: build-coverage)",
    )
    return parser.parse_args(argv)


def run_command(
    cmd: list[str],
    *,
    cwd: Path | None = None,
    capture_output: bool = False,
) -> subprocess.CompletedProcess[str]:
    """Run a subprocess and fail fast on errors."""
    return subprocess.run(cmd, cwd=cwd, check=True, capture_output=capture_output, text=True)


def check_dependencies() -> None:
    """Ensure required tooling is installed."""
    from common.deps import require_tool
    require_tool("gcovr", hint="Install with: pip install gcovr")


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
    run_command(
        [
            "cmake",
            "-B",
            str(build_dir),
            "-S",
            str(repo_root),
            "-DCMAKE_BUILD_TYPE=Debug",
            "-DQGC_ENABLE_COVERAGE=ON",
            "-DQGC_BUILD_TESTING=ON",
            "-G",
            "Ninja",
        ]
    )
    log_ok("Build configured")


def build_project(build_dir: Path) -> None:
    """Build the coverage target."""
    log_info("Building project...")
    run_command(["cmake", "--build", str(build_dir), "--parallel"])
    log_ok("Build complete")


def run_tests(build_dir: Path) -> None:
    """Run tests and fail if any test fails."""
    log_info("Running tests...")
    run_command(["ctest", "--test-dir", str(build_dir), "--output-on-failure", "--timeout", "300"])


def build_step_summary(log_text: str, mode: str) -> str:
    """Build a GitHub Step Summary section from gcovr output."""
    lines = ["## Code Coverage", "", f"Mode: `{mode}`", ""]
    line_match = LINE_COVERAGE_RE.search(log_text)
    branch_match = BRANCH_COVERAGE_RE.search(log_text)
    if line_match:
        lines.extend(
            [
                "| Metric | Coverage |",
                "|--------|----------|",
                f"| Lines | {line_match.group(1).strip()} |",
                f"| Branches | {(branch_match.group(1).strip() if branch_match else 'N/A')} |",
                "",
                "[View detailed report](coverage.html)",
            ]
        )
    else:
        lines.append("Coverage data not available")
    return "\n".join(lines) + "\n"


def maybe_write_step_summary(log_text: str, mode: str) -> None:
    """Append coverage markdown to GITHUB_STEP_SUMMARY when requested."""
    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    if not summary_path:
        return
    with open(summary_path, "a", encoding="utf-8") as handle:
        handle.write(build_step_summary(log_text, mode))


def generate_report(build_dir: Path, *, xml_only: bool, log_file: Path | None = None, mode: str = "full") -> str:
    """Generate coverage artifacts from existing coverage data."""
    log_info("Generating coverage report...")
    result = run_command(
        ["cmake", "--build", str(build_dir), "--target", "coverage-report"],
        capture_output=True,
    )
    if result.stdout:
        print(result.stdout, end="")
    if result.stderr:
        print(result.stderr, end="", file=sys.stderr)
    if log_file is not None:
        log_file.write_text((result.stdout or "") + (result.stderr or ""), encoding="utf-8")
    print()
    log_ok("Coverage report generated")
    log_info(f"  XML:  {build_dir / 'coverage.xml'}")
    if not xml_only:
        log_info(f"  HTML: {build_dir / 'coverage.html'}")
    return (result.stdout or "") + (result.stderr or "")


def open_report(build_dir: Path) -> None:
    """Open the generated HTML coverage report."""
    report = build_dir / "coverage.html"
    if not report.exists():
        raise FileNotFoundError(f"Report not found: {report}")

    log_info("Opening coverage report...")
    opener = shutil.which("xdg-open") or shutil.which("open")
    if opener is None:
        raise RuntimeError(f"Could not find a browser opener. Report at: {report}")
    subprocess.run([opener, str(report)], check=False)


def main(argv: list[str] | None = None) -> int:
    """Run the requested coverage workflow."""
    args = parse_args(argv)
    repo_root = Path(__file__).resolve().parent.parent
    build_dir = Path(args.build_dir)
    mode = "report-only" if args.report else args.mode
    if not build_dir.is_absolute():
        build_dir = repo_root / build_dir
    log_file = Path(args.log_file) if args.log_file else None
    if log_file is not None and not log_file.is_absolute():
        log_file = repo_root / log_file

    try:
        check_dependencies()
        if args.clean:
            clean_coverage(build_dir)
            return 0

        if mode == "report-only":
            report_output = generate_report(build_dir, xml_only=args.xml, log_file=log_file, mode=mode)
        else:
            configure_build(repo_root, build_dir)
            build_project(build_dir)
            run_tests(build_dir)
            report_output = generate_report(build_dir, xml_only=args.xml, log_file=log_file, mode=mode)

        if args.open_report:
            open_report(build_dir)
        if args.step_summary:
            maybe_write_step_summary(report_output, mode)
        return 0
    except (RuntimeError, FileNotFoundError, subprocess.CalledProcessError) as exc:
        log_error(str(exc))
        return 1


if __name__ == "__main__":
    sys.exit(main())
