#!/usr/bin/env python3
"""
Analyze binary size using bloaty and standard tools.

Usage:
    size_analysis.py --binary PATH [--output FILE] [--install-bloaty]

Outputs (for GitHub Actions):
    binary_size, stripped_size, symbol_count
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


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
            subprocess.run(
                ["strip", str(tmp_path)],
                check=True,
                capture_output=True,
            )
            return tmp_path.stat().st_size
        finally:
            tmp_path.unlink(missing_ok=True)

    def get_symbol_count(self) -> int:
        """Return the number of symbols in the binary."""
        try:
            result = subprocess.run(
                ["nm", str(self.binary_path)],
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                return 0
            return len(result.stdout.strip().splitlines())
        except (subprocess.SubprocessError, FileNotFoundError):
            return 0

    def get_section_sizes(self) -> str:
        """Return section sizes using the size command."""
        try:
            result = subprocess.run(
                ["size", "-A", str(self.binary_path)],
                capture_output=True,
                text=True,
                check=True,
            )
            return result.stdout
        except (subprocess.SubprocessError, FileNotFoundError):
            return "Section sizes unavailable"

    @staticmethod
    def install_bloaty(timeout: int = 120) -> bool:
        """
        Install bloaty from source with timeout.

        Returns True if bloaty is available after installation attempt.
        """
        if shutil.which("bloaty"):
            return True

        print(f"Installing bloaty (timeout: {timeout}s)...")

        install_script = """
set -e
sudo apt-get update
sudo apt-get install -y libprotobuf-dev protobuf-compiler libre2-dev libcapstone-dev || true
git clone --depth 1 https://github.com/google/bloaty.git /tmp/bloaty
cd /tmp/bloaty
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBLOATY_ENABLE_RE2=ON
cmake --build build --parallel
sudo cmake --install build
"""
        try:
            subprocess.run(
                ["bash", "-c", install_script],
                timeout=timeout,
                check=True,
                capture_output=True,
            )
            print("bloaty installed successfully")
            return True
        except subprocess.TimeoutExpired:
            print(
                "::warning::bloaty installation timed out, size analysis will be limited",
                file=sys.stderr,
            )
            return False
        except subprocess.CalledProcessError as e:
            print(f"::warning::bloaty installation failed: {e}", file=sys.stderr)
            return False

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
            result = subprocess.run(
                ["bloaty", "-d", analysis_type, "-n", str(top_n), str(self.binary_path)],
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                return "Bloaty analysis skipped"
            return result.stdout
        except (subprocess.SubprocessError, FileNotFoundError):
            return "Bloaty analysis skipped"

    def generate_metrics_json(self) -> list[dict[str, Any]]:
        """Generate metrics in the expected JSON format."""
        return [
            {
                "name": "Binary Size",
                "unit": "bytes",
                "value": self.get_binary_size(),
            },
            {
                "name": "Stripped Size",
                "unit": "bytes",
                "value": self.get_stripped_size(),
            },
            {
                "name": "Symbol Count",
                "unit": "symbols",
                "value": self.get_symbol_count(),
            },
        ]

    def generate_summary(self) -> str:
        """Generate GitHub step summary in Markdown format."""
        binary_size = self.get_binary_size()
        stripped_size = self.get_stripped_size()
        symbol_count = self.get_symbol_count()

        binary_mb = binary_size / 1048576
        stripped_mb = stripped_size / 1048576

        lines: list[str] = []

        lines.append("=== Section sizes ===")
        lines.append("```")
        lines.append(self.get_section_sizes())
        lines.append("```")

        if shutil.which("bloaty"):
            lines.append("")
            lines.append("=== Top 20 largest symbols ===")
            lines.append("```")
            lines.append(self.run_bloaty("symbols", 20))
            lines.append("```")

            lines.append("")
            lines.append("=== Size by compilation unit ===")
            lines.append("```")
            lines.append(self.run_bloaty("compileunits", 20))
            lines.append("```")

        lines.append("")
        lines.append("## Size Metrics")
        lines.append("")
        lines.append("| Metric | Value |")
        lines.append("|--------|-------|")
        lines.append(f"| Binary Size | {binary_mb:.2f} MB ({binary_size} bytes) |")
        lines.append(f"| Stripped Size | {stripped_mb:.2f} MB ({stripped_size} bytes) |")
        lines.append(f"| Symbol Count | {symbol_count} |")

        return "\n".join(lines)


def write_github_output(binary_size: int, stripped_size: int, symbol_count: int) -> None:
    """Write outputs to GITHUB_OUTPUT file if available."""
    github_output = os.environ.get("GITHUB_OUTPUT")
    if github_output:
        with open(github_output, "a") as f:
            f.write(f"binary_size={binary_size}\n")
            f.write(f"stripped_size={stripped_size}\n")
            f.write(f"symbol_count={symbol_count}\n")


def write_github_step_summary(summary: str) -> None:
    """Write summary to GITHUB_STEP_SUMMARY file if available."""
    step_summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if step_summary:
        with open(step_summary, "a") as f:
            f.write(summary)


def parse_args() -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Analyze binary size using bloaty and standard tools",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Outputs (GITHUB_OUTPUT):
  binary_size    - Size of binary in bytes
  stripped_size  - Size of stripped binary in bytes
  symbol_count   - Number of symbols
""",
    )
    parser.add_argument(
        "--binary",
        type=Path,
        required=True,
        help="Path to binary to analyze",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("size-metrics.json"),
        help="Output JSON file (default: size-metrics.json)",
    )
    parser.add_argument(
        "--install-bloaty",
        action="store_true",
        help="Install bloaty from source",
    )
    parser.add_argument(
        "--bloaty-timeout",
        type=int,
        default=120,
        help="Timeout for bloaty installation in seconds (default: 120)",
    )
    return parser.parse_args()


def main() -> int:
    """Main entry point."""
    args = parse_args()

    if args.install_bloaty:
        BinaryAnalyzer.install_bloaty(args.bloaty_timeout)

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

    write_github_output(binary_size, stripped_size, symbol_count)

    metrics = analyzer.generate_metrics_json()
    args.output.write_text(json.dumps(metrics, indent=2) + "\n")
    print(f"Metrics written to: {args.output}")

    if os.environ.get("GITHUB_STEP_SUMMARY"):
        summary = analyzer.generate_summary()
        write_github_step_summary(summary)

    return 0


if __name__ == "__main__":
    sys.exit(main())
