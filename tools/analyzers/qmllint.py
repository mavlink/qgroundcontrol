"""qmllint QML linter."""

from __future__ import annotations

import shutil
from typing import TYPE_CHECKING, ClassVar

from common.analyzer import AnalysisResult, AnalyzerBase
from common.logging import log_info, log_warn
from common.proc import run_captured

if TYPE_CHECKING:
    from pathlib import Path


class QmlLintAnalyzer(AnalyzerBase):
    """QML file linter."""

    name: ClassVar[str] = "qmllint"
    install_hint: ClassVar[str] = (
        "Install with: Qt SDK or 'sudo apt install qt6-declarative-dev-tools'"
    )

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        if shutil.which("qmllint") is None:
            log_warn("qmllint not found - skipping")
            log_info(self.install_hint)
            return AnalysisResult(tool=self.name, passed=True, output="Skipped")

        if not files:
            log_info("No QML files to lint")
            return AnalysisResult(tool=self.name, passed=True)

        log_info(f"Running qmllint on {len(files)} files...")

        issues_found = False
        files_with_issues: list[str] = []
        all_output: list[str] = []

        for file in files:
            result = run_captured(["qmllint", "--bare", str(file)])

            output = result.stdout + result.stderr
            if output.strip():
                print(output)
                all_output.append(output)

            if result.returncode != 0:
                issues_found = True
                files_with_issues.append(self.relative_path(file))

        if issues_found:
            log_warn("QML lint issues found")

        return AnalysisResult(
            tool=self.name,
            passed=not issues_found,
            issues=len(files_with_issues),
            output="\n".join(all_output),
            files_checked=len(files),
            files_with_issues=files_with_issues,
        )
