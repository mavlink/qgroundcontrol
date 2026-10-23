"""Tests for tools/common/proc.py."""

from __future__ import annotations

import io
import subprocess
import sys
from typing import TYPE_CHECKING
from unittest.mock import MagicMock

import pytest
from common import proc
from common.proc import (
    MissingToolError,
    output_text,
    require_tools,
    run_captured,
    run_tee,
    run_with_retry,
    wait_until,
)

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


GH_PULLS = ["gh", "api", "repos/mavlink/qgroundcontrol/pulls"]


def test_run_with_retry_logs_the_command_and_its_stderr_tail_before_raising(
    fp: FakeProcess, sleeps: list[float], capsys: pytest.CaptureFixture[str]
) -> None:
    stderr = [f"trace {number}" for number in range(12)] + ["gh: Not Found (HTTP 404)"]
    fp.register(GH_PULLS, returncode=1, stdout='{"message":"Not Found"}', stderr=stderr)
    with pytest.raises(subprocess.CalledProcessError):
        run_with_retry(GH_PULLS, max_attempts=1, capture_output=True, text=True)
    assert capsys.readouterr().err.splitlines() == [
        "Command failed with exit status 1: gh api repos/mavlink/qgroundcontrol/pulls",
        "stderr (last 10 lines):",
        *(f"  trace {number}" for number in range(3, 12)),
        "  gh: Not Found (HTTP 404)",
    ]


def test_run_with_retry_logs_stdout_when_stderr_is_empty_and_redacts_credentials(
    fp: FakeProcess, sleeps: list[float], capsys: pytest.CaptureFixture[str]
) -> None:
    command = ["git", "clone", "https://x-access-token:ghs_secret@github.com/o/r.git"]
    fp.register(command, returncode=128, stdout="curl -H 'Authorization: Bearer ghs_secret'\n")
    with pytest.raises(subprocess.CalledProcessError):
        run_with_retry(command, max_attempts=1, capture_output=True, text=True)
    err = capsys.readouterr().err
    assert "ghs_secret" not in err
    assert err.splitlines() == [
        "Command failed with exit status 128: git clone https://***@github.com/o/r.git",
        "stdout (last 1 lines):",
        "  curl -H 'Authorization: Bearer ***'",
    ]


def test_run_with_retry_logs_only_the_failure_it_raises(
    fp: FakeProcess, sleeps: list[float], capsys: pytest.CaptureFixture[str]
) -> None:
    fp.register(GH_PULLS, returncode=1, stderr="HTTP 503")
    fp.register(GH_PULLS, stdout="[]")
    assert run_with_retry(GH_PULLS, capture_output=True, text=True).stdout == "[]"
    assert "Command failed with exit status" not in capsys.readouterr().err


def test_run_with_retry_can_kill_each_timed_out_attempts_tree(
    monkeypatch: pytest.MonkeyPatch, sleeps: list[float]
) -> None:
    timeouts: list[object] = []

    def run(command: list[str], **kwargs: object) -> subprocess.CompletedProcess[bytes]:
        timeouts.append(kwargs["timeout"])
        if len(timeouts) == 1:
            raise subprocess.TimeoutExpired(command, 10)
        return subprocess.CompletedProcess(command, 0)

    monkeypatch.setattr(proc, "_run_killing_tree", run)
    assert run_with_retry(["installer"], timeout=10, kill_tree_on_timeout=True).returncode == 0
    assert timeouts == [10, 10]
    assert sleeps == [5]


def test_tree_killing_run_matches_subprocess_run() -> None:
    script = "import sys; print('out'); print('err', file=sys.stderr); sys.exit(3)"
    command = [sys.executable, "-c", script]
    result = proc._run_killing_tree(command, capture_output=True, text=True, timeout=30)
    assert (result.returncode, result.stdout, result.stderr) == (3, "out\n", "err\n")
    with pytest.raises(subprocess.CalledProcessError):
        proc._run_killing_tree(command, capture_output=True, check=True, timeout=30)


def _process_gone(pid: int) -> bool:
    """A process that no longer exists or is only a zombie awaiting its reaper."""
    state = subprocess.run(["ps", "-o", "stat=", "-p", str(pid)], capture_output=True, text=True)
    return state.returncode != 0 or state.stdout.strip().startswith("Z")


@pytest.mark.skipif(sys.platform == "win32", reason="POSIX process groups; Windows uses taskkill")
def test_timeout_kills_grandchildren(tmp_path: Path) -> None:
    pid_file = tmp_path / "grandchild.pid"
    sleeper = [sys.executable, "-c", "import time; time.sleep(60)"]
    script = (
        "import pathlib, subprocess, sys, time\n"
        f"child = subprocess.Popen({sleeper!r})\n"
        f"pathlib.Path({str(pid_file)!r}).write_text(str(child.pid))\n"
        "time.sleep(60)\n"
    )
    with pytest.raises(subprocess.TimeoutExpired):
        run_with_retry(
            [sys.executable, "-c", script], max_attempts=1, timeout=3, kill_tree_on_timeout=True
        )
    grandchild = int(pid_file.read_text())
    wait_until(lambda: _process_gone(grandchild), timeout=10, interval=0.1)


def test_windows_kills_the_tree_with_taskkill(monkeypatch: pytest.MonkeyPatch) -> None:
    commands: list[list[str]] = []
    monkeypatch.setattr(proc.sys, "platform", "win32")
    monkeypatch.setattr(proc.subprocess, "run", lambda command, **_kwargs: commands.append(command))
    process = MagicMock(pid=1234)

    proc.kill_process_tree(process)

    assert commands == [["taskkill", "/T", "/F", "/PID", "1234"]]
    process.kill.assert_called_once_with()


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


class _FlushRecorder(io.BytesIO):
    """A console byte stream that records what it held at each flush."""

    def __init__(self) -> None:
        super().__init__()
        self.flushed: list[bytes] = []

    def flush(self) -> None:
        self.flushed.append(self.getvalue().replace(b"\r\n", b"\n"))


class _Console:
    """A text stdout over a recording byte buffer that the command's output must bypass."""

    def __init__(self) -> None:
        self.buffer = _FlushRecorder()

    def write(self, text: str) -> int:
        raise AssertionError(f"re-encoded the command's output: {text!r}")

    def flush(self) -> None:
        pass


def test_run_tee_without_bash_pipeline_streams_each_line_live(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """Configure runs qt-cmake.bat on Windows, which a Bash pipeline would re-quote."""
    monkeypatch.setattr("common.proc.shutil.which", lambda _name: pytest.fail("looked up bash"))
    console = _Console()
    monkeypatch.setattr(sys, "stdout", console)
    output = tmp_path / "configure.log"
    script = "print('-- first'); print('-- second'); raise SystemExit(3)"

    assert run_tee([sys.executable, "-c", script], output, bash_pipeline=False) == 3
    assert output.read_text(encoding="utf-8") == "-- first\n-- second\n"
    assert b"-- first\n" in console.buffer.flushed
    assert console.buffer.flushed[-1] == b"-- first\n-- second\n"


def test_run_tee_without_bash_pipeline_passes_console_bytes_through(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """A Windows code page byte (cp1252 em dash) must not fail the step by re-encoding."""
    console = _Console()
    monkeypatch.setattr(sys, "stdout", console)
    output = tmp_path / "configure.log"
    script = "import sys; sys.stdout.buffer.write(b'-- QGC: Hardening \\x97 \\xe2\\x9c\\x93\\n')"

    assert run_tee([sys.executable, "-c", script], output, bash_pipeline=False) == 0
    assert console.buffer.getvalue() == b"-- QGC: Hardening \x97 \xe2\x9c\x93\n"
    assert (
        output.read_text(encoding="utf-8")
        == "-- QGC: Hardening \N{REPLACEMENT CHARACTER} \N{CHECK MARK}\n"
    )


def test_run_tee_without_a_byte_console_writes_decoded_text(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    stdout = io.StringIO()
    monkeypatch.setattr(sys, "stdout", stdout)
    script = "import sys; sys.stdout.buffer.write(b'caf\\xc3\\xa9\\n')"
    assert run_tee([sys.executable, "-c", script], tmp_path / "log", bash_pipeline=False) == 0
    assert stdout.getvalue() == "caf\N{LATIN SMALL LETTER E WITH ACUTE}\n"


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


def test_require_tools_returns_each_tool_path(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("common.proc.shutil.which", lambda name: f"/usr/bin/{name}")
    assert require_tools("git", "cmake") == {"git": "/usr/bin/git", "cmake": "/usr/bin/cmake"}


def test_require_tools_names_every_missing_tool_and_the_hint(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.setattr(
        "common.proc.shutil.which", lambda name: "/usr/bin/git" if name == "git" else None
    )
    with pytest.raises(MissingToolError) as raised:
        require_tools("cmake", "git", "ninja", hint="Install the build group")
    assert str(raised.value) == "cmake, ninja not found on PATH. Install the build group"
    assert isinstance(raised.value, FileNotFoundError)


def test_require_tools_without_hint(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("common.proc.shutil.which", lambda _name: None)
    with pytest.raises(MissingToolError) as raised:
        require_tools("gcovr")
    assert str(raised.value) == "gcovr not found on PATH"


class FakeClock:
    """Monotonic clock that advances only when the code under test sleeps."""

    def __init__(self) -> None:
        self.now = 100.0
        self.sleeps: list[float] = []

    def __call__(self) -> float:
        return self.now

    def sleep(self, seconds: float) -> None:
        self.sleeps.append(seconds)
        self.now += seconds


def test_wait_until_returns_the_first_truthy_result() -> None:
    clock = FakeClock()
    results = iter([None, "", "ready"])
    result = wait_until(
        lambda: next(results), timeout=10, interval=2, sleep=clock.sleep, clock=clock
    )
    assert result == "ready"
    assert clock.sleeps == [2, 2]


def test_wait_until_polls_at_the_deadline_before_timing_out() -> None:
    clock = FakeClock()
    polls: list[float] = []

    def never() -> bool:
        polls.append(clock.now)
        return False

    with pytest.raises(TimeoutError, match=r"^Timed out after 5s waiting for emulator boot$"):
        wait_until(
            never,
            timeout=5,
            interval=2,
            description="emulator boot",
            sleep=clock.sleep,
            clock=clock,
        )
    assert clock.sleeps == [2, 2, 1]
    assert polls == [100, 102, 104, 105]


def test_wait_until_with_zero_timeout_polls_once() -> None:
    clock = FakeClock()
    polls: list[float] = []
    with pytest.raises(TimeoutError, match="condition"):
        wait_until(
            lambda: polls.append(clock.now), timeout=0, interval=1, sleep=clock.sleep, clock=clock
        )
    assert polls == [100]
    assert clock.sleeps == []


def test_wait_until_propagates_predicate_errors() -> None:
    clock = FakeClock()

    def stopped() -> bool:
        raise RuntimeError("stopped")

    with pytest.raises(RuntimeError, match="stopped"):
        wait_until(stopped, timeout=5, interval=1, sleep=clock.sleep, clock=clock)
    assert clock.sleeps == []


def test_wait_until_resolves_time_functions_at_call_time(monkeypatch: pytest.MonkeyPatch) -> None:
    clock = FakeClock()
    monkeypatch.setattr(proc.time, "monotonic", clock)
    monkeypatch.setattr(proc.time, "sleep", clock.sleep)
    results = iter([False, True])
    assert wait_until(lambda: next(results), timeout=1, interval=0.25) is True
    assert clock.sleeps == [0.25]


@pytest.mark.parametrize(("timeout", "interval"), [(-1, 1), (1, 0), (1, -1)])
def test_wait_until_rejects_invalid_limits(timeout: float, interval: float) -> None:
    with pytest.raises(ValueError, match="timeout must be non-negative and interval positive"):
        wait_until(lambda: True, timeout=timeout, interval=interval)
