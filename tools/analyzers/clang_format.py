"""clang-format analyzer and changed-region pre-commit hook.

Usage:
    python3 tools/analyzers/clang_format.py [--fix] FILE...

The hook checks (or with --fix, formats) only the lines that differ from the PR merge base
(PRE_COMMIT_FROM_REF/PRE_COMMIT_TO_REF) or, locally, from HEAD. New files, explicitly selected
unchanged files and the tools/coding-style examples are checked in full; deletion-only changes are skipped.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path
from typing import ClassVar

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import _bootstrap as _bootstrap
from common.git import changed_ranges, diff_base, run_git
from common.logging import log_error, log_info, log_ok
from common.proc import run_captured
from common.tool_version import probe_version, uv_lock_version

from analyzers.base import AnalysisResult, AnalyzerBase

# Contributors copy these worked examples, so any change to one checks or formats the whole file.
FULL_FILE_DIRS = ("tools/coding-style/",)


def _clang_format_command(file: Path, lines: list[str], *, fix: bool) -> list[str]:
    mode = ["-i"] if fix else ["--dry-run", "--Werror"]
    return ["clang-format", *mode, *lines, "--", str(file)]


class ClangFormatAnalyzer(AnalyzerBase):
    """Clang-format code formatter."""

    name: ClassVar[str] = "clang-format"
    install_hint: ClassVar[str] = "Install with: python tools/setup/install_python.py lint"
    # Limits formatting to lines changed since this commit; None formats whole files.
    diff_base: str | None = None

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        if not self.require_tool("clang-format"):
            return AnalysisResult(
                tool=self.name, passed=False, execution_error=True, output="Tool not found"
            )

        version = probe_version("clang-format")
        expected = uv_lock_version("clang-format")
        if version is None or ".".join(map(str, version)) != expected:
            return AnalysisResult(
                tool=self.name,
                passed=False,
                execution_error=True,
                output=f"clang-format {expected} required; activate tools/.venv or install the lint group",
            )
        version_str = ".".join(map(str, version)) if version else "unknown"
        log_info(f"Using clang-format version {version_str}")

        try:
            targets = self._line_ranges(files)
        except (OSError, subprocess.CalledProcessError) as error:
            log_error(f"Unable to determine changed lines: {error}")
            return AnalysisResult(
                tool=self.name, passed=False, execution_error=True, output=str(error)
            )

        if not targets:
            log_info("No files to check")
            return AnalysisResult(tool=self.name, passed=True)

        log_info(f"{'Formatting' if fix else 'Checking'} {len(targets)} files...")

        if fix:
            return self._run_fix(targets)
        return self._run_check(targets)

    def _line_ranges(self, files: list[Path]) -> dict[Path, list[str]]:
        """Map each file to its clang-format --lines arguments, dropping deletion-only changes."""
        targets: dict[Path, list[str]] = {}
        for file in files:
            if Path(self.relative_path(file)).as_posix().startswith(FULL_FILE_DIRS):
                targets[file] = []
                continue
            ranges = changed_ranges(file, self.diff_base, cwd=self.repo_root)
            if ranges != []:
                targets[file] = [f"--lines={start}:{end}" for start, end in ranges or []]
        return targets

    def _run_fix(self, targets: dict[Path, list[str]]) -> AnalysisResult:
        formatted = 0
        failed: list[str] = []
        for file, lines in targets.items():
            result = run_captured(_clang_format_command(file, lines, fix=True))
            if result.returncode == 0:
                formatted += 1
            else:
                failed.append(self.relative_path(file))
                log_error(result.stderr or f"clang-format failed: {file}")

        log_ok(f"Formatted {formatted} files")

        diff_result = run_git("diff", "--quiet", cwd=self.repo_root)
        if diff_result.returncode == 0:
            log_info("No formatting changes needed")
        else:
            log_info("Files modified:")
            modified = run_git("diff", "--name-only", cwd=self.repo_root)
            print(modified.stdout)

        return AnalysisResult(
            tool=self.name,
            passed=not failed,
            execution_error=bool(failed),
            files_checked=len(targets),
            files_with_issues=failed,
        )

    def _run_check(self, targets: dict[Path, list[str]]) -> AnalysisResult:
        needs_format: list[str] = []
        for file, lines in targets.items():
            result = run_captured(_clang_format_command(file, lines, fix=False))
            if result.returncode != 0:
                needs_format.append(self.relative_path(file))

        if needs_format:
            log_error("The following files need formatting:")
            for f in needs_format:
                print(f"  {f}")
            print()
            log_info("Run: ./tools/analyze.py --tool clang-format --fix")
            return AnalysisResult(
                tool=self.name,
                passed=False,
                issues=len(needs_format),
                files_checked=len(targets),
                files_with_issues=needs_format,
            )

        log_ok("All files properly formatted")
        return AnalysisResult(tool=self.name, passed=True, files_checked=len(targets))


def main(argv: list[str] | None = None) -> int:
    """Pre-commit entry point; clang-format's own diagnostics are passed through."""
    parser = argparse.ArgumentParser(
        description="Check C++ formatting without rewriting untouched regions of existing files."
    )
    parser.add_argument("--fix", action="store_true", help="Format the checked regions in place")
    parser.add_argument("files", nargs="+", type=Path)
    args = parser.parse_args(argv)
    analyzer = ClangFormatAnalyzer(Path.cwd(), Path.cwd())
    failed = False
    try:
        analyzer.diff_base = diff_base(
            os.environ.get("PRE_COMMIT_FROM_REF"), os.environ.get("PRE_COMMIT_TO_REF", "HEAD")
        )
        for file, lines in analyzer._line_ranges(args.files).items():
            command = _clang_format_command(file, lines, fix=args.fix)
            failed |= subprocess.run(command, check=False).returncode != 0
    except (OSError, subprocess.CalledProcessError) as error:
        print(f"clang-format check failed: {error}", file=sys.stderr)
        return 1
    return int(failed)


if __name__ == "__main__":
    sys.exit(main())
