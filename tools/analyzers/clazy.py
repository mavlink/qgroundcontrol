"""clazy Qt-specific analyzer."""

from __future__ import annotations

import shutil
from typing import TYPE_CHECKING, ClassVar

from common.analyzer import AnalysisResult, AnalyzerBase, FileAnalysis
from common.logging import log_info, log_warn
from common.proc import run_captured

if TYPE_CHECKING:
    from pathlib import Path


class ClazyAnalyzer(AnalyzerBase):
    """Clazy Qt-specific static analyzer."""

    name: ClassVar[str] = "clazy"
    install_hint: ClassVar[str] = "Install with: sudo apt install clazy"
    CHECKS: ClassVar[str] = "level1,connect-non-signal,lambda-in-connect,overridden-signal"

    def _analyze_file(self, file: Path) -> FileAnalysis:
        rel_path = self.relative_path(file)
        result = run_captured(
            [
                "clazy-standalone",
                "-p",
                str(self.build_dir),
                f"--checks={self.CHECKS}",
                str(file),
            ],
        )
        has_issues = result.returncode != 0 and bool(result.stderr.strip())
        return FileAnalysis(path=rel_path, has_issues=has_issues, output=result.stderr)

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        if not self.compile_commands.exists():
            log_warn("compile_commands.json not found - skipping clazy")
            log_info("Run: cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
            return AnalysisResult(tool=self.name, passed=True, output="Skipped")

        if shutil.which("clazy-standalone") is None:
            log_warn("clazy-standalone not found - skipping")
            log_info(self.install_hint)
            return AnalysisResult(tool=self.name, passed=True, output="Skipped")

        if not files:
            log_info("No files to analyze")
            return AnalysisResult(tool=self.name, passed=True)

        log_info(f"Running clazy on {len(files)} files ({self.jobs} jobs)...")

        results = self.run_files_parallel(files, self._analyze_file, jobs=self.jobs)
        files_with_issues = [result.path for result in results if result.has_issues]
        all_output: list[str] = []
        for result in results:
            if result.has_issues:
                log_warn(f"Issues in {result.path}:")
                print(result.output)
                all_output.append(result.output)

        if files_with_issues:
            log_warn("Clazy found Qt-specific issues")

        return AnalysisResult(
            tool=self.name,
            passed=not files_with_issues,
            issues=len(files_with_issues),
            output="\n".join(all_output),
            files_checked=len(files),
            files_with_issues=files_with_issues,
        )
