#!/usr/bin/env python3
"""
Run QGroundControl unit tests.

Usage:
    ./tools/run_tests.py                          # Run all tests
    ./tools/run_tests.py --filter "Vehicle*"      # Filter tests
    ./tools/run_tests.py --timeout 600            # Custom timeout
    ./tools/run_tests.py --xml                    # JUnit XML output
    ./tools/run_tests.py --headless               # Force offscreen mode

Environment:
    QT_QPA_PLATFORM - Qt platform plugin (default: auto-detect)
"""

from __future__ import annotations

import argparse
import os
import platform
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Sequence

from common import find_repo_root, Logger


@dataclass
class TestResult:
    """Result of a test run."""

    passed: int = 0
    failed: int = 0
    skipped: int = 0
    duration: float = 0.0
    output: str = ""
    xml_path: Path | None = None
    log_path: Path | None = None
    exit_code: int = 0


class QtTestRunner:
    """Runs QGroundControl Qt unit tests."""

    BINARY_NAME = "QGroundControl"
    BUILD_TYPES = ("Debug", "Release", "RelWithDebInfo", "MinSizeRel")

    def __init__(
        self,
        build_dir: Path,
        timeout: int = 300,
        verbose: bool = False,
        headless: bool = False,
        verify_only: bool = False,
    ) -> None:
        self.build_dir = build_dir.resolve()
        self.timeout = timeout
        self.verbose = verbose
        self.headless = headless
        self.verify_only = verify_only
        self.repo_root = find_repo_root(Path(__file__).parent)
        self.log = Logger()

    def detect_platform(self) -> str:
        """Detect the current platform."""
        system = platform.system().lower()
        if system == "linux":
            return "linux"
        elif system == "darwin":
            return "macos"
        elif system == "windows" or system.startswith("mingw") or system.startswith("msys"):
            return "windows"
        return "linux"

    def needs_virtual_display(self) -> bool:
        """Check if we need a virtual display (Linux without DISPLAY)."""
        if self.headless:
            return False
        if self.detect_platform() != "linux":
            return False
        return not os.environ.get("DISPLAY")

    def find_binary(self, build_type: str | None = None) -> Path | None:
        """Find the QGroundControl binary in build directory."""
        plat = self.detect_platform()
        binary_name = f"{self.BINARY_NAME}.exe" if plat == "windows" else self.BINARY_NAME

        # Try the shared binary finder helper first.
        find_script = self.repo_root / ".github" / "scripts" / "find_binary.py"
        if find_script.is_file():
            binary = self._run_find_script(find_script, build_type, plat)
            normalized = self._normalize_binary_path(binary) if binary else None
            if normalized:
                return normalized

        # Build search locations
        locations: list[Path] = []

        if build_type:
            locations.append(self.build_dir / build_type / binary_name)

        locations.append(self.build_dir / binary_name)

        for bt in self.BUILD_TYPES:
            locations.append(self.build_dir / bt / binary_name)

        for loc in locations:
            if loc.is_file():
                return loc

        return None

    def _normalize_binary_path(self, path: Path) -> Path | None:
        """Normalize binary path from helpers and platform-specific layouts."""
        if path.is_file():
            return path

        # macOS helper may return .app bundle path.
        if path.is_dir() and path.suffix == ".app":
            app_binary = path / "Contents" / "MacOS" / self.BINARY_NAME
            if app_binary.is_file():
                return app_binary

        return None

    def _run_find_script(self, script: Path, build_type: str | None, platform_name: str) -> Path | None:
        """Run find_binary.py and parse result."""
        args = [
            sys.executable,
            str(script),
            "--build-dir",
            str(self.build_dir),
            "--platform",
            platform_name,
        ]
        if build_type:
            args.extend(["--build-type", build_type])

        try:
            result = subprocess.run(
                args,
                capture_output=True,
                text=True,
                timeout=10,
            )
            for line in result.stdout.splitlines():
                if line.startswith("binary_path="):
                    path = Path(line.split("=", 1)[1])
                    return path
                if line.startswith("Found: "):
                    return Path(line.split(": ", 1)[1])
        except (subprocess.TimeoutExpired, subprocess.SubprocessError):
            pass

        return None

    def run_tests(
        self,
        filter_pattern: str | None = None,
        xml_output: bool = False,
        output_file: Path | None = None,
        log_file: Path | None = None,
        binary_path: Path | None = None,
        build_type: str | None = None,
    ) -> TestResult:
        """Run the unit tests and return results."""
        result = TestResult()

        # Find binary
        binary = binary_path or self.find_binary(build_type)
        if not binary or not binary.is_file():
            self.log.error(f"Binary not found in {self.build_dir}")
            result.exit_code = 1
            return result

        # Make executable on Unix
        if self.detect_platform() != "windows":
            try:
                binary.chmod(binary.stat().st_mode | 0o111)
            except OSError:
                pass

        self.log.info(f"Binary: {binary}")
        self.log.info(f"Timeout: {self.timeout}s")

        # Build test arguments
        test_args: list[str] = []

        if self.verify_only:
            # Boot test mode - just verify the binary starts
            test_args.append("--simple-boot-test")
            self.log.info("Mode: verify-only (boot test)")
        elif filter_pattern:
            test_args.append(f"--unittest:{filter_pattern}")
            self.log.info(f"Filter: {filter_pattern}")
        else:
            test_args.append("--unittest")

        # XML output (not applicable for verify-only)
        if xml_output and not self.verify_only:
            if output_file is None:
                output_file = binary.parent / "junit-results.xml"
            result.xml_path = output_file
            test_args.extend(["--unittest-output", str(output_file)])
            self.log.info(f"Output: {output_file}")

        # Build command
        cmd, extra_env = self._build_command(binary, test_args)

        # Set environment
        env = os.environ.copy()
        env.update(extra_env)
        if self.headless:
            env.setdefault("QT_QPA_PLATFORM", "offscreen")
            self.log.info("Platform: offscreen (forced)")

        # Run tests
        start_time = time.monotonic()

        try:
            proc = subprocess.run(
                cmd,
                env=env,
                capture_output=True,
                text=True,
                timeout=self.timeout,
            )
            result.exit_code = proc.returncode
            result.output = proc.stdout + proc.stderr

            if self.verbose:
                print(result.output)

        except subprocess.TimeoutExpired as e:
            self.log.error(f"Tests timed out after {self.timeout}s")
            result.exit_code = 124
            result.output = (e.stdout or b"").decode() + (e.stderr or b"").decode()

        result.duration = time.monotonic() - start_time

        # Write log file if requested
        if log_file:
            try:
                log_file.parent.mkdir(parents=True, exist_ok=True)
                log_file.write_text(result.output)
                result.log_path = log_file
                self.log.info(f"Log: {log_file}")
            except OSError as e:
                self.log.warn(f"Failed to write log file: {e}")

        # Log result
        if result.exit_code == 0:
            self.log.ok("Tests passed")
        else:
            self.log.error(f"Tests failed (exit code: {result.exit_code})")

        return result

    def _build_command(self, binary: Path, test_args: list[str]) -> tuple[list[str], dict[str, str]]:
        """Build the command to run tests, handling virtual display if needed.

        Returns (command, extra_env) tuple.
        """
        base_cmd = [str(binary)] + test_args
        extra_env: dict[str, str] = {}

        if self.needs_virtual_display():
            xvfb = shutil.which("xvfb-run")
            if xvfb:
                self.log.info("Running with xvfb-run (no DISPLAY)")
                return [xvfb, "-a"] + base_cmd, extra_env
            else:
                self.log.warn("xvfb-run not found, using offscreen platform")
                extra_env["QT_QPA_PLATFORM"] = "offscreen"

        return base_cmd, extra_env

    def write_github_output(self, result: TestResult) -> None:
        """Write results to GITHUB_OUTPUT for CI integration."""
        github_output = os.environ.get("GITHUB_OUTPUT")
        if not github_output:
            return

        try:
            with open(github_output, "a") as f:
                f.write(f"exit_code={result.exit_code}\n")
                f.write(f"passed={'true' if result.exit_code == 0 else 'false'}\n")
                if result.xml_path:
                    f.write(f"output_file={result.xml_path}\n")
                if result.log_path:
                    f.write(f"log_file={result.log_path}\n")
        except OSError as e:
            self.log.warn(f"Failed to write GITHUB_OUTPUT: {e}")


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Run QGroundControl unit tests.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                              Run all tests
  %(prog)s --filter "MAVLink*"          Filter tests
  %(prog)s -B build-debug --xml         Debug build with XML output
""",
    )

    parser.add_argument(
        "-B",
        "--build-dir",
        type=Path,
        default=Path("build"),
        help="Build directory (default: build)",
    )
    parser.add_argument(
        "-t",
        "--build-type",
        help="Build type to find binary in (auto-detect)",
    )
    parser.add_argument(
        "--binary",
        type=Path,
        help="Explicit path to QGroundControl binary",
    )
    parser.add_argument(
        "--filter",
        dest="filter_pattern",
        help="Test filter pattern",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=300,
        help="Timeout in seconds (default: 300)",
    )
    parser.add_argument(
        "--xml",
        action="store_true",
        help="Generate JUnit XML output",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Output file for XML results",
    )
    parser.add_argument(
        "--log-file",
        type=Path,
        help="Write test output to log file",
    )
    parser.add_argument(
        "--headless",
        action="store_true",
        help="Force headless/offscreen mode",
    )
    parser.add_argument(
        "--verify-only",
        action="store_true",
        help="Run boot test only (--simple-boot-test), skip unit tests",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Show test output",
    )

    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    """Main entry point."""
    args = parse_args(argv)

    runner = QtTestRunner(
        build_dir=args.build_dir,
        timeout=args.timeout,
        verbose=args.verbose,
        headless=args.headless,
        verify_only=args.verify_only,
    )

    result = runner.run_tests(
        filter_pattern=args.filter_pattern,
        xml_output=args.xml,
        output_file=args.output,
        log_file=args.log_file,
        binary_path=args.binary,
        build_type=args.build_type,
    )

    runner.write_github_output(result)

    return result.exit_code


if __name__ == "__main__":
    sys.exit(main())
