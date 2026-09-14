"""Qt-specific Clazy analysis using the configured compilation database."""

from typing import ClassVar

from .compiler import CompilerAnalyzer


class ClazyAnalyzer(CompilerAnalyzer):
    name: ClassVar[str] = "clazy"
    executable: ClassVar[str] = "clazy-standalone"
    install_hint: ClassVar[str] = "Install clazy matching the analysis compiler."
    CHECKS: ClassVar[str] = "level1,connect-non-signal,lambda-in-connect,overridden-signal"
    arguments: ClassVar[tuple[str, ...]] = (f"--checks={CHECKS}",)
