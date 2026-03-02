"""
Unified logging utilities for QGC developer tools.

Provides colored terminal output with support for:
- TTY detection (auto-disable colors in non-interactive mode)
- NO_COLOR environment variable (https://no-color.org/)
- Both class-based Logger and module-level convenience functions
- Debug mode via DEBUG environment variable

Usage:
    # Module-level functions (simple)
    from common.logging import log_info, log_error, log_ok, log_warn
    log_info("Starting analysis...")
    log_error("Failed to parse file")

    # Class-based Logger (more control)
    from common.logging import Logger
    logger = Logger()
    logger.info("Processing...")
    logger.ok("Done!")

    # Disable colors explicitly
    logger = Logger(color=False)
"""

from __future__ import annotations

import os
import sys
from dataclasses import dataclass
from enum import Enum
from typing import TextIO


class Color(Enum):
    """ANSI color codes for terminal output."""

    RED = "\033[0;31m"
    GREEN = "\033[0;32m"
    YELLOW = "\033[1;33m"
    BLUE = "\033[0;34m"
    CYAN = "\033[0;36m"
    MAGENTA = "\033[0;35m"
    BOLD = "\033[1m"
    DIM = "\033[2m"
    RESET = "\033[0m"
    NC = "\033[0m"  # Alias for RESET


def use_color(stream: TextIO = None) -> bool:
    """
    Determine if colored output should be used.

    Checks:
    1. NO_COLOR environment variable (disables if set)
    2. FORCE_COLOR environment variable (enables if set)
    3. Whether stdout is a TTY

    Args:
        stream: Output stream to check. Defaults to sys.stdout.

    Returns:
        True if colors should be used.
    """
    if os.environ.get("NO_COLOR"):
        return False
    if os.environ.get("FORCE_COLOR"):
        return True
    if stream is None:
        stream = sys.stdout
    return hasattr(stream, "isatty") and stream.isatty()


def colorize(text: str, color: Color, stream: TextIO = None) -> str:
    """
    Apply ANSI color code to text if colors are enabled.

    Args:
        text: Text to colorize.
        color: Color to apply.
        stream: Output stream for TTY detection. Defaults to sys.stdout.

    Returns:
        Colorized text or original text if colors disabled.
    """
    if use_color(stream):
        return f"{color.value}{text}{Color.RESET.value}"
    return text


def is_debug() -> bool:
    """Check if debug mode is enabled via DEBUG environment variable."""
    return bool(os.environ.get("DEBUG"))


def is_verbose() -> bool:
    """Check if verbose mode is enabled via VERBOSE environment variable."""
    return bool(os.environ.get("VERBOSE"))


# Module-level convenience functions


def log_info(msg: str, *, prefix: str = "[INFO]") -> None:
    """Log an informational message."""
    print(f"{colorize(prefix, Color.BLUE)} {msg}")


def log_ok(msg: str, *, prefix: str = "[OK]") -> None:
    """Log a success message."""
    print(f"{colorize(prefix, Color.GREEN)} {msg}")


def log_warn(msg: str, *, prefix: str = "[WARN]") -> None:
    """Log a warning message."""
    print(f"{colorize(prefix, Color.YELLOW)} {msg}")


def log_error(msg: str, *, prefix: str = "[ERROR]") -> None:
    """Log an error message to stderr."""
    print(f"{colorize(prefix, Color.RED, sys.stderr)} {msg}", file=sys.stderr)


def log_debug(msg: str, *, prefix: str = "[DEBUG]") -> None:
    """Log a debug message to stderr (only if DEBUG env var is set)."""
    if is_debug():
        print(f"{colorize(prefix, Color.DIM, sys.stderr)} {msg}", file=sys.stderr)


def log_verbose(msg: str, *, prefix: str = "[VERBOSE]") -> None:
    """Log a verbose message (only if VERBOSE or DEBUG env var is set)."""
    if is_verbose() or is_debug():
        print(f"{colorize(prefix, Color.DIM)} {msg}")


def log_step(msg: str, step: int = None, total: int = None) -> None:
    """Log a step in a multi-step process."""
    if step is not None and total is not None:
        prefix = f"[{step}/{total}]"
    elif step is not None:
        prefix = f"[{step}]"
    else:
        prefix = "[*]"
    print(f"{colorize(prefix, Color.CYAN)} {msg}")


def log_command(cmd: str | list[str]) -> None:
    """Log a command being executed."""
    if isinstance(cmd, list):
        cmd = " ".join(cmd)
    log_debug(f"$ {cmd}")


@dataclass
class Logger:
    """
    Logger with colored output support.

    Provides instance methods for logging with configurable color support.
    Useful when you need to pass a logger around or control colors per-instance.

    Attributes:
        color: Whether to use colored output. Auto-detected if None.
        prefix_info: Prefix for info messages. Default: "[INFO]"
        prefix_ok: Prefix for success messages. Default: "[OK]"
        prefix_warn: Prefix for warning messages. Default: "[WARN]"
        prefix_error: Prefix for error messages. Default: "[ERROR]"
    """

    color: bool = None
    prefix_info: str = "[INFO]"
    prefix_ok: str = "[OK]"
    prefix_warn: str = "[WARN]"
    prefix_error: str = "[ERROR]"
    prefix_debug: str = "[DEBUG]"

    def __post_init__(self) -> None:
        if self.color is None:
            self.color = use_color()

    def _colorize(self, text: str, color: Color) -> str:
        if self.color:
            return f"{color.value}{text}{Color.RESET.value}"
        return text

    def info(self, msg: str) -> None:
        """Log an informational message."""
        print(f"{self._colorize(self.prefix_info, Color.BLUE)} {msg}")

    def ok(self, msg: str) -> None:
        """Log a success message."""
        print(f"{self._colorize(self.prefix_ok, Color.GREEN)} {msg}")

    def warn(self, msg: str) -> None:
        """Log a warning message."""
        print(f"{self._colorize(self.prefix_warn, Color.YELLOW)} {msg}")

    def error(self, msg: str) -> None:
        """Log an error message to stderr."""
        print(f"{self._colorize(self.prefix_error, Color.RED)} {msg}", file=sys.stderr)

    def debug(self, msg: str) -> None:
        """Log a debug message (only if DEBUG env var is set)."""
        if is_debug():
            print(
                f"{self._colorize(self.prefix_debug, Color.DIM)} {msg}", file=sys.stderr
            )

    def step(self, msg: str, step: int = None, total: int = None) -> None:
        """Log a step in a multi-step process."""
        if step is not None and total is not None:
            prefix = f"[{step}/{total}]"
        elif step is not None:
            prefix = f"[{step}]"
        else:
            prefix = "[*]"
        print(f"{self._colorize(prefix, Color.CYAN)} {msg}")

    def command(self, cmd: str | list[str]) -> None:
        """Log a command being executed."""
        if isinstance(cmd, list):
            cmd = " ".join(cmd)
        self.debug(f"$ {cmd}")


# Convenience aliases for backward compatibility
Colors = Color  # Some scripts use "Colors" instead of "Color"
