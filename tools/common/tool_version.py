"""Probe an external tool's version string into a tuple of ints.

Standardizes the ``tool --version`` + regex parse that ``ccache_helper``,
``release.py``, ``analyze.py``, and ``check_deps.py`` each implement
differently.
"""

from __future__ import annotations

import re
import shutil
from pathlib import Path
from typing import TYPE_CHECKING

from .proc import run_captured

if TYPE_CHECKING:
    from collections.abc import Sequence

__all__ = ["DEFAULT_VERSION_RE", "probe_version", "uv_lock_version"]

DEFAULT_VERSION_RE: re.Pattern[str] = re.compile(r"(\d+)\.(\d+)(?:\.(\d+))?")

_UV_LOCK = Path(__file__).resolve().parents[1] / "uv.lock"


def uv_lock_version(package: str, *, lock_path: Path | None = None) -> str | None:
    """Return the version pinned for *package* in tools/uv.lock, or None if absent/unreadable.

    Lets a setup script pin its fallback binary to the same version the dev/CI venv
    resolves (single source of truth), so the two install paths can't drift. Returns
    None in contexts without the lockfile (e.g. the Docker builders that COPY setup
    scripts out of the repo), leaving the caller to apply its own fallback.
    """
    path = lock_path or _UV_LOCK
    try:
        text = path.read_text(encoding="utf-8")
    except OSError:
        return None
    match = re.search(
        r'\[\[package\]\]\s*\nname\s*=\s*"' + re.escape(package) + r'"\s*\nversion\s*=\s*"([\d.]+)"',
        text,
    )
    return match.group(1) if match else None


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
    except (TimeoutError, OSError):
        return None

    if result.returncode != 0:
        return None

    # version may go to stdout or stderr depending on the tool
    text = result.stdout or result.stderr
    match = pattern.search(text)
    if not match:
        return None

    return tuple(int(g) for g in match.groups() if g is not None)
