#!/usr/bin/env python3
"""
Run Qt Test benchmarks and parse output to JSON.

Usage:
    benchmark_runner.py --binary PATH [--filter FILTER] [--output FILE]

Outputs (for GitHub Actions):
    has_results, result_count
"""

import argparse
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path


def run_benchmarks(binary: Path, test_filter: str, platform: str) -> str:
    """Run Qt Test benchmarks and return output."""
    if not binary.exists():
        print(f"::error::Binary not found: {binary}", file=sys.stderr)
        sys.exit(1)

    try:
        binary.chmod(binary.stat().st_mode | 0o111)
    except OSError:
        pass

    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = platform

    cmd = [str(binary), f"--unittest:{test_filter}"]
    print(f"Running: {' '.join(cmd)}")

    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=300,
            env=env,
        )
        return result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        print("::warning::Benchmark timed out after 5 minutes")
        return ""
    except OSError as e:
        print(f"::warning::Benchmark failed: {e}")
        return ""


def parse_benchmark_output(output: str) -> list[dict]:
    """Parse Qt Test benchmark output to structured results."""
    results = []

    for line in output.splitlines():
        # Match: RESULT : TestClass::testFunc(): X.XXX msecs per iteration
        match = re.search(
            r"RESULT\s*:\s*\w+::(\w+)\(\):\s*([\d.]+)\s*(\w+)\s*per iteration",
            line,
        )
        if match:
            name = match.group(1)
            value = float(match.group(2))
            unit = match.group(3)

            # Normalize to microseconds for consistency
            if unit == "msecs":
                value *= 1000
                unit = "usecs"
            elif unit == "secs":
                value *= 1000000
                unit = "usecs"
            elif unit == "nsecs":
                value /= 1000
                unit = "usecs"

            results.append({"name": name, "unit": unit, "value": value})

    return results


def fallback_startup_benchmark(binary: Path, platform: str) -> list[dict]:
    """Measure startup time as fallback benchmark."""
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = platform

    try:
        start = time.monotonic()
        subprocess.run(
            [str(binary), "--unittest", "--list-tests"],
            capture_output=True,
            timeout=30,
            env=env,
        )
        elapsed_ms = (time.monotonic() - start) * 1000
        return [{"name": "Startup (list tests)", "unit": "usecs", "value": elapsed_ms * 1000}]
    except (subprocess.TimeoutExpired, OSError) as e:
        print(f"Fallback benchmark failed: {e}")
        return []


def write_github_output(results: list[dict]) -> None:
    """Write outputs for GitHub Actions."""
    github_output = os.environ.get("GITHUB_OUTPUT")
    if github_output:
        with open(github_output, "a") as f:
            f.write(f"has_results={'true' if results else 'false'}\n")
            f.write(f"result_count={len(results)}\n")


def write_summary(results: list[dict], output_file: Path) -> None:
    """Write GitHub step summary."""
    summary_file = os.environ.get("GITHUB_STEP_SUMMARY")
    if not summary_file:
        return

    with open(summary_file, "a") as f:
        f.write("## Runtime Benchmarks\n\n")
        f.write(
            "⚠️ **Note**: Runtime benchmarks on shared runners have ~3x variance.\n"
        )
        f.write("Only significant regressions (>50%) trigger alerts.\n\n")
        f.write("### Results\n\n")

        if results and output_file.exists():
            f.write("```json\n")
            f.write(output_file.read_text())
            f.write("\n```\n")
        else:
            f.write("No benchmark results available\n")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run Qt Test benchmarks and parse output to JSON"
    )
    parser.add_argument(
        "--binary",
        type=Path,
        required=True,
        help="Path to the QGC binary",
    )
    parser.add_argument(
        "--filter",
        default="MAVLinkBenchmark",
        help="Test filter (default: MAVLinkBenchmark)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("benchmark.json"),
        help="Path for benchmark JSON output",
    )
    parser.add_argument(
        "--platform",
        default="offscreen",
        help="Qt platform plugin (default: offscreen)",
    )
    parser.add_argument(
        "--save-raw",
        type=Path,
        help="Save raw benchmark output to file",
    )

    args = parser.parse_args()

    # Run benchmarks
    output = run_benchmarks(args.binary, args.filter, args.platform)

    # Save raw output if requested
    if args.save_raw:
        args.save_raw.write_text(output)
        print(f"Raw output saved to: {args.save_raw}")

    # Parse results
    results = parse_benchmark_output(output)

    # Fallback to startup benchmark if no results
    if not results:
        print("No benchmark results found, using startup time fallback")
        results = fallback_startup_benchmark(args.binary, args.platform)

    # Write JSON output
    args.output.write_text(json.dumps(results, indent=2))
    print(f"Parsed {len(results)} benchmark results → {args.output}")

    # GitHub Actions integration
    write_github_output(results)
    write_summary(results, args.output)

    return 0


if __name__ == "__main__":
    sys.exit(main())
