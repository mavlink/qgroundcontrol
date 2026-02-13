#!/usr/bin/env python3
"""
Auto-apply pre-commit fixes for formatting and style issues.

Usage:
    ./tools/lint_fix.py              # Fix all files
    ./tools/lint_fix.py --changed    # Fix only changed files vs master
    ./tools/lint_fix.py --stage      # Fix and stage changes
    ./tools/lint_fix.py --check      # Check only (don't fix)
    ./tools/lint_fix.py -h           # Show help

Fixable Hooks:
    - clang-format (C++ formatting)
    - ruff-format (Python formatting)
    - pretty-format-json (JSON formatting)
    - cmake-format (CMake files)
    - trailing-whitespace
    - end-of-file-fixer
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import TYPE_CHECKING, ClassVar

from common import find_repo_root, Logger, log_info, log_ok, log_warn, log_error

if TYPE_CHECKING:
    from collections.abc import Sequence




@dataclass
class FixResult:
    """Result of running a single formatter."""

    formatter: str
    files_modified: list[str] = field(default_factory=list)
    success: bool = True
    output: str = ""
    skipped: bool = False


@dataclass
class FormatterConfig:
    """Configuration for a formatter."""

    name: str
    command: str
    args: list[str] = field(default_factory=list)
    file_patterns: list[str] = field(default_factory=list)
    exclude_dirs: list[str] = field(default_factory=lambda: ["build", "libs"])


class LintFixer:
    """Manages running formatters and applying fixes."""

    FIXABLE_HOOKS: ClassVar[list[str]] = [
        "clang-format",
        "ruff-format",
        "pretty-format-json",
        "cmake-format",
        "trailing-whitespace",
        "end-of-file-fixer",
    ]

    def __init__(
        self,
        repo_root: Path,
        *,
        all_files: bool = True,
        stage: bool = False,
        check_only: bool = False,
    ) -> None:
        self.repo_root = repo_root
        self.all_files = all_files
        self.stage = stage
        self.check_only = check_only
        self._modified_before: int = 0
        self._modified_after: int = 0

    def _run_command(
        self,
        cmd: Sequence[str],
        *,
        check: bool = False,
        capture: bool = True,
        cwd: Path | None = None,
    ) -> subprocess.CompletedProcess[str]:
        """Run a command and return the result."""
        if os.environ.get("DEBUG"):
            log_info(f"Running: {' '.join(cmd)}")
        return subprocess.run(
            cmd,
            cwd=cwd or self.repo_root,
            capture_output=capture,
            text=True,
            check=check,
        )

    def _get_default_branch(self) -> str:
        """Get the default branch from remote origin."""
        result = self._run_command(
            ["git", "symbolic-ref", "refs/remotes/origin/HEAD"]
        )
        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.strip().replace("refs/remotes/origin/", "")
        return "master"

    def _get_scope_args(self) -> list[str]:
        """Determine scope arguments for pre-commit."""
        if self.all_files:
            return ["--all-files"]

        default_branch = self._get_default_branch()
        result = self._run_command(["git", "rev-parse", "--verify", default_branch])
        if result.returncode != 0:
            result = self._run_command(
                ["git", "rev-parse", "--verify", f"origin/{default_branch}"]
            )

        if result.returncode == 0:
            log_info(f"Running on files changed vs {default_branch}...")
            return ["--from-ref", default_branch, "--to-ref", "HEAD"]

        log_warn(f"{default_branch} not available, running on all files")
        return ["--all-files"]

    def _check_pre_commit(self) -> bool:
        """Check if pre-commit is available."""
        if not shutil.which("pre-commit"):
            log_error("pre-commit not found")
            log_info("Install with: pip install pre-commit")
            return False
        return True

    def _count_modified_files(self) -> int:
        """Count currently modified files."""
        result = self._run_command(["git", "ls-files", "-m"])
        if result.returncode == 0:
            return len([f for f in result.stdout.strip().split("\n") if f])
        return 0

    def _find_files(self, patterns: list[str], exclude_dirs: list[str]) -> list[Path]:
        """Find files matching patterns, excluding specified directories."""
        files: list[Path] = []
        for pattern in patterns:
            for path in self.repo_root.rglob(pattern):
                if not any(excl in path.parts for excl in exclude_dirs):
                    files.append(path)
        return files

    def _run_ruff_format(self) -> FixResult:
        """Run ruff format on Python files."""
        result = FixResult(formatter="ruff-format")

        if not shutil.which("ruff"):
            if os.environ.get("DEBUG"):
                log_info("ruff not found (skipping)")
            result.skipped = True
            return result

        log_info("Running ruff-format...")
        dirs_to_format = ["tools", "test"]
        existing_dirs = [
            str(self.repo_root / d) for d in dirs_to_format if (self.repo_root / d).exists()
        ]

        if not existing_dirs:
            result.skipped = True
            return result

        cmd = ["ruff", "format", "--quiet", *existing_dirs]
        proc = self._run_command(cmd)
        result.success = proc.returncode == 0
        result.output = proc.stdout + proc.stderr
        if not result.success:
            log_warn("ruff-format exited with error")
        return result

    def _run_clang_format(self) -> FixResult:
        """Run clang-format on C++ files."""
        result = FixResult(formatter="clang-format")

        if not shutil.which("clang-format"):
            if os.environ.get("DEBUG"):
                log_info("clang-format not found (skipping)")
            result.skipped = True
            return result

        log_info("Running clang-format...")
        patterns = ["*.cpp", "*.hpp", "*.h", "*.c"]
        exclude_dirs = ["build", "libs"]
        files = self._find_files(patterns, exclude_dirs)

        if not files:
            result.skipped = True
            return result

        for file_path in files:
            proc = self._run_command(["clang-format", "-i", str(file_path)])
            if proc.returncode != 0:
                result.success = False
                log_warn(f"clang-format failed on {file_path}")

        return result

    def _run_cmake_format(self) -> FixResult:
        """Run cmake-format on CMake files."""
        result = FixResult(formatter="cmake-format")

        if not shutil.which("cmake-format"):
            if os.environ.get("DEBUG"):
                log_info("cmake-format not found (skipping)")
            result.skipped = True
            return result

        log_info("Running cmake-format...")
        patterns = ["CMakeLists.txt", "*.cmake"]
        exclude_dirs = ["build", "libs"]
        files = self._find_files(patterns, exclude_dirs)

        if not files:
            result.skipped = True
            return result

        for file_path in files:
            proc = self._run_command(["cmake-format", "-i", str(file_path)])
            if proc.returncode != 0:
                result.success = False
                log_warn(f"cmake-format failed on {file_path}")

        return result

    def _run_precommit_hooks(self) -> FixResult:
        """Run trailing-whitespace and end-of-file-fixer through pre-commit."""
        result = FixResult(formatter="pre-commit-hooks")

        log_info("Running trailing-whitespace and end-of-file-fixer...")
        scope_args = self._get_scope_args()
        cmd = [
            "pre-commit",
            "run",
            "trailing-whitespace",
            "end-of-file-fixer",
            "--color=always",
            *scope_args,
        ]

        proc = self._run_command(cmd, capture=False)
        result.success = proc.returncode == 0
        return result

    def run_check_mode(self) -> int:
        """Run in check mode (no modifications)."""
        log_info("Running in check mode (no modifications)...")
        scope_args = self._get_scope_args()
        cmd = [
            "pre-commit",
            "run",
            "--show-diff-on-failure",
            "--color=always",
            *scope_args,
        ]
        proc = self._run_command(cmd, capture=False)
        return proc.returncode

    def run_formatters(self) -> list[FixResult]:
        """Run all formatters and return results."""
        results: list[FixResult] = []

        self._modified_before = self._count_modified_files()

        scope_desc = "all files" if self.all_files else "changed files"
        log_info(f"Running auto-fix formatters on {scope_desc}...")
        print()

        results.append(self._run_ruff_format())
        results.append(self._run_clang_format())
        results.append(self._run_cmake_format())
        results.append(self._run_precommit_hooks())

        print()
        self._modified_after = self._count_modified_files()

        return results

    def has_changes(self) -> bool:
        """Check if there are uncommitted changes."""
        result = self._run_command(["git", "diff", "--quiet"])
        return result.returncode != 0

    def show_diff_summary(self) -> None:
        """Show summary of changes made."""
        if not self.has_changes():
            log_ok("No changes needed - all files are properly formatted!")
            return

        log_info("Changes made to:")
        print()
        self._run_command(["git", "diff", "--stat"], capture=False)
        print()

        result = self._run_command(["git", "diff"])
        diff_lines = len(result.stdout.strip().split("\n")) if result.stdout else 0

        if diff_lines > 0:
            log_info("Diff preview (first 50 lines):")
            print()
            diff_cmd = ["git", "diff", "--color=always"]
            proc = subprocess.run(
                diff_cmd,
                cwd=self.repo_root,
                capture_output=True,
                text=True,
                check=False,
            )
            lines = proc.stdout.split("\n")[:50]
            print("\n".join(lines))

            if diff_lines > 50:
                print()
                print(f"... (diff truncated, run 'git diff' to see all {diff_lines} lines)")

        print()

    def stage_changes(self) -> None:
        """Stage all modified files."""
        log_info("Staging modified files...")
        self._run_command(["git", "add", "-u"])
        print()
        log_ok("Changes staged for commit")
        print()
        log_info("Next steps:")
        print("  1. Review staged changes: git diff --cached")
        print("  2. Commit: git commit -m 'style: auto-apply formatting fixes'")

    def show_next_steps(self) -> None:
        """Show next steps when not staging."""
        print()
        log_info("To review and commit these changes:")
        print("  1. Review changes: git diff")
        print("  2. Stage all changes: git add -u")
        print("  3. Commit: git commit -m 'style: auto-apply formatting fixes'")
        print()
        log_info("Or run with --stage flag to auto-stage changes:")
        print("  ./tools/lint_fix.py --stage")


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Auto-apply pre-commit fixes for formatting and style issues",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Fixable Hooks:
  - clang-format       C++ code formatting
  - ruff-format        Python code formatting
  - pretty-format-json JSON formatting
  - cmake-format       CMake file formatting
  - trailing-whitespace Trailing whitespace cleanup
  - end-of-file-fixer  EOF enforcement

Examples:
  # Fix all Python and JSON formatting issues
  ./tools/lint_fix.py

  # Fix only files changed in this branch
  ./tools/lint_fix.py --changed

  # Fix and immediately stage changes
  ./tools/lint_fix.py --stage

  # Just check what would be fixed (don't modify files)
  ./tools/lint_fix.py --check
""",
    )

    scope_group = parser.add_mutually_exclusive_group()
    scope_group.add_argument(
        "-a",
        "--all",
        action="store_true",
        default=True,
        dest="all_files",
        help="Run on all files (default)",
    )
    scope_group.add_argument(
        "-c",
        "--changed",
        action="store_true",
        help="Run only on changed files vs master branch",
    )

    parser.add_argument(
        "-s",
        "--stage",
        action="store_true",
        help="Stage fixed files after running",
    )
    parser.add_argument(
        "-C",
        "--check",
        action="store_true",
        help="Check only, don't fix (same as pre-commit.sh)",
    )

    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    """Main entry point."""
    args = parse_args(argv)

    try:
        repo_root = find_repo_root()
    except RuntimeError as e:
        log_error(str(e))
        return 1

    os.chdir(repo_root)

    fixer = LintFixer(
        repo_root,
        all_files=not args.changed,
        stage=args.stage,
        check_only=args.check,
    )

    if not fixer._check_pre_commit():
        return 1

    if args.check:
        return fixer.run_check_mode()

    fixer.run_formatters()

    if not fixer.has_changes():
        log_ok("No changes needed - all files are properly formatted!")
        return 0

    fixer.show_diff_summary()

    if args.stage:
        fixer.stage_changes()
    else:
        fixer.show_next_steps()

    log_ok("Formatting complete!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
