"""clang-tidy analyzer."""

from __future__ import annotations

from typing import TYPE_CHECKING, ClassVar

from common.analyzer import AnalysisResult, AnalyzerBase, FileAnalysis
from common.logging import log_info
from common.proc import run_captured

if TYPE_CHECKING:
    from pathlib import Path


class ClangTidyAnalyzer(AnalyzerBase):
    """Clang-tidy static analyzer."""

    name: ClassVar[str] = "clang-tidy"
    install_hint: ClassVar[str] = "Install with: sudo apt install clang-tidy"

    def _analyze_file(self, file: Path) -> FileAnalysis:
        rel_path = self.relative_path(file)
        result = run_captured(["clang-tidy", "-p", str(self.build_dir), str(file)])
        return FileAnalysis(path=rel_path, has_issues=result.returncode != 0)

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        if not self.require_compile_commands():
            return AnalysisResult(
                tool=self.name,
                passed=False,
                output="compile_commands.json not found",
            )

        if not self.require_tool("clang-tidy"):
            return AnalysisResult(tool=self.name, passed=False, output="Tool not found")

        if not files:
            log_info("No files to analyze")
            return AnalysisResult(tool=self.name, passed=True)

        log_info(f"Running clang-tidy on {len(files)} files ({self.jobs} jobs)...")

        results = self.run_files_parallel(files, self._analyze_file, jobs=self.jobs)
        files_with_issues = [result.path for result in results if result.has_issues]
        for result in results:
            status = "ISSUES" if result.has_issues else "OK"
            print(f"  {result.path}... {status}")

        return AnalysisResult(
            tool=self.name,
            passed=not files_with_issues,
            issues=len(files_with_issues),
            files_checked=len(files),
            files_with_issues=files_with_issues,
        )
