"""Profiling command and failure contracts without running a real profiler."""

import subprocess

import pytest

from ._helpers import load_script


def load():
    return load_script("tools/debuggers/profile.py")


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
    configure = calls[0][0]
    assert configure[-2:] == ["-DQGC_ENABLE_ASAN=ON", "-DQGC_ENABLE_UBSAN=ON"]
    assert not any("-fsanitize" in argument for argument in configure)


def test_sanitizer_default_build_dir_is_under_build(monkeypatch):
    module = load()
    calls = []

    def run(command, **kwargs):
        calls.append(command)
        raise subprocess.CalledProcessError(1, command)

    monkeypatch.setattr(module.subprocess, "run", run)
    assert module.main(["--sanitize"]) != 0
    assert calls[0][calls[0].index("-B") + 1] == str(module.REPO_ROOT / "build" / "sanitize")


def test_missing_profiler_fails_before_recording(tmp_path, monkeypatch, capsys):
    module = load()
    binary = tmp_path / "build/Debug/QGroundControl"
    binary.parent.mkdir(parents=True)
    binary.touch()
    binary.chmod(0o755)
    monkeypatch.setattr(module.shutil, "which", lambda name: None)
    monkeypatch.setattr(module.subprocess, "run", pytest.fail)
    assert module.main(["--heaptrack", "-b", str(binary.parents[1])]) == 1
    assert "Profiling failed: heaptrack not found on PATH" in capsys.readouterr().err
