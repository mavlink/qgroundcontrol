"""Clang-Tidy analysis using the configured compilation database."""

from typing import ClassVar

from .compiler import CompilerAnalyzer


class ClangTidyAnalyzer(CompilerAnalyzer):
    name: ClassVar[str] = "clang-tidy"
    executable: ClassVar[str] = "clang-tidy"
    install_hint: ClassVar[str] = "Install clang-tidy matching the analysis compiler."
