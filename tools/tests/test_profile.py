"""Profiling command and failure contracts without running a real profiler."""

import importlib.util
import subprocess
from pathlib import Path

import pytest


def load():
    spec = importlib.util.spec_from_file_location(
        "qgc_profile", Path(__file__).parents[1] / "debuggers/profile.py"
    )
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@pytest.mark.parametrize("mode", ["perf", "memcheck", "callgrind", "massif", "heaptrack"])
def test_profiler_preserves_arguments_and_propagates_failure(tmp_path, monkeypatch, mode):
    module = load()
    binary = tmp_path / "build/Debug/QGroundControl"
    binary.parent.mkdir(parents=True)
    binary.touch()
    binary.chmod(0o755)
    calls = []

    def run(command, **kwargs):
        calls.append(command)
        return subprocess.CompletedProcess(command, 9)

    monkeypatch.setattr(module.shutil, "which", lambda name: name)
    monkeypatch.setattr(module.subprocess, "run", run)
    assert (
        module.main(
            [
                f"--{mode}",
                "-b",
                str(binary.parents[1]),
                "--output-dir",
                str(tmp_path / "profile"),
                "--",
                "--settings",
                "path with spaces",
                "$(literal)",
            ]
        )
        == 9
    )
    assert len(calls) == 1
    assert calls[0][-4:] == [str(binary), "--settings", "path with spaces", "$(literal)"]


def test_sanitizer_finds_configuration_binary(tmp_path, monkeypatch):
    module = load()
    binary = tmp_path / "Debug/QGroundControl"
    binary.parent.mkdir()
    binary.touch()
    binary.chmod(0o755)
    calls = []

    def run(command, **kwargs):
        calls.append((command, kwargs))
        return subprocess.CompletedProcess(command, 0)

    monkeypatch.setattr(module.subprocess, "run", run)
    assert module.main(["--sanitize", "-b", str(tmp_path), "--", "one argument"]) == 0
    assert calls[-1][0] == [str(binary), "one argument"]
    assert "ASAN_OPTIONS" in calls[-1][1]["env"]
    assert "-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer" in calls[0][0]
