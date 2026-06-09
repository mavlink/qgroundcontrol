"""cppcheck analyzer."""

from __future__ import annotations

import tempfile
from pathlib import Path
from typing import ClassVar

from common.analyzer import AnalysisResult, AnalyzerBase
from common.logging import log_info
from common.proc import run_captured


class CppcheckAnalyzer(AnalyzerBase):
    """Cppcheck static analyzer."""

    name: ClassVar[str] = "cppcheck"
    install_hint: ClassVar[str] = "Install with: sudo apt install cppcheck"

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        if not self.require_tool("cppcheck"):
            return AnalysisResult(tool=self.name, passed=False, output="Tool not found")

        if not files:
            log_info("No files to analyze")
            return AnalysisResult(tool=self.name, passed=True)

        log_info(f"Running cppcheck on {len(files)} files...")

        with tempfile.NamedTemporaryFile(mode="w", suffix=".txt", delete=False) as f:
            for file in files:
                f.write(f"{file}\n")
            filelist_path = f.name

        try:
            result = run_captured(
                [
                    "cppcheck",
                    "--enable=warning,style,performance,portability",
                    "--std=c++20",
                    "--suppress=missingIncludeSystem",
                    "--suppress=unmatchedSuppression",
                    "--inline-suppr",
                    f"--file-list={filelist_path}",
                    "--error-exitcode=1",
                ],
            )

            output = result.stdout + result.stderr
            if output.strip():
                print(output)

            return AnalysisResult(
                tool=self.name,
                passed=result.returncode == 0,
                output=output,
                files_checked=len(files),
            )
        finally:
            Path(filelist_path).unlink(missing_ok=True)
