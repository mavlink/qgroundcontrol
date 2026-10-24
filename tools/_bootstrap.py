"""CI hooks for tools/ entry scripts, installed by `import _bootstrap as _bootstrap`.

In GitHub Actions an uncaught exception becomes an error annotation (a failed command's quotes
the end of its captured stderr), and stdout escapes what its encoding cannot hold. Debug logging
adds the fault handler, line-buffered output, and annotations for unraisable exceptions.

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
_FAILURE_OUTPUT_LINES = 10


def _escape_data(message: str) -> str:
    # Workflow commands end at a newline; percent-encode CR/LF (and % itself) as GitHub requires.
    return message.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def _debug_enabled() -> bool:
    # GitHub sets RUNNER_DEBUG=1 for every step when debug logging is enabled.
    return (
        os.environ.get(_DEBUG_ENV, "").lower() in {"1", "true", "yes", "on"}
        or os.environ.get(_RUNNER_DEBUG_ENV) == "1"
    )


def _describe(exc: BaseException) -> str:
    """Return the exception's type and message; for a failed command, the end of its output."""
    # Imported here, on the failure path, to keep the bootstrap import cheap.
    import re
    import shlex
    import subprocess

    lines = [f"{type(exc).__name__}: {exc}"]
    if isinstance(exc, (subprocess.CalledProcessError, subprocess.TimeoutExpired)):
        if isinstance(exc, subprocess.CalledProcessError):
            command = exc.cmd if isinstance(exc.cmd, str) else shlex.join(map(str, exc.cmd))
            lines = [
                f"{type(exc).__name__}: Command failed with exit status {exc.returncode}: {command}"
            ]
        for name, output in (("stderr", exc.stderr), ("stdout", exc.stdout)):
            if isinstance(output, bytes):
                output = output.decode("utf-8", errors="replace")
            tail = [line for line in (output or "").splitlines() if line.strip()]
            if tail := tail[-_FAILURE_OUTPUT_LINES:]:
                lines += [f"{name} (last {len(tail)} lines):", *(f"  {line}" for line in tail)]
                break
    # GitHub masks registered secrets; these also hide credentials it was never told about.
    text = re.sub(r"(?<=://)[^\s/@]+@", "***@", "\n".join(lines))
    return re.sub(r"(?i)(authorization:\s*(?:(?:basic|bearer|token)\s+)?)[^\s'\"]+", r"\1***", text)


def _configure_debug() -> None:
    """Install the GHA error annotation, plus fault handler + line buffering when debugging.

    Idempotent and re-entry / module reload safe: hooks are tagged via setattr so we won't
    wrap an already-wrapped excepthook.
    """
    debug = _debug_enabled()
    if debug:
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

    # A piped stdout uses the Windows code page, which cannot encode every character that tool
    # output and annotations quote; escape those instead of failing the step.
    reconfigure = getattr(sys.stdout, "reconfigure", None)
    if reconfigure is not None:
        with contextlib.suppress(ValueError, OSError):
            reconfigure(errors="backslashreplace")

    if not getattr(sys.excepthook, _HOOK_MARKER, False):
        prev = sys.excepthook

        def _excepthook(exc_type, exc, tb) -> None:
            # A cancelled job interrupts the script; that is not its error.
            if not issubclass(exc_type, KeyboardInterrupt):
                with contextlib.suppress(Exception):
                    print(f"::error::{_escape_data(_describe(exc))}", file=sys.stderr, flush=True)
            prev(exc_type, exc, tb)

        setattr(_excepthook, _HOOK_MARKER, True)
        sys.excepthook = _excepthook

    # Unraisable exceptions do not fail the step, so they are annotated only when debugging.
    if debug and not getattr(sys.unraisablehook, _HOOK_MARKER, False):
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
