#!/usr/bin/env python3
"""
Generate code coverage reports for QGroundControl.

Usage:
    ./tools/coverage.py                      # Build with coverage and run tests
    ./tools/coverage.py --report             # Generate report only (after tests)
    ./tools/coverage.py --open               # Generate and open report in browser
    ./tools/coverage.py --clean              # Clean coverage data
    ./tools/coverage.py --xml                # Generate XML only (for CI tools)
    ./tools/coverage.py --threshold 80       # Fail if coverage below 80%

Requirements:
    - gcovr (pip install gcovr)
    - cmake, ninja

This script wraps CMake coverage targets defined in cmake/modules/Coverage.cmake
"""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
import webbrowser
from dataclasses import dataclass, field
from pathlib import Path

from common import Logger


@dataclass
class CoverageResult:
    """Results from coverage analysis."""

    line_coverage: float = 0.0
    branch_coverage: float = 0.0
    html_path: Path | None = None
    xml_path: Path | None = None
    success: bool = True
    error_message: str = ""


@dataclass
class CoverageRunner:
    """Manages code coverage workflow for QGroundControl."""

    build_dir: Path
    output_dir: Path
    repo_root: Path = field(default_factory=lambda: Path(__file__).parent.parent.resolve())
    logger: Logger = field(default_factory=Logger)

    def __post_init__(self) -> None:
        self.build_dir = self.build_dir.resolve()
        self.output_dir = self.output_dir.resolve()

    def check_dependencies(self) -> bool:
        """Verify required tools are available."""
        if not shutil.which("gcovr"):
            self.logger.error("gcovr not found")
            self.logger.info("Install with: pip install gcovr")
            return False

        if not shutil.which("cmake"):
            self.logger.error("cmake not found")
            return False

        if not shutil.which("ninja"):
            self.logger.error("ninja not found")
            return False

        return True

    def configure_build(self) -> bool:
        """Configure CMake build with coverage flags."""
        cmake_cache = self.build_dir / "CMakeCache.txt"

        if cmake_cache.exists():
            cache_content = cmake_cache.read_text()
            if "QGC_ENABLE_COVERAGE:BOOL=ON" in cache_content:
                self.logger.info("Using existing coverage build configuration")
                return True

        self.logger.info("Configuring build with coverage...")

        result = subprocess.run(
            [
                "cmake",
                "-B", str(self.build_dir),
                "-S", str(self.repo_root),
                "-DCMAKE_BUILD_TYPE=Debug",
                "-DQGC_ENABLE_COVERAGE=ON",
                "-DQGC_BUILD_TESTING=ON",
                "-G", "Ninja",
            ],
            cwd=self.repo_root,
            capture_output=False,
        )

        if result.returncode != 0:
            self.logger.error("CMake configuration failed")
            return False

        self.logger.ok("Build configured")
        return True

    def build_project(self) -> bool:
        """Build the project."""
        self.logger.info("Building project...")

        result = subprocess.run(
            ["cmake", "--build", str(self.build_dir), "--parallel"],
            cwd=self.repo_root,
            capture_output=False,
        )

        if result.returncode != 0:
            self.logger.error("Build failed")
            return False

        self.logger.ok("Build complete")
        return True

    def run_tests(self) -> bool:
        """Run tests via ctest."""
        self.logger.info("Running tests...")

        result = subprocess.run(
            [
                "ctest",
                "--test-dir", str(self.build_dir),
                "--output-on-failure",
                "--timeout", "300",
            ],
            cwd=self.repo_root,
            capture_output=False,
        )

        # Continue even if some tests fail - we still want coverage
        self.logger.ok("Tests complete")
        return True

    def generate_report(self, *, html: bool = True, xml: bool = True) -> CoverageResult:
        """Generate coverage report(s)."""
        self.logger.info("Generating coverage report...")

        result = CoverageResult()

        if xml and not html:
            # XML only (for CI tools)
            xml_path = self.build_dir / "coverage.xml"
            proc = subprocess.run(
                [
                    "gcovr",
                    "-r", str(self.repo_root),
                    "-o", str(xml_path),
                    "--xml-pretty",
                    "--filter=src/",
                ],
                cwd=self.repo_root,
                capture_output=True,
                text=True,
            )

            if proc.returncode != 0:
                result.success = False
                result.error_message = proc.stderr
                self.logger.error(f"gcovr failed: {proc.stderr}")
                return result

            result.xml_path = xml_path

            # Parse coverage from XML
            result = self._parse_xml_coverage(xml_path, result)

        else:
            # Use CMake target for full report (HTML + XML)
            proc = subprocess.run(
                ["cmake", "--build", str(self.build_dir), "--target", "coverage-report"],
                cwd=self.repo_root,
                capture_output=True,
                text=True,
            )

            if proc.returncode != 0:
                result.success = False
                result.error_message = proc.stderr
                self.logger.error(f"Coverage report generation failed: {proc.stderr}")
                return result

            html_path = self.build_dir / "coverage.html"
            xml_path = self.build_dir / "coverage.xml"

            if html and html_path.exists():
                result.html_path = html_path

            if xml_path.exists():
                result.xml_path = xml_path
                result = self._parse_xml_coverage(xml_path, result)

        print()
        self.logger.ok("Coverage report generated:")
        if result.html_path:
            self.logger.info(f"  HTML: {result.html_path}")
        if result.xml_path:
            self.logger.info(f"  XML:  {result.xml_path}")

        if result.line_coverage > 0:
            self.logger.info(f"  Line coverage:   {result.line_coverage:.1f}%")
        if result.branch_coverage > 0:
            self.logger.info(f"  Branch coverage: {result.branch_coverage:.1f}%")

        return result

    def _parse_xml_coverage(self, xml_path: Path, result: CoverageResult) -> CoverageResult:
        """Parse coverage percentages from XML report."""
        try:
            content = xml_path.read_text()

            # Parse line-rate and branch-rate from coverage element
            line_match = re.search(r'line-rate="([0-9.]+)"', content)
            branch_match = re.search(r'branch-rate="([0-9.]+)"', content)

            if line_match:
                result.line_coverage = float(line_match.group(1)) * 100
            if branch_match:
                result.branch_coverage = float(branch_match.group(1)) * 100

        except Exception as e:
            self.logger.warn(f"Could not parse coverage XML: {e}")

        return result

    def open_report(self) -> bool:
        """Open HTML report in browser."""
        report_path = self.build_dir / "coverage.html"

        if not report_path.exists():
            self.logger.error("Report not found. Run coverage first.")
            return False

        self.logger.info("Opening coverage report...")

        try:
            webbrowser.open(f"file://{report_path}")
            return True
        except Exception as e:
            self.logger.warn(f"Could not open browser: {e}")
            self.logger.info(f"Report at: {report_path}")
            return False

    def clean(self) -> None:
        """Clean coverage data."""
        self.logger.info("Cleaning coverage data...")

        if self.build_dir.exists():
            shutil.rmtree(self.build_dir)

        self.logger.ok("Coverage data cleaned")

    def check_threshold(self, result: CoverageResult, threshold: float) -> bool:
        """Check if coverage meets threshold."""
        if result.line_coverage < threshold:
            self.logger.error(
                f"Line coverage {result.line_coverage:.1f}% is below threshold {threshold:.1f}%"
            )
            return False

        self.logger.ok(f"Coverage {result.line_coverage:.1f}% meets threshold {threshold:.1f}%")
        return True


def parse_args() -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Generate code coverage reports for QGroundControl",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                      Build with coverage and run tests
  %(prog)s --report             Generate report only (after tests)
  %(prog)s --open               Generate and open report in browser
  %(prog)s --clean              Clean coverage data
  %(prog)s --xml                Generate XML only (for CI tools)
  %(prog)s --threshold 80       Fail if coverage below 80%%
""",
    )

    repo_root = Path(__file__).parent.parent.resolve()
    default_build_dir = repo_root / "build-coverage"

    parser.add_argument(
        "-b", "--build-dir",
        type=Path,
        default=default_build_dir,
        help=f"Build directory (default: {default_build_dir})",
    )

    parser.add_argument(
        "-o", "--output",
        type=Path,
        default=None,
        help="Output directory for reports (default: same as build-dir)",
    )

    parser.add_argument(
        "-r", "--report",
        action="store_true",
        help="Generate report only (skip build and tests)",
    )

    parser.add_argument(
        "--open",
        action="store_true",
        help="Open HTML report in browser after generation",
    )

    parser.add_argument(
        "-c", "--clean",
        action="store_true",
        help="Clean coverage data and exit",
    )

    parser.add_argument(
        "--xml",
        action="store_true",
        help="Generate XML report only (for CI tools)",
    )

    parser.add_argument(
        "--html",
        action="store_true",
        default=True,
        help="Generate HTML report (default: true)",
    )

    parser.add_argument(
        "-t", "--threshold",
        type=float,
        default=None,
        help="Minimum coverage threshold (exit with error if not met)",
    )

    parser.add_argument(
        "--no-color",
        action="store_true",
        help="Disable colored output",
    )

    return parser.parse_args()


def main() -> int:
    """Main entry point."""
    args = parse_args()

    logger = Logger(color=not args.no_color)

    output_dir = args.output or args.build_dir

    runner = CoverageRunner(
        build_dir=args.build_dir,
        output_dir=output_dir,
        logger=logger,
    )

    # Check dependencies first
    if not runner.check_dependencies():
        return 1

    # Handle clean
    if args.clean:
        runner.clean()
        return 0

    # Handle report-only mode
    if args.report:
        result = runner.generate_report(html=not args.xml, xml=True)
        if not result.success:
            return 1

        if args.open:
            runner.open_report()

        if args.threshold is not None:
            if not runner.check_threshold(result, args.threshold):
                return 1

        return 0

    # Full coverage workflow
    if not runner.configure_build():
        return 1

    if not runner.build_project():
        return 1

    runner.run_tests()

    result = runner.generate_report(html=not args.xml, xml=True)
    if not result.success:
        return 1

    if args.open:
        runner.open_report()

    if args.threshold is not None:
        if not runner.check_threshold(result, args.threshold):
            return 1

    logger.ok("Coverage complete!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
