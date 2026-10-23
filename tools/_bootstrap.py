"""CI debug hooks for tools/ entry scripts, installed by `import _bootstrap as _bootstrap`.

Stdlib-only: scripts import it before `common`, including under sparse checkouts.
"""

from __future__ import annotations

import contextlib
import faulthandler
import os
import sys

_DEBUG_ENV = "QGC_CI_DEBUG"
_RUNNER_DEBUG_ENV = "RUNNER_DEBUG"
_GHA_ENV = "GITHUB_ACTIONS"
_HOOK_MARKER = "_qgc_ci_debug_hook"


def _escape_data(message: str) -> str:
    # Workflow commands end at a newline; percent-encode CR/LF (and % itself) as GitHub requires.
    return message.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def _debug_enabled() -> bool:
    # GitHub sets RUNNER_DEBUG=1 for every step when debug logging is enabled.
    return (
        os.environ.get(_DEBUG_ENV, "").lower() in {"1", "true", "yes", "on"}
        or os.environ.get(_RUNNER_DEBUG_ENV) == "1"
    )


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
                message = _escape_data(f"{exc_type.__name__}: {exc}")
                print(f"::error::{message}", file=sys.stderr, flush=True)
            prev(exc_type, exc, tb)

        setattr(_excepthook, _HOOK_MARKER, True)
        sys.excepthook = _excepthook

    if not getattr(sys.unraisablehook, _HOOK_MARKER, False):
        prev_un = sys.unraisablehook

        def _unraisablehook(unraisable) -> None:
            try:
                msg = _escape_data(f"{unraisable.exc_type.__name__}: {unraisable.exc_value}")
                print(f"::error::unraisable: {msg}", file=sys.stderr, flush=True)
            except Exception:
                pass
            prev_un(unraisable)

        setattr(_unraisablehook, _HOOK_MARKER, True)
        sys.unraisablehook = _unraisablehook


_configure_debug()
