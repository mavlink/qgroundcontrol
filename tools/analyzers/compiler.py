"""Shared execution and diagnostics for compilation-database analyzers."""

from __future__ import annotations

import re
import subprocess
from concurrent.futures import ThreadPoolExecutor
from typing import TYPE_CHECKING, ClassVar

from common.analyzer import AnalysisResult, AnalyzerBase
from common.proc import run_captured

if TYPE_CHECKING:
    from pathlib import Path


class CompilerAnalyzer(AnalyzerBase):
    executable: ClassVar[str]
    arguments: ClassVar[tuple[str, ...]] = ()

    def __init__(self, repo_root: Path, build_dir: Path, jobs: int = 1) -> None:
        super().__init__(repo_root, build_dir)
        self.jobs = jobs

    def _analyze_file(self, file: Path) -> tuple[str, str, bool, bool]:
        try:
            result = run_captured(
                [self.executable, "-p", str(self.build_dir), *self.arguments, str(file)],
                timeout=300,
            )
        except (OSError, subprocess.TimeoutExpired) as exc:
            return self.relative_path(file), str(exc), False, True
        output = result.stdout + result.stderr
        findings = bool(re.search(r"\b(?:warning|error):", output))
        # A failed compiler invocation is an analysis failure, not a clean scan.
        compiler_error = bool(
            re.search(
                r"fatal error:|\[clang-diagnostic-error\]|^error:|:\d+:\d+: error: (?!.*\[[^]]+\]$)",
                output,
                re.MULTILINE,
            )
        )
        error = compiler_error or (result.returncode != 0 and not findings)
        return self.relative_path(file), output, findings, error

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        if not files:
            return AnalysisResult(tool=self.name, passed=True, skipped=True)
        if not self.require_compile_commands() or not self.require_tool(self.executable):
            return AnalysisResult(tool=self.name, passed=False, execution_error=True)

        outputs: list[str] = []
        affected: list[str] = []
        errors = False
        with ThreadPoolExecutor(max_workers=self.jobs) as pool:
            for name, output, findings, error in pool.map(self._analyze_file, files):
                print(f"{name}: {'error' if error else 'findings' if findings else 'passed'}")
                if output:
                    print(output, end="" if output.endswith("\n") else "\n")
                    outputs.append(output)
                if findings or error:
                    affected.append(name)
                errors |= error
        return AnalysisResult(
            tool=self.name,
            passed=not affected,
            issues=len(affected),
            files_checked=len(files),
            files_with_issues=affected,
            output="\n".join(outputs),
            execution_error=errors,
        )
