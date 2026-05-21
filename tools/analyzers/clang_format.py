"""clang-format analyzer."""

from __future__ import annotations

from typing import TYPE_CHECKING, ClassVar

from common.analyzer import AnalysisResult, AnalyzerBase
from common.logging import log_error, log_info, log_ok
from common.proc import run_captured

if TYPE_CHECKING:
    from pathlib import Path


class ClangFormatAnalyzer(AnalyzerBase):
    """Clang-format code formatter."""

    name: ClassVar[str] = "clang-format"
    install_hint: ClassVar[str] = "Install with: sudo apt install clang-format"

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        if not self.require_tool("clang-format"):
            return AnalysisResult(tool=self.name, passed=False, output="Tool not found")

        version_result = run_captured(["clang-format", "--version"])
        parts = version_result.stdout.split() if version_result.stdout else []
        version_match = parts[2] if len(parts) > 2 else "unknown"
        log_info(f"Using clang-format version {version_match}")

        if not files:
            log_info("No files to check")
            return AnalysisResult(tool=self.name, passed=True)

        log_info(f"{'Formatting' if fix else 'Checking'} {len(files)} files...")

        if fix:
            return self._run_fix(files)
        return self._run_check(files)

    def _run_fix(self, files: list[Path]) -> AnalysisResult:
        formatted = 0
        for file in files:
            result = run_captured(["clang-format", "-i", str(file)])
            if result.returncode == 0:
                formatted += 1

        log_ok(f"Formatted {formatted} files")

        diff_result = run_captured(["git", "-C", str(self.repo_root), "diff", "--quiet"])
        if diff_result.returncode == 0:
            log_info("No formatting changes needed")
        else:
            log_info("Files modified:")
            modified = run_captured(["git", "-C", str(self.repo_root), "diff", "--name-only"])
            print(modified.stdout)

        return AnalysisResult(tool=self.name, passed=True, files_checked=len(files))

    def _run_check(self, files: list[Path]) -> AnalysisResult:
        needs_format: list[str] = []
        for file in files:
            result = run_captured(["clang-format", "--dry-run", "--Werror", str(file)])
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
                files_checked=len(files),
                files_with_issues=needs_format,
            )

        log_ok("All files properly formatted")
        return AnalysisResult(tool=self.name, passed=True, files_checked=len(files))
