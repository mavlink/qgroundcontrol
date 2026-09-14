"""SITL readiness and container ownership without starting Docker."""

import importlib.util
import json
import subprocess
from pathlib import Path

import pytest


@pytest.fixture
def sitl():
    spec = importlib.util.spec_from_file_location(
        "sitl", Path(__file__).parents[1] / "simulation/run_arducopter_sitl.py"
    )
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


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
    monkeypatch.setattr(sitl.time, "sleep", sleeps.append)
    sitl.wait_ready("owned-id", 10)
    assert len(sleeps) == 1


def test_stopped_container_fails_without_waiting(sitl, monkeypatch):
    monkeypatch.setattr(sitl, "inspect", lambda _: {"State": {"Running": False, "ExitCode": 7}})
    with pytest.raises(RuntimeError, match="7"):
        sitl.wait_ready("owned-id", 10)
