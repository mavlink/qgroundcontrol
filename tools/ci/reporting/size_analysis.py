#!/usr/bin/env python3
"""
Analyze binary size using bloaty and standard tools.

Usage:
    size_analysis.py --binary PATH [--install-bloaty]
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.build_config import get_build_config_value
from common.format import format_bytes, format_count
from common.gh_actions import gh_warning, write_step_summary
from common.markdown import code_block, code_span, summary_section
from common.paths import TOOLS_DIR
from common.proc import run_captured

APT_INSTALL_RETRY = TOOLS_DIR / "ci" / "runner" / "apt_install_retry.py"
BLOATY_BUILD_TIMEOUT_SECONDS = 120


def install_bloaty() -> bool:
    """Ensure bloaty is on PATH, building a pinned commit from source (Ubuntu doesn't package it)."""
    if shutil.which("bloaty"):
        return True

    print(f"Building bloaty from source (timeout: {BLOATY_BUILD_TIMEOUT_SECONDS}s)...")
    try:
        run_captured(
            [
                sys.executable,
                str(APT_INSTALL_RETRY),
                "--no-install-recommends",
                "--install-timeout",
                "300",
                "--label",
                "bloaty build dependencies",
                "libprotobuf-dev",
                "protobuf-compiler",
                "libre2-dev",
                "libcapstone-dev",
            ],
            check=True,
        )
        with tempfile.TemporaryDirectory(prefix="bloaty-") as bloaty_dir:
            run_captured(["git", "init", bloaty_dir], timeout=10, check=True)
            run_captured(
                [
                    "git",
                    "-C",
                    bloaty_dir,
                    "fetch",
                    "--depth",
                    "1",
                    "https://github.com/google/bloaty.git",
                    get_build_config_value("tools.bloaty.revision"),
                ],
                timeout=30,
                check=True,
            )
            run_captured(
                ["git", "-C", bloaty_dir, "checkout", "FETCH_HEAD"],
                timeout=10,
                check=True,
            )
            run_captured(
                [
                    "cmake",
                    "-B",
                    f"{bloaty_dir}/build",
                    "-S",
                    bloaty_dir,
                    "-DCMAKE_BUILD_TYPE=Release",
                    "-DBLOATY_ENABLE_RE2=ON",
                ],
                timeout=60,
                check=True,
            )
            run_captured(
                ["cmake", "--build", f"{bloaty_dir}/build", "--parallel"],
                timeout=BLOATY_BUILD_TIMEOUT_SECONDS,
                check=True,
            )
            run_captured(
                ["sudo", "cmake", "--install", f"{bloaty_dir}/build"],
                timeout=30,
                check=True,
            )
        print("bloaty installed from source")
        return True
    except subprocess.TimeoutExpired:
        gh_warning("bloaty installation timed out, size analysis will be limited")
        return False
    except subprocess.CalledProcessError as e:
        gh_warning(f"bloaty installation failed: {e}")
        return False


class BinaryAnalyzer:
    """Analyzes binary size and symbol information."""

    def __init__(self, binary_path: Path) -> None:
        if not binary_path.exists():
            raise FileNotFoundError(f"Binary not found: {binary_path}")
        self.binary_path = binary_path.resolve()

    def get_binary_size(self) -> int:
        """Return the size of the binary in bytes."""
        return self.binary_path.stat().st_size

    def get_stripped_size(self) -> int:
        """Return the size of the stripped binary in bytes."""
        with tempfile.NamedTemporaryFile(delete=False) as tmp:
            tmp_path = Path(tmp.name)
        try:
            shutil.copy2(self.binary_path, tmp_path)
            run_captured(["strip", str(tmp_path)], check=True)
            return tmp_path.stat().st_size
        finally:
            tmp_path.unlink(missing_ok=True)

    def get_symbol_count(self) -> int:
        """Return the number of symbols in the binary."""
        try:
            result = run_captured(["nm", str(self.binary_path)])
            if result.returncode != 0:
                return 0
            return len(result.stdout.strip().splitlines())
        except (subprocess.SubprocessError, FileNotFoundError):
            return 0

    def get_section_sizes(self) -> str:
        """Return section sizes using the size command."""
        try:
            result = run_captured(["size", "-A", str(self.binary_path)], check=True)
            return result.stdout
        except (subprocess.SubprocessError, FileNotFoundError):
            return "Section sizes unavailable"

    def run_bloaty(self, analysis_type: str, top_n: int = 20) -> str:
        """
        Run bloaty analysis on the binary.

        Args:
            analysis_type: 'symbols' or 'compileunits'
            top_n: Number of top entries to show

        Returns:
            Bloaty output or error message.
        """
        if not shutil.which("bloaty"):
            return "Bloaty not available"

        try:
            result = run_captured(
                ["bloaty", "-d", analysis_type, "-n", str(top_n), str(self.binary_path)],
            )
            if result.returncode != 0:
                return "Bloaty analysis skipped"
            return result.stdout
        except (subprocess.SubprocessError, FileNotFoundError):
            return "Bloaty analysis skipped"

    def generate_summary(
        self,
        binary_size: int,
        stripped_size: int,
        symbol_count: int,
    ) -> str:
        """Generate the job summary's Binary size section."""
        collapsed = [("Section sizes", code_block(self.get_section_sizes()))]
        if shutil.which("bloaty"):
            collapsed += [
                ("Largest symbols (top 20)", code_block(self.run_bloaty("symbols", 20))),
                (
                    "Size by compilation unit (top 20)",
                    code_block(self.run_bloaty("compileunits", 20)),
                ),
            ]
        return summary_section(
            "Binary size",
            [
                ("Binary", code_span(self.binary_path.name)),
                ("Size", f"{format_bytes(binary_size)} ({format_count(binary_size)} bytes)"),
                (
                    "Stripped",
                    f"{format_bytes(stripped_size)} ({format_count(stripped_size)} bytes)",
                ),
                ("Symbols", format_count(symbol_count)),
            ],
            collapsed=collapsed,
        )


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Analyze binary size using bloaty and standard tools",
    )
    parser.add_argument(
        "--binary",
        type=Path,
        required=True,
        help="Path to binary to analyze",
    )
    parser.add_argument(
        "--install-bloaty",
        action="store_true",
        help="Install bloaty from source",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Main entry point."""
    args = parse_args(argv)

    if args.install_bloaty:
        install_bloaty()

    try:
        analyzer = BinaryAnalyzer(args.binary)
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    print(f"Analyzing: {args.binary}")

    binary_size = analyzer.get_binary_size()
    print(f"Binary size: {binary_size} bytes")

    stripped_size = analyzer.get_stripped_size()
    print(f"Stripped size: {stripped_size} bytes")

    symbol_count = analyzer.get_symbol_count()
    print(f"Symbol count: {symbol_count}")

    if os.environ.get("GITHUB_STEP_SUMMARY"):
        summary = analyzer.generate_summary(binary_size, stripped_size, symbol_count)
        write_step_summary(summary)

    return 0


if __name__ == "__main__":
    sys.exit(main())
