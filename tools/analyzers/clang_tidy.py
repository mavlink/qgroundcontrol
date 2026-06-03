"""clang-tidy analyzer."""

from __future__ import annotations

from concurrent.futures import ThreadPoolExecutor, as_completed
from typing import TYPE_CHECKING, ClassVar

from common.analyzer import AnalysisResult, AnalyzerBase
from common.logging import log_info
from common.proc import run_captured

if TYPE_CHECKING:
    from pathlib import Path


class ClangTidyAnalyzer(AnalyzerBase):
    """Clang-tidy static analyzer."""

    name: ClassVar[str] = "clang-tidy"
    install_hint: ClassVar[str] = "Install with: sudo apt install clang-tidy"

    def __init__(self, repo_root: Path, build_dir: Path, jobs: int = 1) -> None:
        super().__init__(repo_root, build_dir)
        self.jobs = jobs

    def _analyze_file(self, file: Path) -> tuple[str, bool]:
        rel_path = self.relative_path(file)
        result = run_captured(["clang-tidy", "-p", str(self.build_dir), str(file)])
        return rel_path, result.returncode != 0

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

        files_with_issues: list[str] = []

        with ThreadPoolExecutor(max_workers=self.jobs) as pool:
            futures = {pool.submit(self._analyze_file, f): f for f in files}
            for future in as_completed(futures):
                rel_path, has_issues = future.result()
                status = "ISSUES" if has_issues else "OK"
                print(f"  {rel_path}... {status}")
                if has_issues:
                    files_with_issues.append(rel_path)

        return AnalysisResult(
            tool=self.name,
            passed=not files_with_issues,
            issues=len(files_with_issues),
            files_checked=len(files),
            files_with_issues=files_with_issues,
        )
