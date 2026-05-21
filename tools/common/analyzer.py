"""Base class and result shape for QGC code analyzers.

Lives in ``common/`` so both ``analyze.py`` (the dispatcher) and individual
analyzer modules under ``analyzers/`` can import it without a circular
dependency.
"""

from __future__ import annotations

import shutil
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import TYPE_CHECKING, ClassVar

from .logging import log_error, log_info

if TYPE_CHECKING:
    from pathlib import Path

__all__ = ["AnalysisResult", "AnalyzerBase"]


@dataclass
class AnalysisResult:
    """Result from running an analyzer."""

    tool: str
    passed: bool
    issues: int = 0
    output: str = ""
    files_checked: int = 0
    files_with_issues: list[str] = field(default_factory=list)


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
        if shutil.which(tool_name) is None:
            log_error(f"{tool_name} not found. {self.install_hint}")
            return False
        return True

    def require_compile_commands(self) -> bool:
        if not self.compile_commands.exists():
            log_error(f"compile_commands.json not found at {self.compile_commands}")
            log_info("Run: cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
            return False
        return True

    def relative_path(self, path: Path) -> str:
        try:
            return str(path.relative_to(self.repo_root))
        except ValueError:
            return str(path)
