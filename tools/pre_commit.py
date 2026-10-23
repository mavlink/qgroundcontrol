#!/usr/bin/env python3
"""Run the .pre-commit-config.yaml hooks with prek, adding CI-friendly summaries."""

from __future__ import annotations

import argparse
import contextlib
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import TYPE_CHECKING

import _bootstrap as _bootstrap
from common.file_traversal import find_repo_root
from common.gh_actions import gh_error, write_github_output
from common.gh_actions import write_step_summary as _write_step_summary
from common.git import pr_diff_refs
from common.io import chdir, write_json
from common.logging import log_error, log_info, log_ok
from common.markdown import code_block, code_span, summary_section
from common.proc import run_captured, run_tee

if TYPE_CHECKING:
    from collections.abc import Sequence

HOOK_RESULT_RE = re.compile(r"\b(Passed|Failed|Skipped)\b")
# "name.......Failed" or "name.....(no files to check)Skipped": the hook name precedes the dots.
HOOK_NAME_RE = re.compile(r"^(?P<name>.*?[^.\s])\.{3,}")
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
# The files of the only hook that needs Qt (qmllint).
QT_PATHSPECS = ("*.qml",)
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
    parser.add_argument(
        "--ci", action="store_true", help="Write the GitHub Actions step summary and annotation"
    )
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
    parser.add_argument(
        "--needs-qt",
        action="store_true",
        help="Only report whether the selected files include QML, which qmllint checks with Qt, "
        "and set the needs-qt step output",
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


def failed_hooks(output: str) -> list[str]:
    """Return the names of the hooks that failed, in run order."""
    names: list[str] = []
    for line in strip_ansi(output).splitlines():
        if ".........." not in line or not line.rstrip().endswith("Failed"):
            continue
        if match := HOOK_NAME_RE.match(line):
            names.append(match.group("name").strip())
    return names


def changed_refs(args: argparse.Namespace) -> tuple[str, str] | None:
    """Return the (from, to) refs for --changed, or None to check every file."""
    if not args.changed:
        return None
    refs = pr_diff_refs(os.environ.get("PR_BASE_SHA"), os.environ.get("PR_HEAD_SHA"))
    if refs:
        log_info(f"Running on files changed vs {refs[0]}...")
    return refs


def _git_paths(*args: str) -> list[str]:
    result = run_captured(["git", *args], check=True)
    return [path for path in result.stdout.split("\0") if path]


def needs_qt(refs: tuple[str, str] | None) -> bool:
    """Return whether the runner's selection includes QML files, which qmllint checks with Qt."""
    if not refs:
        return True
    diff_range = f"{refs[0]}...{refs[1]}"
    return bool(
        _git_paths("diff", "--name-only", "-z", "--diff-filter=d", diff_range, "--", *QT_PATHSPECS)
    )


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


def build_precommit_args(selection: list[str], hooks: Sequence[str] = ()) -> list[str]:
    return [RUNNER, "run", *hooks, "--show-diff-on-failure", "--color=always", *selection]


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
        "failed_hooks": failed_hooks(output),
        "run_url": run_url,
    }
    results_dir.mkdir(parents=True, exist_ok=True)
    write_json(results_dir / RESULTS_FILE, results)
    (results_dir / OUTPUT_FILE).write_text(output, encoding="utf-8")


def write_step_summary(
    exit_code: int,
    passed: int,
    failed: int,
    skipped: int,
    output: str,
    *,
    title: str = "Pre-commit",
    selection: str = "",
) -> None:
    failures = failed_hooks(output)
    body = ""
    if failures:
        body = (
            f"Failed hooks: {', '.join(code_span(name) for name in failures)}. "
            f"Reproduce with {code_span(f'{RUNNER} run <hook> --all-files')} or `just lint`."
        )
    diff = run_captured(["git", "diff", "--stat"])
    _write_step_summary(
        summary_section(
            title,
            [
                ("Result", "passed" if exit_code == 0 else "failed"),
                ("Hooks", f"{passed} passed, {failed} failed, {skipped} skipped"),
                ("Files", selection),
            ],
            body=body,
            collapsed=[
                ("Hook results", code_block("\n".join(extract_hook_lines(output)))),
                ("Files modified by hooks", code_block(diff.stdout) if diff.stdout.strip() else ""),
            ],
        )
    )


def describe_selection(selection: list[str]) -> str:
    """Describe the runner's file selection for the job summary."""
    if "--all-files" in selection:
        return "all files"
    if "--from-ref" in selection:
        return f"changed since {selection[selection.index('--from-ref') + 1][:12]}"
    return f"{len(selection) - 1} selected files"


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

            if args.needs_qt:
                qt = needs_qt(changed_refs(args))
                log_info(f"Qt {'is' if qt else 'is not'} needed for the selected files")
                write_github_output({"needs-qt": str(qt).lower()})
                return 0

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

            hooks = args.hook or (DOCS_HOOKS if args.docs else ())
            log_info("Running pre-commit checks...")
            print()
            with contextlib.ExitStack() as stack:
                # Stream the hooks live; the summary and results are read back from the copy.
                output_dir = results_dir or Path(stack.enter_context(tempfile.TemporaryDirectory()))
                output_file = output_dir / OUTPUT_FILE
                sys.stdout.flush()
                returncode = run_tee(build_precommit_args(selection, hooks), output_file)
                output = output_file.read_text(encoding="utf-8", errors="replace")
            print()

            passed, failed, skipped = summarize_output(output)
            if results_dir:
                write_results(results_dir, returncode, output)

            if returncode == 0:
                log_ok(f"All checks passed ({passed} passed)")
            else:
                log_error(f"Some checks failed ({passed} passed, {failed} failed)")

            if args.ci:
                write_step_summary(
                    returncode,
                    passed,
                    failed,
                    skipped,
                    output,
                    title="Docs lint" if args.docs else "Pre-commit",
                    selection=describe_selection(selection),
                )
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
