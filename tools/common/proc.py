"""Subprocess wrappers, required-tool lookup, and polling used across QGC dev tools.

Centralizes the ``capture_output=True, text=True, check=False`` ritual that
gets repeated in nearly every tool, plus a couple of convenience entry points.

Use ``run_captured`` whenever you need stdout/stderr to make a decision.
"""

from __future__ import annotations

import contextlib
import os
import re
import shlex
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path
from typing import TYPE_CHECKING, Any, TypeVar

if TYPE_CHECKING:
    from collections.abc import Callable, Mapping, Sequence

__all__ = [
    "MissingToolError",
    "kill_process_tree",
    "output_text",
    "require_tools",
    "run_bytes",
    "run_captured",
    "run_tee",
    "run_with_retry",
    "wait_until",
]

_T = TypeVar("_T")

FAILURE_OUTPUT_LINES = 10
# GitHub masks registered secrets in logs; these also hide credentials it was never told about.
_URL_CREDENTIALS = re.compile(r"(?<=://)[^\s/@]+@")
_AUTHORIZATION = re.compile(r"(?i)(authorization:\s*(?:(?:basic|bearer|token)\s+)?)[^\s'\"]+")


class MissingToolError(FileNotFoundError):
    """A required executable is not on ``PATH``."""


def require_tools(*names: str, hint: str = "") -> dict[str, str]:
    """Return the ``PATH`` location of each tool in *names*.

    Raises :class:`MissingToolError` naming every missing tool, followed by *hint*.
    """
    found: dict[str, str] = {}
    missing: list[str] = []
    for name in names:
        if path := shutil.which(name):
            found[name] = path
        else:
            missing.append(name)
    if missing:
        message = f"{', '.join(missing)} not found on PATH"
        raise MissingToolError(f"{message}. {hint}" if hint else message)
    return found


def wait_until(
    predicate: Callable[[], _T],
    *,
    timeout: float,
    interval: float,
    description: str = "condition",
    sleep: Callable[[float], object] | None = None,
    clock: Callable[[], float] | None = None,
) -> _T:
    """Poll *predicate* every *interval* seconds and return its first truthy result.

    The last poll runs at the deadline; :class:`TimeoutError` follows if that one fails too.
    *sleep* and *clock* default to ``time.sleep`` and ``time.monotonic`` at call time.
    """
    if timeout < 0 or interval <= 0:
        raise ValueError("timeout must be non-negative and interval positive")
    sleep = sleep or time.sleep
    clock = clock or time.monotonic
    deadline = clock() + timeout
    while not (result := predicate()):
        remaining = deadline - clock()
        if remaining <= 0:
            raise TimeoutError(f"Timed out after {timeout:g}s waiting for {description}")
        sleep(min(interval, remaining))
    return result


def kill_process_tree(process: subprocess.Popen[Any]) -> None:
    """Kill *process* and its descendants.

    POSIX kills the process group, so start *process* with ``start_new_session=True``; Windows
    runs ``taskkill /T``, which finds descendants through their parent process IDs.
    """
    if sys.platform == "win32":
        with contextlib.suppress(OSError, subprocess.SubprocessError):
            subprocess.run(
                ["taskkill", "/T", "/F", "/PID", str(process.pid)],
                capture_output=True,
                check=False,
                timeout=60,
            )
    else:
        with contextlib.suppress(ProcessLookupError, PermissionError):
            os.killpg(process.pid, signal.SIGKILL)
    process.kill()


def _run_killing_tree(
    command: list[str],
    *,
    capture_output: bool = False,
    timeout: float | None = None,
    check: bool = False,
    **popen_kwargs: Any,
) -> subprocess.CompletedProcess[Any]:
    """``subprocess.run`` that kills the whole process tree, not only the child, on timeout."""
    if capture_output:
        popen_kwargs["stdout"] = popen_kwargs["stderr"] = subprocess.PIPE
    with subprocess.Popen(
        command, start_new_session=sys.platform != "win32", **popen_kwargs
    ) as process:
        try:
            stdout, stderr = process.communicate(timeout=timeout)
        except BaseException:
            kill_process_tree(process)
            raise
    result = subprocess.CompletedProcess(command, process.returncode, stdout, stderr)
    if check:
        result.check_returncode()
    return result


def _redact(text: str) -> str:
    return _AUTHORIZATION.sub(r"\1***", _URL_CREDENTIALS.sub("***@", text))


def _describe_failure(error: subprocess.CalledProcessError | subprocess.TimeoutExpired) -> str:
    """Name the failed command and quote the end of its captured stderr, else stdout."""
    command = error.cmd if isinstance(error.cmd, str) else shlex.join(map(str, error.cmd))
    if isinstance(error, subprocess.TimeoutExpired):
        lines = [f"Command timed out after {error.timeout:g}s: {command}"]
    else:
        lines = [f"Command failed with exit status {error.returncode}: {command}"]
    for name, output in (("stderr", error.stderr), ("stdout", error.stdout)):
        tail = [line for line in output_text(output).splitlines() if line.strip()]
        if tail := tail[-FAILURE_OUTPUT_LINES:]:
            lines += [f"{name} (last {len(tail)} lines):", *(f"  {line}" for line in tail)]
            break
    return _redact("\n".join(lines))


def run_with_retry(
    cmd: Sequence[str],
    *,
    cwd: Path | str | None = None,
    env: Mapping[str, str] | None = None,
    max_attempts: int = 3,
    retry_backoff_seconds: float = 5.0,
    before_retry: Callable[[], None] | None = None,
    retry_if: Callable[[Exception], bool] | None = None,
    retry_result_if: Callable[[subprocess.CompletedProcess[Any]], bool] | None = None,
    timeout: float | None = None,
    kill_tree_on_timeout: bool = False,
    check: bool = True,
    capture_output: bool = False,
    text: bool = False,
    encoding: str | None = None,
    errors: str | None = None,
) -> subprocess.CompletedProcess[Any]:
    """Run a command with bounded retries and linear backoff.

    A timed-out attempt kills only the command itself unless *kill_tree_on_timeout* is set,
    which also kills the processes it started (installers that spawn their own workers).
    The exception that ends the retries is logged to stderr first, with the end of the
    command's captured stderr (else stdout), so callers that handle it still leave the cause.
    """
    command = list(cmd)
    if not command:
        raise ValueError("cmd must not be empty")
    if max_attempts < 1:
        raise ValueError("max_attempts must be positive")
    if retry_backoff_seconds < 0:
        raise ValueError("retry_backoff_seconds must be non-negative")

    run = _run_killing_tree if kill_tree_on_timeout else subprocess.run
    for attempt in range(1, max_attempts + 1):
        try:
            result = run(
                command,
                capture_output=capture_output,
                text=text,
                encoding=encoding,
                errors=errors,
                cwd=cwd,
                env=dict(env) if env is not None else None,
                check=check,
                timeout=timeout,
            )
            if retry_result_if is None or not retry_result_if(result):
                return result
            if attempt >= max_attempts:
                return result
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
            if attempt >= max_attempts or (retry_if is not None and not retry_if(error)):
                print(_describe_failure(error), file=sys.stderr, flush=True)
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
    logcat under a native crash). Decode at call sites with :func:`output_text`.
    """
    return subprocess.run(
        list(cmd),
        capture_output=True,
        cwd=cwd,
        env=dict(env) if env is not None else None,
        timeout=timeout,
        check=check,
    )


def output_text(output: str | bytes | None) -> str:
    """Decode captured output, replacing invalid UTF-8; None becomes an empty string.

    Bytes-mode results are bytes, and ``TimeoutExpired`` output is bytes on POSIX even in text mode.
    """
    if isinstance(output, bytes):
        return output.decode("utf-8", errors="replace")
    return output or ""


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
        encoding="utf-8",
        errors="replace",
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
    bash_pipeline: bool = True,
) -> int:
    """Stream combined output to the console and *output_file*; return the exit code.

    Prefer a Bash pipeline when available so grandchildren retain a real stdout.
    This avoids Gradle/javac deadlocks observed with a Python-owned pipe on
    Windows runners. The direct streaming fallback supports hosts without Bash;
    *bash_pipeline* False selects it for commands Bash would re-quote, such as batch files.
    """
    command = list(cmd)
    log_path = Path(output_file).resolve()
    process_env = dict(env) if env is not None else None
    # Earlier prints must not land after the command's own output.
    sys.stdout.flush()
    bash = shutil.which("bash") if bash_pipeline else None
    if bash:
        if sys.platform == "win32":
            command[0] = command[0].replace("\\", "/")
        quoted_cmd = " ".join(shlex.quote(part) for part in command)
        script = f"set -o pipefail; {quoted_cmd} 2>&1 | tee {shlex.quote(log_path.as_posix())}"
        return subprocess.run(
            [bash, "-c", script], cwd=cwd, env=process_env, check=False
        ).returncode

    # The console gets the command's bytes unchanged: re-encoding its text for a Windows code page
    # could fail. The log holds them decoded as UTF-8.
    console = getattr(sys.stdout, "buffer", None)
    with log_path.open("w", encoding="utf-8", newline="") as log:
        process = subprocess.Popen(
            command,
            cwd=cwd,
            env=process_env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        assert process.stdout is not None
        for raw in process.stdout:
            line = raw.decode("utf-8", errors="replace")
            if console is not None:
                console.write(raw)
                # A piped stdout is block-buffered; flush so the step log streams live.
                console.flush()
            else:
                sys.stdout.write(line)
                sys.stdout.flush()
            log.write(line)
        process.wait()
        return process.returncode
