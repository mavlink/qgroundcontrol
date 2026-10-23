"""Base class and result shape for ``analyze.py`` analyzer backends."""

from __future__ import annotations

import argparse
import json
import sys
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, ClassVar

from common.file_traversal import find_cpp_files
from common.logging import log_error, log_info
from common.proc import MissingToolError, require_tools

__all__ = ["AnalysisResult", "AnalyzerBase", "run_line_check_cli"]


@dataclass
class AnalysisResult:
    """Result from running an analyzer."""

    tool: str
    passed: bool
    issues: int = 0
    output: str = ""
    files_checked: int = 0
    files_with_issues: list[str] = field(default_factory=list)
    execution_error: bool = False
    error_findings: bool = False
    skipped: bool = False
    # One JSON-serializable record per finding, for line checks' --json output.
    findings: list[dict[str, Any]] = field(default_factory=list)

    @property
    def status(self) -> str:
        if self.execution_error or self.error_findings:
            return "error"
        if self.skipped:
            return "skipped"
        return "passed" if self.passed else "findings"


class AnalyzerBase(ABC):
    """Base class for all code analyzers."""

    name: ClassVar[str] = "base"
    install_hint: ClassVar[str] = ""

    def __init__(self, repo_root: Path, build_dir: Path) -> None:
        self.repo_root = repo_root
        self.build_dir = build_dir
        self.compile_commands = build_dir / "compile_commands.json"

    @abstractmethod
    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        """Run the analyzer on the given files."""

    def require_tool(self, tool_name: str) -> bool:
        try:
            require_tools(tool_name, hint=self.install_hint)
        except MissingToolError as error:
            log_error(str(error))
            return False
        return True

    def require_compile_commands(self) -> bool:
        if not self.compile_commands.exists():
            log_error(f"compile_commands.json not found at {self.compile_commands}")
            log_info("Run: just configure")
            return False
        return True

    def relative_path(self, path: Path) -> str:
        try:
            return str(path.relative_to(self.repo_root))
        except ValueError:
            return str(path)


def run_line_check_cli(analyzer_cls: type[AnalyzerBase], argv: list[str] | None = None) -> int:
    """Command line of a line-based C++ check, which its pre-commit hook runs."""
    parser = argparse.ArgumentParser(description=analyzer_cls.__doc__)
    parser.add_argument(
        "paths", nargs="+", type=Path, metavar="PATH", help="C++ files or directories to check"
    )
    parser.add_argument("--json", action="store_true", help="Print the findings as a JSON list")
    args = parser.parse_args(argv)

    root = Path.cwd()
    result = analyzer_cls(root, root / "build").run(list(find_cpp_files(args.paths)))
    if args.json:
        print(json.dumps(result.findings, indent=2))
    else:
        print(result.output, end="")
        if not result.passed:
            print(f"{result.issues} {result.tool} finding(s)", file=sys.stderr)
    return int(not result.passed)
