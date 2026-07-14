"""Contracts for CMake CI argument and cache helpers."""

from __future__ import annotations

import sys
from subprocess import CompletedProcess
from typing import TYPE_CHECKING

import pytest
from cmake_helper import detect_jobs, main, split_extra_args

if TYPE_CHECKING:
    from pathlib import Path


_SAMPLE_CACHE = """\
CMAKE_BUILD_TYPE:STRING=Release
QGC_COVERAGE_LINE_THRESHOLD:STRING=42
QGC_ENABLE_GST:BOOL=ON
"""


def _write_cache(directory: Path) -> Path:
    cache = directory / "CMakeCache.txt"
    cache.write_text(_SAMPLE_CACHE)
    return cache


def test_detect_jobs_accepts_positive_values_and_auto(monkeypatch: pytest.MonkeyPatch) -> None:
    for value, expected in (("4", 4), ("16", 16)):
        assert detect_jobs(value) == expected
    monkeypatch.setattr("os.cpu_count", lambda: 8)
    assert detect_jobs("auto") == 8
    monkeypatch.setattr("os.cpu_count", lambda: None)
    assert detect_jobs("auto") == 2
    for value in ("abc", "0", "-1"):
        with pytest.raises(SystemExit):
            detect_jobs(value)


def test_cache_var_command_outputs_value_default_or_required_error(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    gh_output: Path,
) -> None:
    _write_cache(tmp_path)
    cases = [
        (["--name", "CMAKE_BUILD_TYPE"], "Release", "cmake_build_type=Release"),
        (
            ["--name", "ABSENT", "--default", "fallback", "--output-key", "value"],
            "fallback",
            "value=fallback",
        ),
    ]
    for args, stdout, output in cases:
        gh_output.write_text("")
        monkeypatch.setattr(sys, "argv", ["prog", "cache-var", "--build-dir", str(tmp_path), *args])
        main()
        assert capsys.readouterr().out.strip() == stdout
        assert output in gh_output.read_text()

    monkeypatch.setattr(
        sys,
        "argv",
        ["prog", "cache-var", "--build-dir", str(tmp_path), "--name", "ABSENT", "--required"],
    )
    with pytest.raises(SystemExit) as error:
        main()
    assert error.value.code == 1


def test_configure_preserves_quoted_extra_arguments(monkeypatch: pytest.MonkeyPatch) -> None:
    commands: list[list[str]] = []

    def run(command: list[str], **_kwargs) -> CompletedProcess[str]:
        commands.append(command)
        return CompletedProcess(command, 0)

    monkeypatch.setattr("cmake_helper.subprocess.run", run)
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "prog",
            "configure",
            "--source-dir",
            ".",
            "--build-dir",
            "build",
            "--extra-args",
            '-DNAME="hello world" -DOTHER=ON',
        ],
    )
    with pytest.raises(SystemExit) as error:
        main()
    assert error.value.code == 0
    separator = commands[0].index("--")
    assert commands[0][separator + 1 :] == ["-DNAME=hello world", "-DOTHER=ON"]


def test_configure_uses_preset_with_only_ci_overrides(monkeypatch: pytest.MonkeyPatch) -> None:
    commands: list[list[str]] = []

    def run(command: list[str], **_kwargs) -> CompletedProcess[str]:
        commands.append(command)
        return CompletedProcess(command, 0)

    monkeypatch.setattr("cmake_helper.subprocess.run", run)
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "prog",
            "configure",
            "--source-dir",
            "/workspace",
            "--build-dir",
            "/tmp/build",
            "--preset",
            "Linux-debug",
            "--stable",
            "--extra-args",
            '-DNAME="hello world" -DOTHER=ON',
        ],
    )
    with pytest.raises(SystemExit) as error:
        main()
    assert error.value.code == 0
    assert commands == [
        [
            "cmake",
            "--preset",
            "Linux-debug",
            "-S",
            "/workspace",
            "-B",
            "/tmp/build",
            "-DQGC_STABLE_BUILD=ON",
            "-DNAME=hello world",
            "-DOTHER=ON",
        ]
    ]


def test_split_extra_args_preserves_windows_paths_and_quoted_values() -> None:
    arguments = split_extra_args(
        r'-DCMAKE_TOOLCHAIN_FILE=D:\a\Qt\qt.toolchain.cmake -DNAME="hello world"',
        windows=True,
    )
    assert arguments == [
        r"-DCMAKE_TOOLCHAIN_FILE=D:\a\Qt\qt.toolchain.cmake",
        "-DNAME=hello world",
    ]


def test_ctest_sharding_omits_end_and_rejects_invalid_index(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    commands: list[list[str]] = []
    monkeypatch.setattr(
        "cmake_helper.run_tee",
        lambda command, _output: commands.append(command) or 0,
    )
    monkeypatch.setattr("cmake_helper._maybe_wrap_xvfb", lambda command: command)

    base = [
        "prog",
        "ctest",
        "--junit-output",
        "junit.xml",
        "--ctest-output",
        "ctest.txt",
        "--jobs",
        "2",
        "--shard-count",
        "3",
    ]
    monkeypatch.setattr(sys, "argv", [*base, "--shard-index", "1"])
    with pytest.raises(SystemExit) as error:
        main()
    assert error.value.code == 0
    assert commands[0][commands[0].index("-I") + 1] == "2,,3"

    monkeypatch.setattr(sys, "argv", [*base, "--shard-index", "3"])
    with pytest.raises(SystemExit) as error:
        main()
    assert error.value.code == 1
