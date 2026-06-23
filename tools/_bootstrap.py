"""Shared import bootstrap for Python entrypoints under tools/ and .github/scripts/."""

from __future__ import annotations

import contextlib
import faulthandler
import os
import sys
from pathlib import Path

_DEBUG_ENV = "QGC_CI_DEBUG"
_GHA_ENV = "GITHUB_ACTIONS"
_HOOK_MARKER = "_qgc_ci_debug_hook"


def _debug_enabled() -> bool:
    return os.environ.get(_DEBUG_ENV, "").lower() in {"1", "true", "yes", "on"}


def _configure_debug() -> None:
    """Install fault handler + line buffering + GHA error annotation (idempotent).

    Re-entry / module reload safe: hooks are tagged via setattr so we won't
    wrap an already-wrapped excepthook.
    """
    if not _debug_enabled():
        return

    if not faulthandler.is_enabled():
        # stderr may be redirected by a test runner; non-fatal.
        with contextlib.suppress(RuntimeError, ValueError, OSError):
            faulthandler.enable(file=sys.stderr)

    for stream in (sys.stdout, sys.stderr):
        reconfigure = getattr(stream, "reconfigure", None)
        if reconfigure is not None:
            with contextlib.suppress(ValueError, OSError):
                reconfigure(line_buffering=True)

    if os.environ.get(_GHA_ENV, "").lower() != "true":
        return

    if not getattr(sys.excepthook, _HOOK_MARKER, False):
        prev = sys.excepthook

        def _excepthook(exc_type, exc, tb) -> None:
            with contextlib.suppress(Exception):
                print(f"::error::{exc_type.__name__}: {exc}", file=sys.stderr, flush=True)
            prev(exc_type, exc, tb)

        setattr(_excepthook, _HOOK_MARKER, True)
        sys.excepthook = _excepthook

    if not getattr(sys.unraisablehook, _HOOK_MARKER, False):
        prev_un = sys.unraisablehook

        def _unraisablehook(unraisable) -> None:
            try:
                msg = f"{unraisable.exc_type.__name__}: {unraisable.exc_value}"
                print(f"::error::unraisable: {msg}", file=sys.stderr, flush=True)
            except Exception:
                pass
            prev_un(unraisable)

        setattr(_unraisablehook, _HOOK_MARKER, True)
        sys.unraisablehook = _unraisablehook


_configure_debug()


def ensure_tools_dir(start: str | Path) -> Path:
    """Ensure the top-level ``tools`` directory is importable.

    Locates ``tools`` either as an ancestor of ``start`` (script lives under
    tools/) or as a sibling subdirectory of an ancestor (script lives under
    .github/scripts/ or similar). Raises if neither shape is found.
    """
    path = Path(start).resolve()
    current = path if path.is_dir() else path.parent
    for candidate in [current, *current.parents]:
        if candidate.name == "tools":
            tools_dir = candidate
            break
        sibling = candidate / "tools"
        if sibling.is_dir():
            tools_dir = sibling
            break
    else:
        raise RuntimeError(f"Could not locate tools directory from {start}")

    if str(tools_dir) not in sys.path:
        sys.path.insert(0, str(tools_dir))
    return tools_dir
