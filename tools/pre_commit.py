#!/usr/bin/env python3
"""
Run pre-commit checks with formatted output.

Usage:
    ./tools/pre_commit.py              # Run on all files
    ./tools/pre_commit.py --changed    # Run only on changed files (vs master)
    ./tools/pre_commit.py --install    # Install pre-commit hooks
    ./tools/pre_commit.py --update     # Update hook versions
    ./tools/pre_commit.py --ci         # CI mode (machine-readable output)

Environment:
    PRE_COMMIT_OUTPUT  - File to write output (default: stdout)
    GITHUB_STEP_SUMMARY - GitHub Actions step summary file
    GITHUB_OUTPUT - GitHub Actions output file
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path

from common import find_repo_root, log_info, log_ok, log_warn, log_error


@dataclass
class PreCommitResult:
    """Result of a pre-commit run."""

    passed: int = 0
    failed: int = 0
    skipped: int = 0
    output: str = ""
    exit_code: int = 0
    modified_files: list[str] = field(default_factory=list)


def check_pre_commit() -> bool:
    """Check if pre-commit is installed."""
    return shutil.which("pre-commit") is not None


def run_install() -> int:
    """Install pre-commit and hooks."""
    log_info("Installing pre-commit and hooks...")

    try:
        subprocess.run(["pip", "install", "pre-commit"], check=True)
        subprocess.run(["pre-commit", "install"], check=True)
        subprocess.run(["pre-commit", "install", "--hook-type", "commit-msg"], check=True)
        log_ok("Pre-commit hooks installed")
        return 0
    except subprocess.CalledProcessError as e:
        log_error(f"Installation failed: {e}")
        return 1


def run_update() -> int:
    """Update pre-commit hooks."""
    log_info("Updating pre-commit hooks...")

    try:
        subprocess.run(["pre-commit", "autoupdate"], check=True)
        log_ok("Hooks updated. Review changes in .pre-commit-config.yaml")
        return 0
    except subprocess.CalledProcessError as e:
        log_error(f"Update failed: {e}")
        return 1


def has_master_branch() -> bool:
    """Check if master branch is available."""
    try:
        subprocess.run(
            ["git", "rev-parse", "--verify", "master"],
            capture_output=True,
            check=True,
        )
        return True
    except subprocess.CalledProcessError:
        pass

    try:
        subprocess.run(
            ["git", "rev-parse", "--verify", "origin/master"],
            capture_output=True,
            check=True,
        )
        return True
    except subprocess.CalledProcessError:
        return False


def get_modified_files() -> list[str]:
    """Get list of files modified by hooks."""
    try:
        result = subprocess.run(
            ["git", "diff", "--name-only"],
            capture_output=True,
            text=True,
            check=True,
        )
        return [f for f in result.stdout.strip().split("\n") if f]
    except subprocess.CalledProcessError:
        return []


def get_diff_stat() -> str:
    """Get git diff --stat output."""
    try:
        result = subprocess.run(
            ["git", "diff", "--stat"],
            capture_output=True,
            text=True,
            check=True,
        )
        return result.stdout
    except subprocess.CalledProcessError:
        return ""


def parse_output(output: str) -> tuple[int, int, int]:
    """
    Parse pre-commit output to count passed, failed, and skipped hooks.

    Returns:
        Tuple of (passed, failed, skipped) counts.
    """
    passed = len(re.findall(r"Passed", output))
    failed = len(re.findall(r"Failed", output))
    skipped = len(re.findall(r"Skipped", output))
    return passed, failed, skipped


def strip_ansi(text: str) -> str:
    """Remove ANSI escape codes from text."""
    ansi_escape = re.compile(r"\x1b\[[0-9;]*m")
    return ansi_escape.sub("", text)


def extract_hook_results(output: str, max_lines: int = 40) -> str:
    """Extract hook result lines from output."""
    lines = []
    for line in output.split("\n"):
        if ".........." in line:
            lines.append(strip_ansi(line))
            if len(lines) >= max_lines:
                break
    return "\n".join(lines) if lines else "No results"


def run_pre_commit(changed_only: bool = False) -> PreCommitResult:
    """
    Run pre-commit checks.

    Args:
        changed_only: If True, only check files changed vs master.

    Returns:
        PreCommitResult with check results.
    """
    args = ["pre-commit", "run", "--show-diff-on-failure", "--color=always"]

    if changed_only:
        if has_master_branch():
            log_info("Running on files changed vs master...")
            args.extend(["--from-ref", "master", "--to-ref", "HEAD"])
        else:
            log_warn("master branch not available, running on all files")
            args.append("--all-files")
    else:
        args.append("--all-files")

    log_info("Running pre-commit checks...")
    print()

    result = subprocess.run(args, capture_output=True, text=True)
    output = result.stdout + result.stderr

    # Print output in real-time style
    print(output, end="")
    print()

    passed, failed, skipped = parse_output(output)
    modified_files = get_modified_files()

    return PreCommitResult(
        passed=passed,
        failed=failed,
        skipped=skipped,
        output=output,
        exit_code=result.returncode,
        modified_files=modified_files,
    )


def write_github_output(result: PreCommitResult) -> None:
    """Write results to GITHUB_OUTPUT file."""
    github_output = os.environ.get("GITHUB_OUTPUT")
    if not github_output:
        return

    delimiter = f"PRECOMMIT_SUMMARY_{int(time.time())}"
    hook_results = extract_hook_results(result.output)

    with open(github_output, "a") as f:
        f.write(f"exit_code={result.exit_code}\n")
        f.write(f"passed={result.passed}\n")
        f.write(f"failed={result.failed}\n")
        f.write(f"skipped={result.skipped}\n")
        f.write(f"summary<<{delimiter}\n")
        f.write(f"{hook_results}\n")
        f.write(f"{delimiter}\n")


def write_github_summary(result: PreCommitResult) -> None:
    """Write results to GITHUB_STEP_SUMMARY file."""
    github_summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if not github_summary:
        return

    status = "**All checks passed**" if result.exit_code == 0 else "**Some checks failed**"
    hook_results = extract_hook_results(result.output)

    lines = [
        "## Pre-commit Results",
        "",
        status,
        "",
        "| Status | Count |",
        "|--------|-------|",
        f"| Passed | {result.passed} |",
        f"| Failed | {result.failed} |",
    ]

    if result.skipped > 0:
        lines.append(f"| Skipped | {result.skipped} |")

    lines.extend(
        [
            "",
            "<details>",
            "<summary>Hook Results</summary>",
            "",
            "```",
            hook_results,
            "```",
            "</details>",
        ]
    )

    if result.modified_files:
        diff_stat = get_diff_stat()
        lines.extend(
            [
                "",
                "<details>",
                "<summary>Files Modified by Hooks</summary>",
                "",
                "```",
                diff_stat.strip() if diff_stat else "No diff available",
                "```",
                "</details>",
            ]
        )

    with open(github_summary, "a") as f:
        f.write("\n".join(lines))
        f.write("\n")


def show_fix_instructions() -> None:
    """Print instructions for fixing issues locally."""
    print()
    log_info("To fix issues locally:")
    print("  1. Run: pre-commit run --all-files")
    print("  2. Review and stage changes: git add -u")
    print("  3. Amend your commit: git commit --amend --no-edit")


def parse_args() -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Run pre-commit checks with formatted output",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Environment variables:
  PRE_COMMIT_OUTPUT   - File to write output (default: stdout)
  GITHUB_STEP_SUMMARY - GitHub Actions step summary file
  GITHUB_OUTPUT       - GitHub Actions output file
""",
    )

    mode_group = parser.add_mutually_exclusive_group()
    mode_group.add_argument(
        "-a",
        "--all",
        action="store_true",
        default=True,
        help="Run on all files (default)",
    )
    mode_group.add_argument(
        "-c",
        "--changed",
        action="store_true",
        help="Run only on files changed vs master",
    )
    mode_group.add_argument(
        "-i",
        "--install",
        action="store_true",
        help="Install pre-commit hooks",
    )
    mode_group.add_argument(
        "-u",
        "--update",
        action="store_true",
        help="Update hook versions",
    )

    parser.add_argument(
        "--ci",
        action="store_true",
        help="CI mode (write to GITHUB_OUTPUT and GITHUB_STEP_SUMMARY)",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="File to write output",
    )

    return parser.parse_args()


def main() -> int:
    """Main entry point."""
    args = parse_args()

    # Change to repo root
    repo_root = find_repo_root()
    os.chdir(repo_root)

    # Handle install mode (doesn't require pre-commit to be installed)
    if args.install:
        return run_install()

    # Check for pre-commit
    if not check_pre_commit():
        log_error("pre-commit not found")
        log_info("Install with: pip install pre-commit")
        log_info("Or run: ./tools/pre_commit.py --install")
        return 1

    # Handle update mode
    if args.update:
        return run_update()

    # Run pre-commit checks
    result = run_pre_commit(changed_only=args.changed)

    # Print summary
    if result.exit_code == 0:
        log_ok(f"All checks passed ({result.passed} passed)")
    else:
        log_error(f"Some checks failed ({result.passed} passed, {result.failed} failed)")

    # Write to output file if specified
    output_file = args.output or os.environ.get("PRE_COMMIT_OUTPUT")
    if output_file:
        output_path = Path(output_file)
        output_path.write_text(result.output)
        log_info(f"Output written to: {output_path}")

    # CI mode: write structured output
    if args.ci:
        write_github_output(result)
        write_github_summary(result)

    # Show fix instructions on failure
    if result.exit_code != 0:
        show_fix_instructions()

    return result.exit_code


if __name__ == "__main__":
    sys.exit(main())
