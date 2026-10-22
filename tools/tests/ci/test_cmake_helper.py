"""Tests for cmake_helper.py."""

from __future__ import annotations

import json
import subprocess
from pathlib import Path
from typing import TYPE_CHECKING
from unittest.mock import patch

import pytest
from ci.compile.cmake_helper import _maybe_wrap_xvfb, detect_jobs, main

from .._helpers import REPO_ROOT

if TYPE_CHECKING:
    from qgc_tools.cmake_configure import CMakeConfig

pytestmark = pytest.mark.windows


class TestDetectJobs:
    def test_explicit_value(self) -> None:
        assert detect_jobs("4") == 4

    def test_explicit_value_large(self) -> None:
        assert detect_jobs("16") == 16

    def test_auto_uses_cpu_count(self) -> None:
        with patch("os.cpu_count", return_value=8):
            assert detect_jobs("auto") == 8

    def test_auto_fallback_none(self) -> None:
        with patch("os.cpu_count", return_value=None):
            assert detect_jobs("auto") == 2

    def test_invalid_exits(self) -> None:
        with pytest.raises(SystemExit):
            detect_jobs("abc")

    def test_zero_exits(self) -> None:
        with pytest.raises(SystemExit):
            detect_jobs("0")

    def test_negative_exits(self) -> None:
        with pytest.raises(SystemExit):
            detect_jobs("-1")


def _run_configure(
    monkeypatch: pytest.MonkeyPatch, venv_python: Path, *arguments: str
) -> CMakeConfig:
    configs: list[CMakeConfig] = []

    def fake_configure(config: CMakeConfig) -> int:
        configs.append(config)
        return 0

    monkeypatch.setattr("ci.compile.cmake_helper.configure", fake_configure)
    monkeypatch.setattr("ci.compile.cmake_helper.python_env.executable", lambda _name: venv_python)
    monkeypatch.setattr("sys.argv", ["cmake_helper.py", "configure", *arguments])
    with pytest.raises(SystemExit) as exc:
        main()
    assert exc.value.code == 0
    (config,) = configs
    return config


@pytest.fixture
def venv_python(tmp_path: Path) -> Path:
    python = tmp_path / "venv" / "bin" / "python"
    python.parent.mkdir(parents=True)
    python.touch(mode=0o755)
    return python


@pytest.fixture(autouse=True)
def _clear_action_inputs(monkeypatch: pytest.MonkeyPatch) -> None:
    for name in [*CONFIGURE_INPUTS, *BUILD_INPUTS, "QGC_CACHE_PROGRAM"]:
        monkeypatch.delenv(name, raising=False)


CONFIGURE_INPUTS = {
    "INPUT_BUILD_DIR": "out",
    "INPUT_BUILD_TYPE": "Debug",
    "INPUT_PRESET": "Linux-sanitizers",
    "INPUT_TESTING": "true",
    "INPUT_STABLE": "true",
    "INPUT_EXTRA_ARGS": "-DA=1\n-DB=2",
    "INPUT_USE_QT_CMAKE": "false",
}
BUILD_INPUTS = {
    "INPUT_TARGET": "autogen",
    "INPUT_BUILD_TYPE": "Debug",
    "INPUT_PARALLEL": "true",
    "INPUT_PARALLEL_JOBS": "3",
    "INPUT_OUTPUT_FILE": "",
    "INPUT_REVIEWDOG": "false",
}


def test_configure_defaults_to_release_in_the_checkout(
    monkeypatch: pytest.MonkeyPatch, venv_python: Path
) -> None:
    config = _run_configure(monkeypatch, venv_python)

    assert config.source_dir == REPO_ROOT
    assert config.build_dir == Path("build")
    assert config.build_type == "Release"
    assert config.preset is None
    assert config.use_qt_cmake
    assert not (config.testing or config.stable or config.coverage)
    assert config.extra_args == [f"-DPython3_EXECUTABLE={venv_python}"]


@pytest.mark.parametrize("cache_program", ["", "/installed tools/bin/ccache"])
def test_configure_reads_action_inputs_and_appends_ci_cache_arguments(
    monkeypatch: pytest.MonkeyPatch, venv_python: Path, cache_program: str
) -> None:
    for name, value in CONFIGURE_INPUTS.items():
        monkeypatch.setenv(name, value)
    monkeypatch.setenv("QGC_CACHE_PROGRAM", cache_program)

    config = _run_configure(monkeypatch, venv_python)

    assert config.build_dir == Path("out")
    assert config.build_type == "Debug"
    assert config.preset == "Linux-sanitizers"
    assert config.testing and config.stable
    assert not config.use_qt_cmake
    expected = ["-DA=1", "-DB=2", f"-DPython3_EXECUTABLE={venv_python}"]
    if cache_program:
        expected.append(f"-DQGC_CACHE_PROGRAM:FILEPATH={cache_program}")
    assert config.extra_args == expected


def test_configure_command_line_overrides_action_inputs(
    monkeypatch: pytest.MonkeyPatch, venv_python: Path
) -> None:
    monkeypatch.setenv("INPUT_PRESET", "Linux")
    config = _run_configure(monkeypatch, venv_python, "--preset", "Linux-debug", "--no-qt-cmake")
    assert config.preset == "Linux-debug"
    assert not config.use_qt_cmake


def test_configure_warns_without_locked_python(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    missing = tmp_path / "missing" / "python"
    config = _run_configure(monkeypatch, missing)

    assert config.extra_args == []
    assert f"::warning::venv python not found at {missing}" in capsys.readouterr().out


def test_configure_propagates_failure(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("ci.compile.cmake_helper.configure", lambda _config: 3)
    monkeypatch.setattr("sys.argv", ["cmake_helper.py", "configure"])
    with pytest.raises(SystemExit) as exc:
        main()
    assert exc.value.code == 3


def test_build_reads_action_inputs(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    monkeypatch.chdir(tmp_path)
    monkeypatch.delenv("GITHUB_OUTPUT", raising=False)
    for name, value in BUILD_INPUTS.items():
        monkeypatch.setenv(name, value)
    commands: list[list[str]] = []

    def fake_run(command: list[str], **_kwargs: object) -> subprocess.CompletedProcess[list[str]]:
        commands.append(command)
        return subprocess.CompletedProcess(command, 0)

    monkeypatch.setattr("ci.compile.cmake_helper.subprocess.run", fake_run)
    monkeypatch.setattr("sys.argv", ["cmake_helper.py", "build"])
    main()

    assert commands == [
        ["cmake", "--build", ".", "--target", "autogen", "--config", "Debug", "--parallel", "3"]
    ]


def _run_ctest(monkeypatch: pytest.MonkeyPatch, *arguments: str, exit_code: int = 0) -> list[str]:
    invocation: list[str] = []

    def fake_run_tee(command: list[str], output_file: str) -> int:
        assert output_file == "output.txt"
        invocation.extend(command)
        return exit_code

    monkeypatch.setattr("ci.compile.cmake_helper.run_tee", fake_run_tee)
    monkeypatch.setattr("ci.compile.cmake_helper._maybe_wrap_xvfb", lambda command: command)
    monkeypatch.setattr(
        "sys.argv",
        [
            "cmake_helper.py",
            "ctest",
            "--junit-output",
            "junit.xml",
            "--ctest-output",
            "output.txt",
            *arguments,
        ],
    )
    with pytest.raises(SystemExit) as exc:
        main()
    assert exc.value.code == exit_code
    return invocation


def test_ctest_resolves_auto_jobs_and_forwards_selection(monkeypatch: pytest.MonkeyPatch) -> None:
    with patch("os.cpu_count", return_value=6):
        command = _run_ctest(
            monkeypatch,
            "--build-type",
            "Release",
            "--include-labels",
            "Unit",
            "--exclude-labels",
            "Flaky|Network",
        )
    assert command == [
        "ctest",
        "--output-on-failure",
        "--no-tests=error",
        "--output-junit",
        "junit.xml",
        "--parallel",
        "6",
        "-C",
        "Release",
        "-L",
        "Unit",
        "-LE",
        "Flaky|Network",
    ]


def test_ctest_defaults_to_auto_jobs_and_propagates_failure(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    with patch("os.cpu_count", return_value=3):
        command = _run_ctest(monkeypatch, exit_code=8)
    assert command[command.index("--parallel") + 1] == "3"
    assert "-C" not in command
    assert "-L" not in command


def test_ctest_rejects_invalid_jobs_before_running(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("ci.compile.cmake_helper.run_tee", lambda *_args: pytest.fail("ctest ran"))
    monkeypatch.setattr(
        "sys.argv",
        [
            "cmake_helper.py",
            "ctest",
            "--junit-output",
            "junit.xml",
            "--ctest-output",
            "output.txt",
            "--jobs",
            "0",
        ],
    )
    with pytest.raises(SystemExit) as exc:
        main()
    assert exc.value.code == 1


def test_empty_ctest_selection_fails(tmp_path):
    import sys

    (tmp_path / "CTestTestfile.cmake").touch()
    result = subprocess.run(
        [
            sys.executable,
            str(REPO_ROOT / "tools/ci/compile/cmake_helper.py"),
            "ctest",
            "--junit-output",
            "junit.xml",
            "--ctest-output",
            "output.txt",
            "--jobs",
            "1",
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0
    assert "No tests were found" in result.stdout + result.stderr


@pytest.mark.parametrize("exit_code", [0, 1])
def test_build_checkpoint_reports_actual_compile_result(monkeypatch, tmp_path, exit_code):
    output = tmp_path / "github-output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output))
    monkeypatch.setattr(
        "ci.compile.cmake_helper.subprocess.run",
        lambda command, **kwargs: subprocess.CompletedProcess(command, exit_code),
    )
    monkeypatch.setattr("sys.argv", ["cmake_helper.py", "build"])
    if exit_code:
        with pytest.raises(SystemExit):
            main()
    else:
        main()
    assert f"build_success={str(exit_code == 0).lower()}" in output.read_text()


def test_failed_build_still_preserves_current_invocation_timings(monkeypatch, tmp_path):
    monkeypatch.chdir(tmp_path)
    output = tmp_path / "output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output))
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    log = tmp_path / ".ninja_log"
    old = "# ninja log v5\n0\t100\t1\told.o\ta\n"
    log.write_text(old)

    def build(command, **kwargs):
        log.write_text(old + "0\t250\t2\tnew.o\tb\n")
        return subprocess.CompletedProcess(command, 1)

    monkeypatch.setattr("ci.compile.cmake_helper.subprocess.run", build)
    monkeypatch.setattr("sys.argv", ["cmake_helper.py", "build"])
    with pytest.raises(SystemExit) as error:
        main()
    assert error.value.code == 1
    reports = list(tmp_path.glob("build-profile-*/report.json"))
    assert len(reports) == 1
    report = json.loads(reports[0].read_text())
    assert report["edge_count"] == 1
    assert report["slowest_edges"][0]["output"] == "new.o"
    assert "profile_path=" in output.read_text()
    assert "build_success=false" in output.read_text()


@pytest.mark.parametrize(
    ("xvfb", "expected", "annotation"),
    [
        ("/usr/bin/xvfb-run", ["/usr/bin/xvfb-run", "-a", "ctest"], "::notice::No DISPLAY"),
        (None, ["ctest"], "::warning::xvfb-run not found"),
    ],
)
def test_headless_linux_ctest_wraps_xvfb_with_annotations(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    xvfb: str | None,
    expected: list[str],
    annotation: str,
) -> None:
    monkeypatch.setattr("ci.compile.cmake_helper.sys.platform", "linux")
    monkeypatch.delenv("DISPLAY", raising=False)
    monkeypatch.setattr("ci.compile.cmake_helper.shutil.which", lambda _name: xvfb)

    assert _maybe_wrap_xvfb(["ctest"]) == expected
    assert capsys.readouterr().out.startswith(annotation)
