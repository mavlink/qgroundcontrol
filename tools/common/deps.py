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
import subprocess
import sys
from pathlib import Path
from typing import TYPE_CHECKING

from .errors import ToolNotFoundError

if TYPE_CHECKING:
    from collections.abc import Iterable, Sequence

__all__ = ["check_and_report", "check_dependencies", "pip_install", "require_tool"]


def check_dependencies(tools: Iterable[str]) -> list[str]:
    """Return list of tools not found in PATH."""
    return [t for t in tools if shutil.which(t) is None]


def require_tool(name: str, *, hint: str = "") -> Path:
    """Return the path to a tool, or raise ToolNotFoundError."""
    path = shutil.which(name)
    if path is None:
        raise ToolNotFoundError(name, hint=hint or None)
    return Path(path)


def check_and_report(tools: Sequence[str], *, exit_on_missing: bool = True) -> bool:
    """Check tools and print a summary. Returns True if all found."""
    from .logging import log_error, log_ok

    missing = check_dependencies(tools)
    if not missing:
        log_ok(f"All {len(tools)} required tools found")
        return True

    for tool in missing:
        log_error(f"Missing: {tool}")
    if exit_on_missing:
        sys.exit(1)
    return False


def pip_install(packages: Sequence[str], *, quiet: bool = True) -> None:
    """Install packages into the project virtual environment when available.

    ``uv`` is preferred so setup scripts use the same environment as the rest
    of the tooling. The stdlib pip fallback uses the current interpreter.
    """
    if shutil.which("uv"):
        from .file_traversal import find_repo_root

        rel = "Scripts/python.exe" if sys.platform == "win32" else "bin/python"
        venv_python = find_repo_root() / ".venv" / rel
        if venv_python.exists():
            cmd = ["uv", "pip", "install", "--python", str(venv_python), *packages]
        else:
            cmd = ["uv", "pip", "install", "--system", *packages]
    else:
        cmd = [sys.executable, "-m", "pip", "install", *packages]
        if quiet:
            cmd.append("--quiet")
    subprocess.run(cmd, check=True)
