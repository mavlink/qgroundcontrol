"""External-tool and uv.lock version lookup."""

from __future__ import annotations

import re
import shutil
import subprocess
from typing import TYPE_CHECKING

from .io import read_toml
from .paths import TOOLS_DIR
from .proc import run_captured

if TYPE_CHECKING:
    from collections.abc import Sequence
    from pathlib import Path

__all__ = ["DEFAULT_VERSION_RE", "probe_version", "uv_lock_version", "version_prefix_matches"]

DEFAULT_VERSION_RE: re.Pattern[str] = re.compile(r"(\d+)\.(\d+)(?:\.(\d+))?")

_UV_LOCK = TOOLS_DIR / "uv.lock"


def version_prefix_matches(installed: tuple[int, ...], expected: str) -> bool:
    """Compare installed and expected versions over their shared components."""
    try:
        wanted = tuple(int(part) for part in expected.split("."))
    except ValueError:
        return False
    compare_len = min(len(installed), len(wanted))
    return compare_len > 0 and installed[:compare_len] == wanted[:compare_len]


def uv_lock_version(package: str, *, lock_path: Path | None = None) -> str | None:
    """Return the version pinned for *package* in tools/uv.lock, or None if absent/unreadable.

    Lets a check compare an installed tool against the version the dev/CI venv resolves.
    Returns None in contexts without the lockfile, leaving the caller to decide.
    """
    path = lock_path or _UV_LOCK
    if not path.exists():
        return None
    packages = read_toml(path).get("package", [])
    versions = {
        str(entry["version"])
        for entry in packages
        if entry.get("name") == package and "version" in entry
    }
    if len(versions) > 1:
        raise ValueError(f"Multiple locked versions of {package}: {', '.join(sorted(versions))}")
    return next(iter(versions), None)


def probe_version(
    tool: str,
    *,
    args: Sequence[str] = ("--version",),
    pattern: re.Pattern[str] = DEFAULT_VERSION_RE,
    timeout: float = 5.0,
) -> tuple[int, ...] | None:
    """Run ``<tool> <args>`` and parse the first version-shaped token.

    Returns a tuple like ``(4, 13, 6)`` or ``None`` if the tool is missing,
    times out, exits non-zero, or produces no parseable version.
    Patch component is optional; missing trailing components are dropped.
    """
    if not shutil.which(tool):
        return None

    try:
        result = run_captured([tool, *args], timeout=timeout)
    except (subprocess.TimeoutExpired, OSError):
        return None

    if result.returncode != 0:
        return None

    # version may go to stdout or stderr depending on the tool
    text = result.stdout or result.stderr
    match = pattern.search(text)
    if not match:
        return None

    return tuple(int(g) for g in match.groups() if g is not None)
