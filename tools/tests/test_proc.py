"""Tests for tools/common/proc.py."""

from __future__ import annotations

import subprocess
import sys
from typing import TYPE_CHECKING
from unittest.mock import MagicMock, call, patch

import pytest
from common import proc
from common.proc import run_captured, run_tee, run_text, run_with_retry

if TYPE_CHECKING:
    from pathlib import Path


def test_run_captured_returns_completed_process() -> None:
    result = run_captured([sys.executable, "-c", "print('hello')"])
    assert result.returncode == 0
    assert result.stdout.strip() == "hello"
    assert isinstance(result.stdout, str)


def test_run_captured_replaces_invalid_utf8() -> None:
    result = run_captured([sys.executable, "-c", "import sys; sys.stdout.buffer.write(b'\\x9d')"])
    assert result.returncode == 0
    assert result.stdout == "\N{REPLACEMENT CHARACTER}"


def test_run_captured_check_raises() -> None:
    with pytest.raises(subprocess.CalledProcessError):
        run_captured([sys.executable, "-c", "raise SystemExit(1)"], check=True)


def test_run_captured_failing_command_does_not_raise_by_default() -> None:
    result = run_captured([sys.executable, "-c", "raise SystemExit(1)"])
    assert result.returncode != 0


def test_run_captured_input() -> None:
    result = run_captured(
        [sys.executable, "-c", "import sys; print(sys.stdin.read(), end='')"],
        input_text="payload\n",
    )
    assert result.stdout == "payload\n"


def test_run_with_retry_uses_linear_backoff_and_cleanup_callback() -> None:
    command = ["uv", "sync"]
    cleanup = MagicMock()
    with (
        patch.object(
            proc.subprocess,
            "run",
            side_effect=[
                subprocess.CalledProcessError(1, command),
                subprocess.CalledProcessError(1, command),
                subprocess.CompletedProcess(command, 0),
            ],
        ) as mock_run,
        patch.object(proc.time, "sleep") as mock_sleep,
    ):
        result = run_with_retry(command, retry_backoff_seconds=2, before_retry=cleanup)

    assert result.returncode == 0
    assert mock_run.call_count == 3
    assert mock_sleep.call_args_list == [call(2), call(4)]
    assert cleanup.call_count == 2


@pytest.mark.parametrize(
    ("kwargs", "message"),
    [
        ({"max_attempts": 0}, "max_attempts must be positive"),
        ({"retry_backoff_seconds": -1}, "retry_backoff_seconds must be non-negative"),
    ],
)
def test_run_with_retry_rejects_invalid_limits(kwargs: dict, message: str) -> None:
    with pytest.raises(ValueError, match=message):
        run_with_retry(["true"], **kwargs)


def test_run_with_retry_rejects_empty_command() -> None:
    with pytest.raises(ValueError, match="cmd must not be empty"):
        run_with_retry([])


def test_run_with_retry_exhausts_attempts_and_preserves_check() -> None:
    command = ["uv", "sync"]
    failure = subprocess.CalledProcessError(7, command)
    with (
        patch.object(proc.subprocess, "run", side_effect=failure) as run,
        patch.object(proc.time, "sleep"),
        pytest.raises(subprocess.CalledProcessError) as raised,
    ):
        run_with_retry(command, max_attempts=2)
    assert raised.value.returncode == 7
    assert run.call_count == 2
    assert all(invocation.kwargs["check"] is True for invocation in run.call_args_list)


def test_run_with_retry_retries_timeout() -> None:
    command = ["uv", "sync"]
    timeout = subprocess.TimeoutExpired(command, 10)
    success = subprocess.CompletedProcess(command, 0)
    with (
        patch.object(proc.subprocess, "run", side_effect=[timeout, success]) as run,
        patch.object(proc.time, "sleep"),
    ):
        assert run_with_retry(command, timeout=10).returncode == 0
    assert run.call_count == 2
    assert all(invocation.kwargs["timeout"] == 10 for invocation in run.call_args_list)


def test_run_with_retry_supports_unchecked_result_filter() -> None:
    command = ["gh", "api", "rate_limit"]
    transient = subprocess.CompletedProcess(command, 1)
    success = subprocess.CompletedProcess(command, 0)
    with (
        patch.object(proc.subprocess, "run", side_effect=[transient, success]) as run,
        patch.object(proc.time, "sleep"),
    ):
        result = run_with_retry(
            command,
            check=False,
            retry_result_if=lambda completed: completed.returncode != 0,
        )
    assert result.returncode == 0
    assert run.call_count == 2
    assert all(invocation.kwargs["check"] is False for invocation in run.call_args_list)


def test_run_text_returns_stdout() -> None:
    assert run_text([sys.executable, "-c", "print('hi')"]) == "hi"


def test_run_text_default_on_missing_binary() -> None:
    assert run_text(["this-binary-does-not-exist"], default="fallback") == "fallback"


def test_run_text_default_on_nonzero_exit() -> None:
    assert run_text([sys.executable, "-c", "raise SystemExit(1)"], default="fb") == "fb"


def test_run_tee_streams_output_and_returns_exit_code(tmp_path: Path) -> None:
    output = tmp_path / "command.log"
    assert run_tee([sys.executable, "-c", "print('streamed')"], output) == 0
    assert output.read_text(encoding="utf-8").strip() == "streamed"


def test_run_tee_falls_back_without_bash(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    output = tmp_path / "fallback.log"
    monkeypatch.setattr("common.proc.shutil.which", lambda _name: None)
    assert run_tee([sys.executable, "-c", "print('fallback')"], output) == 0
    assert output.read_text(encoding="utf-8").strip() == "fallback"


def test_run_tee_resolves_relative_output_before_changing_cwd(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    invocation_dir = tmp_path / "invocation"
    working_dir = tmp_path / "working"
    invocation_dir.mkdir()
    working_dir.mkdir()
    monkeypatch.chdir(invocation_dir)

    assert run_tee([sys.executable, "-c", "print('relative')"], "command.log", cwd=working_dir) == 0
    assert (invocation_dir / "command.log").read_text(encoding="utf-8").strip() == "relative"
    assert not (working_dir / "command.log").exists()
