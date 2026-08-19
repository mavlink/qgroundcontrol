"""Subprocess wrappers used across QGC dev tools.

Centralizes the ``capture_output=True, text=True, check=False`` ritual that
gets repeated in nearly every tool, plus a couple of convenience entry points.

Use ``run_captured`` whenever you need stdout/stderr to make a decision.
Use ``run_text`` when you only care about stdout and want it as a string.
"""

from __future__ import annotations

import shlex
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Callable, Mapping, Sequence

__all__ = ["run_bytes", "run_captured", "run_checked_with_retry", "run_tee", "run_text"]


def run_checked_with_retry(
    cmd: Sequence[str],
    *,
    cwd: Path | str | None = None,
    env: Mapping[str, str] | None = None,
    max_attempts: int = 3,
    retry_backoff_seconds: float = 5.0,
    before_retry: Callable[[], None] | None = None,
) -> subprocess.CompletedProcess[bytes]:
    """Run a checked command with bounded linear backoff."""
    command = list(cmd)
    if not command:
        raise ValueError("cmd must not be empty")
    if max_attempts < 1:
        raise ValueError("max_attempts must be positive")
    if retry_backoff_seconds < 0:
        raise ValueError("retry_backoff_seconds must be non-negative")

    for attempt in range(1, max_attempts + 1):
        try:
            return subprocess.run(
                command,
                cwd=cwd,
                env=dict(env) if env is not None else None,
                check=True,
            )
        except subprocess.CalledProcessError:
            if attempt >= max_attempts:
                raise
            if before_retry is not None:
                before_retry()
            delay = retry_backoff_seconds * attempt
            print(
                f"Command failed (attempt {attempt}/{max_attempts}); retrying in {delay:g}s: "
                f"{Path(command[0]).name}",
                file=sys.stderr,
            )
            if delay > 0:
                time.sleep(delay)

    raise RuntimeError("unreachable")


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


def run_tee(
    cmd: Sequence[str],
    output_file: Path | str,
    *,
    cwd: Path | str | None = None,
    env: Mapping[str, str] | None = None,
) -> int:
    """Stream combined output to the console and *output_file*; return the exit code.

    Prefer a Bash pipeline when available so grandchildren retain a real stdout.
    This avoids Gradle/javac deadlocks observed with a Python-owned pipe on
    Windows runners. The direct streaming fallback supports hosts without Bash.
    """
    command = list(cmd)
    log_path = Path(output_file).resolve()
    process_env = dict(env) if env is not None else None
    bash = shutil.which("bash")
    if bash:
        quoted_cmd = " ".join(shlex.quote(part) for part in command)
        script = f"set -o pipefail; {quoted_cmd} 2>&1 | tee {shlex.quote(str(log_path))}"
        return subprocess.run(
            [bash, "-c", script], cwd=cwd, env=process_env, check=False
        ).returncode

    with log_path.open("w", encoding="utf-8") as log:
        process = subprocess.Popen(
            command,
            cwd=cwd,
            env=process_env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        assert process.stdout is not None
        for line in process.stdout:
            sys.stdout.write(line)
            log.write(line)
        process.wait()
        return process.returncode


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
