#!/usr/bin/env python3
"""Run pre-commit checks with CI-friendly summaries."""

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

from common.gh_actions import write_github_output as _write_github_output, write_step_summary as _write_step_summary
from common.logging import log_error, log_info, log_ok, log_warn

HOOK_RESULT_RE = re.compile(r"\b(Passed|Failed|Skipped)\b")
ANSI_ESCAPE_RE = re.compile(r"\x1b\[[0-9;]*m")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run pre-commit checks.")
    parser.add_argument("-c", "--changed", action="store_true", help="Run only files changed vs master")
    parser.add_argument("-i", "--install", action="store_true", help="Install pre-commit hooks")
    parser.add_argument("-u", "--update", action="store_true", help="Update hook versions")
    parser.add_argument("--ci", action="store_true", help="Enable GitHub Actions outputs")
    parser.add_argument("-o", "--output", default="", help="Write raw output to file")
    return parser.parse_args(argv)


def repo_root() -> Path:
    return Path(__file__).resolve().parent.parent


def ensure_precommit_available() -> bool:
    return shutil.which("pre-commit") is not None


def strip_ansi(value: str) -> str:
    return ANSI_ESCAPE_RE.sub("", value)


def summarize_output(output: str) -> tuple[int, int, int]:
    passed = failed = skipped = 0
    for line in output.splitlines():
        match = HOOK_RESULT_RE.search(line)
        if not match:
            continue
        state = match.group(1)
        if state == "Passed":
            passed += 1
        elif state == "Failed":
            failed += 1
        elif state == "Skipped":
            skipped += 1
    return passed, failed, skipped


def extract_hook_lines(output: str, *, limit: int = 40) -> list[str]:
    lines = [strip_ansi(line) for line in output.splitlines() if ".........." in line]
    return lines[:limit] or ["No results"]


def git_default_branch_ref() -> str | None:
    """Find the default branch ref, trying remote HEAD then common names."""
    result = subprocess.run(
        ["git", "symbolic-ref", "refs/remotes/origin/HEAD", "--short"],
        capture_output=True, text=True, check=False,
    )
    if result.returncode == 0:
        return result.stdout.strip().removeprefix("origin/")

    for ref in ("master", "main", "origin/master", "origin/main"):
        if subprocess.run(
            ["git", "rev-parse", "--verify", ref],
            capture_output=True, check=False,
        ).returncode == 0:
            return ref
    return None


def build_precommit_args(args: argparse.Namespace) -> list[str]:
    result = ["pre-commit", "run", "--show-diff-on-failure", "--color=always"]
    if args.changed:
        ref = git_default_branch_ref()
        if ref:
            log_info(f"Running on files changed vs {ref}...")
            result.extend(["--from-ref", ref, "--to-ref", "HEAD"])
        else:
            log_warn("Default branch not available, running on all files")
            result.append("--all-files")
    else:
        result.append("--all-files")
    return result


def run_command(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, capture_output=True, text=True, check=False)


def write_github_output(exit_code: int, passed: int, failed: int, skipped: int, summary_lines: list[str]) -> None:
    _write_github_output({
        "exit_code": str(exit_code),
        "passed": str(passed),
        "failed": str(failed),
        "skipped": str(skipped),
        "summary": "\n".join(summary_lines),
    })


def write_step_summary(exit_code: int, passed: int, failed: int, skipped: int, output: str) -> None:
    parts = ["## Pre-commit Results\n"]
    parts.append("**All checks passed**\n" if exit_code == 0 else "**Some checks failed**\n")
    parts.append("| Status | Count |\n|--------|-------|")
    parts.append(f"| Passed | {passed} |")
    parts.append(f"| Failed | {failed} |")
    if skipped > 0:
        parts.append(f"| Skipped | {skipped} |")
    hook_lines = "\n".join(extract_hook_lines(output))
    parts.append(f"\n<details>\n<summary>Hook Results</summary>\n\n```\n{hook_lines}\n```\n</details>")

    diff = subprocess.run(["git", "diff", "--stat"], capture_output=True, text=True, check=False)
    if diff.stdout.strip():
        parts.append(f"\n<details>\n<summary>Files Modified by Hooks</summary>\n\n```\n{diff.stdout}```\n</details>")

    _write_step_summary("\n".join(parts) + "\n")


def handle_install() -> int:
    log_info("Installing pre-commit and hooks...")
    from common import pip_install
    pip_install(["pre-commit"])
    subprocess.run(["pre-commit", "install"], check=True)
    subprocess.run(["pre-commit", "install", "--hook-type", "commit-msg"], check=True)
    log_ok("Pre-commit hooks installed")
    return 0


def handle_update() -> int:
    log_info("Updating pre-commit hooks...")
    subprocess.run(["pre-commit", "autoupdate"], check=True)
    log_ok("Hooks updated. Review changes in .pre-commit-config.yaml")
    return 0


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    os.chdir(repo_root())

    try:
        if args.install:
            return handle_install()
        if args.update:
            if not ensure_precommit_available():
                log_error("pre-commit not found")
                return 1
            return handle_update()

        if not ensure_precommit_available():
            log_error("pre-commit not found")
            log_info("Install with: pip install pre-commit")
            log_info("Or run: python3 ./tools/pre_commit.py --install")
            return 1

        command = build_precommit_args(args)
        log_info("Running pre-commit checks...")
        print()
        result = run_command(command)
        if result.stdout:
            print(result.stdout, end="")
        if result.stderr:
            print(result.stderr, end="", file=sys.stderr)
        print()

        passed, failed, skipped = summarize_output(result.stdout + result.stderr)
        if args.output or os.environ.get("PRE_COMMIT_OUTPUT"):
            output_path = Path(args.output or os.environ["PRE_COMMIT_OUTPUT"])
            output_path.write_text(result.stdout + result.stderr, encoding="utf-8")
            log_info(f"Output written to: {output_path}")

        if result.returncode == 0:
            log_ok(f"All checks passed ({passed} passed)")
        else:
            log_error(f"Some checks failed ({passed} passed, {failed} failed)")

        if args.ci:
            summary_lines = extract_hook_lines(result.stdout + result.stderr)
            write_github_output(result.returncode, passed, failed, skipped, summary_lines)
            write_step_summary(result.returncode, passed, failed, skipped, result.stdout + result.stderr)

        if result.returncode != 0:
            print()
            log_info("To fix issues locally:")
            print("  1. Run: pre-commit run --all-files")
            print("  2. Review and stage changes: git add -u")
            print("  3. Amend your commit: git commit --amend --no-edit")

        return result.returncode
    except subprocess.CalledProcessError as exc:
        log_error(str(exc))
        return exc.returncode or 1


if __name__ == "__main__":
    sys.exit(main())
