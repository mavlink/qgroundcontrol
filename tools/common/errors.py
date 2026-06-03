"""Tool-error exceptions for QGC developer scripts."""

from __future__ import annotations


class ToolNotFoundError(Exception):
    """External tool not found in PATH (clang-format, cmake, etc.)."""

    def __init__(self, tool: str, *, hint: str | None = None) -> None:
        self.tool = tool
        self.hint = hint
        message = f"Required tool not found: {tool}"
        if hint:
            message += f"\nSuggestion: Install with: {hint}"
        super().__init__(message)
