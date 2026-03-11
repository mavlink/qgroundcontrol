"""
Custom exception hierarchy for QGC developer tools.

Provides structured error handling with context preservation
and consistent error reporting across all tools.

Usage:
    from common.errors import QGCToolError, ConfigError, FileNotFoundError

    try:
        config = load_config(path)
    except ConfigError as e:
        log_error(f"Configuration error: {e}")
        return 1

Exception Hierarchy:
    QGCToolError (base)
    ├── ConfigError - Configuration file issues
    ├── BuildError - Build/compilation failures
    ├── ToolNotFoundError - Missing external tools
    ├── ValidationError - Input validation failures
    ├── GitError - Git operation failures
    └── SubprocessError - External command failures
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


class QGCToolError(Exception):
    """
    Base exception for all QGC tool errors.

    Attributes:
        message: Human-readable error description.
        context: Additional context about the error.
        suggestion: Optional suggestion for fixing the error.
    """

    def __init__(
        self,
        message: str,
        *,
        context: dict[str, Any] = None,
        suggestion: str = None,
    ) -> None:
        self.message = message
        self.context = context or {}
        self.suggestion = suggestion
        super().__init__(self._format_message())

    def _format_message(self) -> str:
        parts = [self.message]
        if self.context:
            context_str = ", ".join(f"{k}={v}" for k, v in self.context.items())
            parts.append(f"[{context_str}]")
        if self.suggestion:
            parts.append(f"\nSuggestion: {self.suggestion}")
        return " ".join(parts)


class ConfigError(QGCToolError):
    """
    Configuration file error.

    Raised when a configuration file is missing, invalid, or has
    unexpected content.
    """

    def __init__(
        self,
        message: str,
        *,
        path: Path = None,
        key: str = None,
        expected: Any = None,
        actual: Any = None,
        **kwargs,
    ) -> None:
        context = kwargs.pop("context", {})
        if path:
            context["path"] = str(path)
        if key:
            context["key"] = key
        if expected is not None:
            context["expected"] = expected
        if actual is not None:
            context["actual"] = actual
        super().__init__(message, context=context, **kwargs)


class BuildError(QGCToolError):
    """
    Build or compilation error.

    Raised when CMake configuration, compilation, or linking fails.
    """

    def __init__(
        self,
        message: str,
        *,
        build_type: str = None,
        exit_code: int = None,
        output: str = None,
        **kwargs,
    ) -> None:
        context = kwargs.pop("context", {})
        if build_type:
            context["build_type"] = build_type
        if exit_code is not None:
            context["exit_code"] = exit_code
        self.output = output
        super().__init__(message, context=context, **kwargs)


class ToolNotFoundError(QGCToolError):
    """
    External tool not found.

    Raised when a required external tool (clang-format, cmake, etc.)
    is not available in PATH.
    """

    def __init__(
        self,
        tool: str,
        *,
        install_hint: str = None,
        **kwargs,
    ) -> None:
        message = f"Required tool not found: {tool}"
        suggestion = kwargs.pop("suggestion", None)
        if install_hint and not suggestion:
            suggestion = f"Install with: {install_hint}"
        super().__init__(message, suggestion=suggestion, context={"tool": tool}, **kwargs)
        self.tool = tool


class ValidationError(QGCToolError):
    """
    Input validation error.

    Raised when user input, file content, or arguments fail validation.
    """

    def __init__(
        self,
        message: str,
        *,
        field: str = None,
        value: Any = None,
        constraint: str = None,
        **kwargs,
    ) -> None:
        context = kwargs.pop("context", {})
        if field:
            context["field"] = field
        if value is not None:
            context["value"] = repr(value)
        if constraint:
            context["constraint"] = constraint
        super().__init__(message, context=context, **kwargs)


class GitError(QGCToolError):
    """
    Git operation error.

    Raised when git commands fail or repository state is unexpected.
    """

    def __init__(
        self,
        message: str,
        *,
        command: str | list[str] = None,
        exit_code: int = None,
        output: str = None,
        **kwargs,
    ) -> None:
        context = kwargs.pop("context", {})
        if command:
            if isinstance(command, list):
                command = " ".join(command)
            context["command"] = command
        if exit_code is not None:
            context["exit_code"] = exit_code
        self.output = output
        super().__init__(message, context=context, **kwargs)


class SubprocessError(QGCToolError):
    """
    External command execution error.

    Raised when a subprocess fails with non-zero exit code or other errors.
    """

    def __init__(
        self,
        message: str,
        *,
        command: str | list[str] = None,
        exit_code: int = None,
        stdout: str = None,
        stderr: str = None,
        **kwargs,
    ) -> None:
        context = kwargs.pop("context", {})
        if command:
            if isinstance(command, list):
                command = " ".join(command)
            context["command"] = command
        if exit_code is not None:
            context["exit_code"] = exit_code
        self.stdout = stdout
        self.stderr = stderr
        super().__init__(message, context=context, **kwargs)


class FileError(QGCToolError):
    """
    File operation error.

    Raised when file read/write/parse operations fail.
    """

    def __init__(
        self,
        message: str,
        *,
        path: Path = None,
        operation: str = None,
        **kwargs,
    ) -> None:
        context = kwargs.pop("context", {})
        if path:
            context["path"] = str(path)
        if operation:
            context["operation"] = operation
        super().__init__(message, context=context, **kwargs)
        self.path = path


class TestError(QGCToolError):
    """
    Test execution error.

    Raised when test discovery, execution, or assertion fails.
    """

    def __init__(
        self,
        message: str,
        *,
        test_name: str = None,
        failed_count: int = None,
        total_count: int = None,
        **kwargs,
    ) -> None:
        context = kwargs.pop("context", {})
        if test_name:
            context["test"] = test_name
        if failed_count is not None:
            context["failed"] = failed_count
        if total_count is not None:
            context["total"] = total_count
        super().__init__(message, context=context, **kwargs)


@dataclass
class Result:
    """
    Generic result container for tool operations.

    Use this for returning structured results from tool functions
    instead of just returning int exit codes.

    Attributes:
        success: Whether the operation succeeded.
        message: Human-readable result message.
        data: Optional structured data from the operation.
        errors: List of error messages if operation failed.
        warnings: List of warning messages.
    """

    success: bool
    message: str = ""
    data: dict[str, Any] = field(default_factory=dict)
    errors: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)

    @classmethod
    def ok(cls, message: str = "Success", **data) -> "Result":
        """Create a successful result."""
        return cls(success=True, message=message, data=data)

    @classmethod
    def fail(cls, message: str, errors: list[str] = None) -> "Result":
        """Create a failed result."""
        return cls(success=False, message=message, errors=errors or [message])

    def __bool__(self) -> bool:
        """Allow using Result in boolean context."""
        return self.success
