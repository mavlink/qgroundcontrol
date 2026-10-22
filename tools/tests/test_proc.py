"""Tests for tools/common/proc.py."""

from __future__ import annotations

import subprocess
import sys
from typing import TYPE_CHECKING
from unittest.mock import MagicMock

import pytest
from common import proc
from common.proc import output_text, run_captured, run_tee, run_with_retry

if TYPE_CHECKING:
    from pathlib import Path

    from pytest_subprocess import FakeProcess
    from pytest_subprocess.fake_popen import FakePopen

pytestmark = pytest.mark.windows


@pytest.mark.parametrize(
    ("value", "expected"),
    [(None, ""), (b"caf\xc3\xa9 \xff", "caf\u00e9 \ufffd"), ("already text", "already text")],
)
def test_output_text_accepts_bytes_str_or_none(value: str | bytes | None, expected: str) -> None:
    assert output_text(value) == expected


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


@pytest.fixture
def sleeps(monkeypatch: pytest.MonkeyPatch) -> list[float]:
    """Backoff delays requested by run_with_retry, which then returns without sleeping."""
    delays: list[float] = []
    monkeypatch.setattr(proc.time, "sleep", delays.append)
    return delays


def test_run_with_retry_uses_linear_backoff_and_cleanup_callback(
    fp: FakeProcess, sleeps: list[float], tmp_path: Path
) -> None:
    command = ["uv", "sync"]
    attempts = [
        fp.register(command, returncode=1),
        fp.register(command, returncode=1),
        fp.register(command, stdout="synced\n"),
    ]
    cleanup = MagicMock()

    result = run_with_retry(
        command,
        cwd=tmp_path,
        env={"UV_FROZEN": "1"},
        retry_backoff_seconds=2,
        before_retry=cleanup,
        capture_output=True,
        text=True,
    )

    assert (result.returncode, result.stdout) == (0, "synced\n")
    assert sleeps == [2, 4]
    assert cleanup.call_count == 2
    for attempt in attempts:
        (process,) = attempt.calls
        assert process.args == command
        assert process.kwargs is not None
        assert process.kwargs["cwd"] == tmp_path
        assert process.kwargs["env"] == {"UV_FROZEN": "1"}


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


def test_run_with_retry_exhausts_attempts_and_preserves_check(
    fp: FakeProcess, sleeps: list[float]
) -> None:
    command = ["uv", "sync"]
    fp.register(command, returncode=7, occurrences=2)
    with pytest.raises(subprocess.CalledProcessError) as raised:
        run_with_retry(command, max_attempts=2)
    assert raised.value.returncode == 7
    assert fp.call_count(command) == 2
    assert sleeps == [5]


def test_run_with_retry_retries_timeout(
    fp: FakeProcess, sleeps: list[float], monkeypatch: pytest.MonkeyPatch
) -> None:
    command = ["uv", "sync"]

    def time_out(process: FakePopen) -> None:
        raise subprocess.TimeoutExpired(process.args, 10)

    fp.register(command, callback=time_out)
    fp.register(command)
    # The fake process never sees run()'s timeout, so check that it is forwarded on every attempt.
    run = MagicMock(wraps=subprocess.run)
    monkeypatch.setattr(proc.subprocess, "run", run)

    assert run_with_retry(command, timeout=10).returncode == 0
    assert fp.call_count(command) == 2
    assert [invocation.kwargs["timeout"] for invocation in run.call_args_list] == [10, 10]
    assert sleeps == [5]


def test_run_with_retry_supports_unchecked_result_filter(
    fp: FakeProcess, sleeps: list[float]
) -> None:
    command = ["gh", "api", "rate_limit"]
    fp.register(command, returncode=1, stderr="HTTP 503")
    fp.register(command, stdout="{}")
    result = run_with_retry(
        command,
        check=False,
        capture_output=True,
        text=True,
        retry_result_if=lambda completed: completed.returncode != 0,
    )
    assert (result.returncode, result.stdout) == (0, "{}")
    assert fp.call_count(command) == 2
    assert sleeps == [5]


def test_run_with_retry_returns_last_unchecked_failure(
    fp: FakeProcess, sleeps: list[float]
) -> None:
    command = ["gh", "api", "rate_limit"]
    fp.register(command, returncode=1, stderr="HTTP 503", occurrences=2)
    result = run_with_retry(
        command,
        max_attempts=2,
        check=False,
        capture_output=True,
        text=True,
        retry_result_if=lambda completed: completed.returncode != 0,
    )
    assert (result.returncode, result.stderr) == (1, "HTTP 503")
    assert fp.call_count(command) == 2
    assert sleeps == [5]


def test_run_tee_streams_output_and_returns_exit_code(tmp_path: Path) -> None:
    output = tmp_path / "command.log"
    assert run_tee([sys.executable, "-c", "print('streamed')"], output) == 0
    assert output.read_text(encoding="utf-8").strip() == "streamed"


def test_run_tee_falls_back_without_bash(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    output = tmp_path / "fallback.log"
    monkeypatch.setattr("common.proc.shutil.which", lambda _name: None)
    assert run_tee([sys.executable, "-c", "print('fallback')"], output) == 0
    assert output.read_text(encoding="utf-8").strip() == "fallback"


def test_run_tee_fallback_decodes_utf8_and_replaces_invalid_bytes(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    output = tmp_path / "fallback.log"
    monkeypatch.setattr("common.proc.shutil.which", lambda _name: None)
    script = "import sys; sys.stdout.buffer.write('\\u2713 '.encode() + b'\\x9d\\n')"
    assert run_tee([sys.executable, "-c", script], output) == 0
    assert output.read_text(encoding="utf-8") == "\N{CHECK MARK} \N{REPLACEMENT CHARACTER}\n"


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
