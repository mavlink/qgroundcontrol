"""SITL readiness and container ownership without starting Docker."""

import json
import re
import subprocess
from pathlib import Path

import pytest
from simulation import run_arducopter_sitl


@pytest.fixture
def sitl():
    return run_arducopter_sitl


def test_existing_unowned_container_is_untouched(sitl, monkeypatch):
    calls = []

    def run(command, **kwargs):
        calls.append(command)
        stdout = (
            json.dumps([{"Config": {"Labels": None}}])
            if "inspect" in command
            else "arducopter-sitl\n"
        )
        return subprocess.CompletedProcess(command, 0, stdout)

    monkeypatch.setattr(sitl.subprocess, "run", run)
    assert sitl.main([]) == 1
    assert not any("rm" in call or "run" in call for call in calls)


@pytest.mark.parametrize("latency", [False, True])
def test_startup_failure_removes_only_created_container(sitl, monkeypatch, latency):
    calls = []

    def run(command, **kwargs):
        calls.append(command)
        return subprocess.CompletedProcess(
            command, 0, "created-id\n" if command[1] == "run" else ""
        )

    def fail(*args):
        raise TimeoutError("not ready")

    monkeypatch.setattr(sitl.subprocess, "run", run)
    monkeypatch.setattr(sitl, "wait_ready", fail)
    assert sitl.main(["--with-latency"] if latency else []) == 1
    assert calls[-1] == ["docker", "rm", "-f", "created-id"]
    start = next(call for call in calls if call[1] == "run")
    assert "127.0.0.1:5760:5760" in start
    assert ("--cap-add=NET_ADMIN" in start) == latency
    if latency:
        assert "-ec" in start
        assert "|| true" not in start[-1]


def test_readiness_waits_for_listening_socket_without_connecting(sitl, monkeypatch):
    monkeypatch.setattr(sitl, "inspect", lambda _: {"State": {"Running": True}})
    responses = iter(["0: 00000000:1680 00000000:0000 01", "0: 00000000:1680 00000000:0000 0A"])
    monkeypatch.setattr(
        sitl.subprocess,
        "run",
        lambda command, **kwargs: subprocess.CompletedProcess(command, 0, next(responses)),
    )
    sleeps = []
    monkeypatch.setattr("common.proc.time.sleep", sleeps.append)
    sitl.wait_ready("owned-id", 10)
    assert len(sleeps) == 1


def test_stopped_container_fails_without_waiting(sitl, monkeypatch):
    monkeypatch.setattr(sitl, "inspect", lambda _: {"State": {"Running": False, "ExitCode": 7}})
    with pytest.raises(RuntimeError, match="7"):
        sitl.wait_ready("owned-id", 10)


def test_image_build_uses_pinned_dockerfile_commit(sitl, monkeypatch):
    calls = []

    def run(command, **kwargs):
        calls.append(command)
        missing_image = command[1:3] == ["image", "inspect"]
        return subprocess.CompletedProcess(
            command, int(missing_image), "created-id\n" if command[1] == "run" else ""
        )

    monkeypatch.setattr(sitl.subprocess, "run", run)
    monkeypatch.setattr(sitl, "wait_ready", lambda container, timeout: None)
    assert sitl.main([]) == 0
    build = next(call for call in calls if call[1] == "build")
    assert build[-1] == sitl.BUILD_CONTEXT
    assert re.fullmatch(
        r"https://github\.com/radarku/ardupilot-sitl-docker\.git#[0-9a-f]{40}", sitl.BUILD_CONTEXT
    )


class FakeProcess:
    """Writes one line of SITL's log per poll, then exits with ``exit_code`` if given.

    ``wait`` raises KeyboardInterrupt, as Ctrl+C does, unless ``exit_on_wait`` gives SITL's own exit
    code and last log line.
    """

    def __init__(self, log, lines, exit_code=None, exit_on_wait=None):
        self.log = log
        self.lines = list(lines)
        self.exit_code = exit_code
        self.exit_on_wait = exit_on_wait
        self.returncode = None
        self.killed = False

    def _write(self, line):
        with self.log.open("a", encoding="utf-8") as output:
            output.write(line + "\n")

    def poll(self):
        if self.returncode is None:
            if self.lines:
                self._write(self.lines.pop(0))
            elif self.exit_code is not None:
                self.returncode = self.exit_code
        return self.returncode

    def wait(self):
        if self.returncode is not None:
            return self.returncode
        if self.exit_on_wait is None:
            raise KeyboardInterrupt
        self.returncode, line = self.exit_on_wait
        self._write(line)
        return self.returncode

    def kill(self):
        self.killed = True
        self.returncode = -9


SITL_READY = ["bind port 5760 for SERIAL0", "SERIAL0 on TCP port 5760"]


def _fake_popen(launches, **process_args):
    def popen(command, **kwargs):
        process = FakeProcess(Path(kwargs["stdout"].name), **process_args)
        launches.append((command, kwargs, process))
        return process

    return popen


@pytest.fixture
def ardupilot(tmp_path, sitl, monkeypatch):
    binary = tmp_path / "ardupilot" / "build" / "sitl" / "bin" / "arducopter"
    binary.parent.mkdir(parents=True)
    binary.touch()
    workdir = tmp_path / "work"
    workdir.mkdir()
    monkeypatch.setattr(sitl.tempfile, "mkdtemp", lambda prefix: str(workdir))
    clock = [0.0]
    monkeypatch.setattr("common.proc.time.monotonic", lambda: clock[0])
    monkeypatch.setattr(
        "common.proc.time.sleep", lambda seconds: clock.__setitem__(0, clock[0] + seconds)
    )
    return tmp_path / "ardupilot"


@pytest.mark.parametrize("relative", [False, True], ids=["absolute", "relative"])
def test_native_runs_build_until_interrupted(sitl, ardupilot, monkeypatch, capsys, relative):
    launches = []
    monkeypatch.setattr(sitl.subprocess, "Popen", _fake_popen(launches, lines=SITL_READY))
    monkeypatch.setattr(sitl.subprocess, "run", lambda *args, **kwargs: pytest.fail("ran Docker"))

    if relative:
        monkeypatch.chdir(ardupilot.parent)
    assert sitl.main(["--native", ardupilot.name if relative else str(ardupilot)]) == 0
    [(command, kwargs, process)] = launches
    checkout = ardupilot.resolve()
    assert command == sitl.sitl_command(
        str(checkout / "build" / "sitl" / "bin" / "arducopter"),
        str(checkout / "Tools" / "autotest" / "default_params" / "copter.parm"),
    )
    assert kwargs["cwd"] == ardupilot.parent / "work"
    # Ctrl+C stops SITL with the launcher, which SITL needs to keep running
    assert process.killed
    assert "Stop: Ctrl+C" in capsys.readouterr().out


def test_native_reports_sitl_exit_after_startup(sitl, ardupilot, monkeypatch, capsys):
    # Another SITL instance holds a later serial port
    launches = []
    bind_failure = "bind failed on port 5762 - Address already in use"
    monkeypatch.setattr(
        sitl.subprocess,
        "Popen",
        _fake_popen(launches, lines=SITL_READY, exit_on_wait=(1, bind_failure)),
    )

    assert sitl.main(["--native", str(ardupilot)]) == 1
    assert f"SITL exited with 1 ({bind_failure})" in capsys.readouterr().err


@pytest.mark.parametrize(
    ("lines", "exit_code", "message", "killed"),
    [
        (
            ["bind failed on port 5760 - Address already in use"],
            1,
            "SITL exited with 1 (bind failed on port 5760",
            False,
        ),
        (["bind port 5760 for SERIAL0"], None, "Timed out", True),
    ],
    ids=["port-busy", "timeout"],
)
def test_native_startup_failures(
    sitl, ardupilot, monkeypatch, capsys, lines, exit_code, message, killed
):
    launches = []
    monkeypatch.setattr(
        sitl.subprocess, "Popen", _fake_popen(launches, lines=lines, exit_code=exit_code)
    )

    assert sitl.main(["--native", str(ardupilot), "--timeout", "1"]) == 1
    [(_, _, process)] = launches
    assert process.killed == killed
    assert message in capsys.readouterr().err


def test_native_needs_a_build(sitl, tmp_path, capsys):
    assert sitl.main(["--native", str(tmp_path)]) == 1
    assert "./waf configure --board sitl" in capsys.readouterr().err


def test_native_has_no_latency_simulation(sitl, tmp_path):
    with pytest.raises(SystemExit):
        sitl.main(["--native", str(tmp_path), "--with-latency"])
