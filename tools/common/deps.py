"""Pre-flight dependency checks for external tools.

Usage:
    from common.deps import check_dependencies, require_tool

    # Check multiple tools at once, return missing list
    missing = check_dependencies(["cmake", "ninja", "clang-format"])

    # Require a single tool, raise ToolNotFoundError if missing
    path = require_tool("gcovr")
"""

from __future__ import annotations

import shutil
from pathlib import Path

from .errors import ToolNotFoundError


def check_dependencies(tools: list[str]) -> list[str]:
    """Return list of tools not found in PATH."""
    return [t for t in tools if shutil.which(t) is None]


def require_tool(name: str, *, hint: str = "") -> Path:
    """Return the path to a tool, or raise ToolNotFoundError."""
    path = shutil.which(name)
    if path is None:
        msg = f"Required tool not found: {name}"
        if hint:
            msg += f" ({hint})"
        raise ToolNotFoundError(msg)
    return Path(path)


def check_and_report(tools: list[str], *, exit_on_missing: bool = True) -> bool:
    """Check tools and print a summary. Returns True if all found."""
    from .logging import log_ok, log_error

    missing = check_dependencies(tools)
    if not missing:
        log_ok(f"All {len(tools)} required tools found")
        return True

    for tool in missing:
        log_error(f"Missing: {tool}")
    if exit_on_missing:
        import sys
        sys.exit(1)
    return False
