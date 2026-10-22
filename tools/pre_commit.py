#!/usr/bin/env python3
"""Run the .pre-commit-config.yaml hooks with prek, adding CI-friendly summaries."""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

import _bootstrap as _bootstrap
from common.file_traversal import find_repo_root
from common.gh_actions import gh_error
from common.gh_actions import write_github_output as _write_github_output
from common.gh_actions import write_step_summary as _write_step_summary
from common.git import get_default_branch_ref
from common.io import chdir
from common.logging import log_error, log_info, log_ok, log_warn
from common.proc import run_captured

HOOK_RESULT_RE = re.compile(r"\b(Passed|Failed|Skipped)\b")
ANSI_ESCAPE_RE = re.compile(r"\x1b\[[0-9;]*m")

DOCS_HOOKS = ("markdownlint", "typos", "vale")
DOCS_PATHSPECS = ("docs/en/*.md", "docs/en/**/*.md")
# Changing these rechecks every English page, so new rules apply to unchanged pages too.
DOCS_FULL_SCAN_PATHSPECS = (
    ".pre-commit-config.yaml",
    ".vale.ini",
    ".markdownlint*",
    ".typos.toml",
    ".github/workflows/docs-lint.yml",
    "tools/pre_commit.py",
)
# Runs .pre-commit-config.yaml; the config stays compatible with stock pre-commit.
RUNNER = "prek"
RESULTS_FILE = "pre-commit-results.json"
OUTPUT_FILE = "pre-commit-output.txt"


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run pre-commit checks.")
    parser.add_argument(
        "-c", "--changed", action="store_true", help="Run only files changed vs master"
    )
    parser.add_argument(
        "-i", "--install", action="store_true", help="Install the pre-commit and commit-msg hooks"
    )
    parser.add_argument("--ci", action="store_true", help="Enable GitHub Actions outputs")
    parser.add_argument(
        "--hook", action="append", default=[], metavar="ID", help="Run only this hook (repeatable)"
    )
    parser.add_argument(
        "--docs",
        action="store_true",
        help="Check English documentation pages with the Docs Lint hooks; with --changed, "
        "lint configuration changes select every page",
    )
    parser.add_argument(
        "--results-dir",
        type=Path,
        help=f"Write {RESULTS_FILE} and {OUTPUT_FILE} for the CI results artifact",
    )
    return parser.parse_args(argv)


def repo_root() -> Path:
    return find_repo_root(Path(__file__))


def ensure_precommit_available() -> bool:
    return shutil.which(RUNNER) is not None


def strip_ansi(value: str) -> str:
    return ANSI_ESCAPE_RE.sub("", value)


def summarize_output(output: str) -> tuple[int, int, int]:
    passed = failed = skipped = 0
    for line in strip_ansi(output).splitlines():
        if ".........." not in line:
            continue
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


def changed_refs(args: argparse.Namespace) -> tuple[str, str] | None:
    """Return the (from, to) refs for --changed, or None to check every file."""
    if not args.changed:
        return None
    ref = os.environ.get("PR_BASE_SHA") or get_default_branch_ref()
    if ref == "0" * 40:
        ref = None
    if not ref:
        log_warn("Default branch not available, running on all files")
        return None
    head = os.environ.get("PR_HEAD_SHA") or "HEAD"
    if merge_base_tip := _merge_checkout_base(head):
        # The checkout is GitHub's merge of the PR head into the current base tip, which can be newer than
        # PR_BASE_SHA; diffing against that tip keeps newer base-branch changes out of the checked regions.
        ref, head = merge_base_tip, "HEAD"
    log_info(f"Running on files changed vs {ref}...")
    return ref, head


def _merge_checkout_base(head: str) -> str | None:
    """Return HEAD's first parent when HEAD merges *head* into it (a pull_request checkout), else None."""
    if head == "HEAD":
        return None
    parents = run_captured(["git", "rev-list", "--parents", "-n", "1", "HEAD"]).stdout.split()
    resolved = run_captured(["git", "rev-parse", "--verify", f"{head}^{{commit}}"]).stdout.strip()
    if len(parents) == 3 and resolved and parents[2] == resolved:
        return parents[1]
    return None


def _git_paths(*args: str) -> list[str]:
    result = run_captured(["git", *args], check=True)
    return [path for path in result.stdout.split("\0") if path]


def select_docs_files(refs: tuple[str, str] | None) -> list[str]:
    """Return English pages changed between refs; every page without refs or after config edits."""
    if refs:
        # Same merge-base range as the runner's --from-ref/--to-ref.
        diff_range = f"{refs[0]}...{refs[1]}"
        config = run_captured(
            ["git", "diff", "--quiet", diff_range, "--", *DOCS_FULL_SCAN_PATHSPECS]
        )
        if config.returncode == 0:
            return _git_paths(
                "diff", "--name-only", "-z", "--diff-filter=ACMR", diff_range, "--", *DOCS_PATHSPECS
            )
        log_info("Documentation lint configuration changed; checking every page")
    return _git_paths("ls-files", "-z", "--", *DOCS_PATHSPECS)


def file_selection_args(args: argparse.Namespace) -> list[str] | None:
    """Return the runner's file-selection arguments, or None when no file is selected."""
    refs = changed_refs(args)
    if args.docs:
        files = select_docs_files(refs)
        return ["--files", *files] if files else None
    if refs:
        return ["--from-ref", refs[0], "--to-ref", refs[1]]
    return ["--all-files"]


def build_precommit_args(selection: list[str], hook: str | None = None) -> list[str]:
    hook_args = [hook] if hook else []
    return [RUNNER, "run", *hook_args, "--show-diff-on-failure", "--color=always", *selection]


def write_results(results_dir: Path, exit_code: int, output: str) -> None:
    """Write the artifact that the build-results PR comment reads."""
    passed, failed, skipped = summarize_output(output)
    server, repository, run_id = (
        os.environ.get(name, "")
        for name in ("GITHUB_SERVER_URL", "GITHUB_REPOSITORY", "GITHUB_RUN_ID")
    )
    run_url = (
        f"{server}/{repository}/actions/runs/{run_id}" if server and repository and run_id else ""
    )
    results = {
        "exit_code": str(exit_code),
        "passed": str(passed),
        "failed": str(failed),
        "skipped": str(skipped),
        "run_url": run_url,
    }
    results_dir.mkdir(parents=True, exist_ok=True)
    (results_dir / RESULTS_FILE).write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
    (results_dir / OUTPUT_FILE).write_text(output, encoding="utf-8")


def write_github_output(
    exit_code: int, passed: int, failed: int, skipped: int, summary_lines: list[str]
) -> None:
    _write_github_output(
        {
            "exit_code": str(exit_code),
            "passed": str(passed),
            "failed": str(failed),
            "skipped": str(skipped),
            "summary": "\n".join(summary_lines),
        }
    )


def write_step_summary(exit_code: int, passed: int, failed: int, skipped: int, output: str) -> None:
    parts = ["## Pre-commit Results\n"]
    parts.append("**All checks passed**\n" if exit_code == 0 else "**Some checks failed**\n")
    parts.append("| Status | Count |\n|--------|-------|")
    parts.append(f"| Passed | {passed} |")
    parts.append(f"| Failed | {failed} |")
    if skipped > 0:
        parts.append(f"| Skipped | {skipped} |")
    hook_lines = "\n".join(extract_hook_lines(output))
    parts.append(
        f"\n<details>\n<summary>Hook Results</summary>\n\n```\n{hook_lines}\n```\n</details>"
    )

    diff = run_captured(["git", "diff", "--stat"])
    if diff.stdout.strip():
        parts.append(
            f"\n<details>\n<summary>Files Modified by Hooks</summary>\n\n```\n{diff.stdout}```\n</details>"
        )

    _write_step_summary("\n".join(parts) + "\n")


def handle_install() -> int:
    log_info(f"Installing {RUNNER} and Git hooks...")
    from qgc_tools.python_env import tool_command

    command = tool_command(RUNNER, "precommit")
    subprocess.run([*command, "install"], check=True)
    log_ok("Git hooks installed")
    return 0


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    results_dir = args.results_dir.resolve() if args.results_dir else None

    with chdir(repo_root()):
        try:
            if args.install:
                return handle_install()

            if results_dir:
                # Records a failure unless the run completes.
                write_results(results_dir, 1, "")

            if not ensure_precommit_available():
                log_error(f"{RUNNER} not found")
                log_info("Install with: python tools/setup/install_python.py precommit")
                log_info("Or run: python3 ./tools/pre_commit.py --install")
                return 1

            selection = file_selection_args(args)
            if selection is None:
                log_ok("No changed files match the selection")
                if results_dir:
                    write_results(results_dir, 0, "")
                return 0

            hooks: list[str | None] = list(args.hook or (DOCS_HOOKS if args.docs else [None]))
            log_info("Running pre-commit checks...")
            print()
            returncode = 0
            output = ""
            for hook in hooks:
                result = run_captured(build_precommit_args(selection, hook))
                if result.stdout:
                    print(result.stdout, end="")
                if result.stderr:
                    print(result.stderr, end="", file=sys.stderr)
                output += result.stdout + result.stderr
                returncode = returncode or result.returncode
            print()

            passed, failed, skipped = summarize_output(output)
            if results_dir:
                write_results(results_dir, returncode, output)

            if returncode == 0:
                log_ok(f"All checks passed ({passed} passed)")
            else:
                log_error(f"Some checks failed ({passed} passed, {failed} failed)")

            if args.ci:
                summary_lines = extract_hook_lines(output)
                write_github_output(returncode, passed, failed, skipped, summary_lines)
                write_step_summary(returncode, passed, failed, skipped, output)
                if returncode != 0:
                    gh_error("Required pre-commit checks failed; see the job summary.")

            if returncode != 0:
                print()
                log_info("To fix issues locally:")
                print(f"  1. Run: {RUNNER} run --all-files")
                print("  2. Review and stage changes: git add -u")
                print("  3. Amend your commit: git commit --amend --no-edit")

            return returncode
        except subprocess.CalledProcessError as exc:
            log_error(str(exc))
            return exc.returncode or 1


if __name__ == "__main__":
    sys.exit(main())
