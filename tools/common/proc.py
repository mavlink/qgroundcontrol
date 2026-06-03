"""Subprocess wrappers used across QGC dev tools.

Centralizes the ``capture_output=True, text=True, check=False`` ritual that
gets repeated in nearly every tool, plus a couple of convenience entry points.

Use ``run_captured`` whenever you need stdout/stderr to make a decision.
Use ``run_text`` when you only care about stdout and want it as a string.
"""

from __future__ import annotations

import subprocess
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Mapping, Sequence
    from pathlib import Path

__all__ = ["run_bytes", "run_captured", "run_text"]


def run_bytes(
    cmd: Sequence[str],
    *,
    cwd: Path | str | None = None,
    env: Mapping[str, str] | None = None,
    timeout: float | None = None,
    check: bool = False,
) -> subprocess.CompletedProcess[bytes]:
    """Like ``run_captured`` but returns raw bytes — for outputs that may not be valid UTF-8.

    Use this for tools whose stdout/stderr can contain undecodable bytes (e.g. adb
    logcat under a native crash). Decode at call sites with ``errors="replace"``.
    """
    return subprocess.run(
        list(cmd),
        capture_output=True,
        cwd=cwd,
        env=dict(env) if env is not None else None,
        timeout=timeout,
        check=check,
    )


def run_captured(
    cmd: Sequence[str],
    *,
    cwd: Path | str | None = None,
    env: Mapping[str, str] | None = None,
    timeout: float | None = None,
    check: bool = False,
    input_text: str | None = None,
) -> subprocess.CompletedProcess[str]:
    """Run *cmd*, capturing stdout/stderr as text.

    Returns the :class:`subprocess.CompletedProcess` so the caller can inspect
    ``returncode``, ``stdout``, ``stderr``. Raises :class:`subprocess.CalledProcessError`
    only when ``check=True``.
    """
    return subprocess.run(
        list(cmd),
        capture_output=True,
        text=True,
        cwd=cwd,
        env=dict(env) if env is not None else None,
        timeout=timeout,
        check=check,
        input=input_text,
    )


def run_text(
    cmd: Sequence[str],
    *,
    cwd: Path | str | None = None,
    timeout: float | None = None,
    default: str = "",
) -> str:
    """Run *cmd* and return its stdout (stripped). Returns *default* on failure."""
    try:
        result = run_captured(cmd, cwd=cwd, timeout=timeout)
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return default
    if result.returncode != 0:
        return default
    return result.stdout.strip()
