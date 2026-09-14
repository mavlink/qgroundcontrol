"""Tests for cmake_helper.py."""

from __future__ import annotations

import json
import subprocess
from typing import TYPE_CHECKING
from unittest.mock import patch

import pytest
from cmake_helper import detect_jobs, main
from common.cmake import read_cache_var

if TYPE_CHECKING:
    from pathlib import Path


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


@pytest.mark.parametrize("cache_program", ["", "/installed tools/bin/ccache"])
def test_configure_forwards_explicit_preset_and_cache_binary(
    monkeypatch: pytest.MonkeyPatch, cache_program: str
) -> None:
    invocation: list[str] = []
    monkeypatch.setenv("QGC_CACHE_PROGRAM", cache_program)

    def fake_run(command: list[str], **_kwargs: object) -> subprocess.CompletedProcess[list[str]]:
        invocation.extend(command)
        return subprocess.CompletedProcess(command, 0)

    monkeypatch.setattr("cmake_helper.subprocess.run", fake_run)
    monkeypatch.setattr(
        "sys.argv",
        [
            "cmake_helper.py",
            "configure",
            "--source-dir",
            ".",
            "--build-dir",
            "build",
            "--preset",
            "Linux-debug",
        ],
    )

    with pytest.raises(SystemExit) as exc:
        main()

    assert exc.value.code == 0
    tail = ["--preset", "Linux-debug"]
    if cache_program:
        tail += ["--", f"-DQGC_CACHE_PROGRAM:FILEPATH={cache_program}"]
    assert invocation[-len(tail) :] == tail


_SAMPLE_CACHE = """\
# This is the CMakeCache file.
//Build type
CMAKE_BUILD_TYPE:STRING=Release
QGC_COVERAGE_LINE_THRESHOLD:STRING=42
QGC_COVERAGE_BRANCH_THRESHOLD:STRING=23
QGC_ENABLE_GST:BOOL=ON
//Comment
"""


def _write_cache(tmp_path: Path) -> Path:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text(_SAMPLE_CACHE)
    return cache


class TestReadCacheVar:
    def test_returns_string_value(self, tmp_path: Path) -> None:
        cache = _write_cache(tmp_path)
        assert read_cache_var(str(cache), "QGC_COVERAGE_LINE_THRESHOLD") == "42"

    def test_returns_bool_value(self, tmp_path: Path) -> None:
        cache = _write_cache(tmp_path)
        assert read_cache_var(str(cache), "QGC_ENABLE_GST") == "ON"

    def test_missing_var_returns_none(self, tmp_path: Path) -> None:
        cache = _write_cache(tmp_path)
        assert read_cache_var(str(cache), "ABSENT_VAR") is None

    def test_missing_file_returns_none(self, tmp_path: Path) -> None:
        assert read_cache_var(str(tmp_path / "nope.txt"), "X") is None

    def test_partial_name_does_not_match(self, tmp_path: Path) -> None:
        cache = _write_cache(tmp_path)
        assert read_cache_var(str(cache), "QGC_COVERAGE_LINE") is None


class TestCmdCacheVar:
    def test_main_prints_and_writes_output(
        self, tmp_path: Path, monkeypatch, capsys, gh_output: Path
    ) -> None:
        _write_cache(tmp_path)
        monkeypatch.setattr(
            "sys.argv",
            ["prog", "cache-var", "--build-dir", str(tmp_path), "--name", "CMAKE_BUILD_TYPE"],
        )
        main()
        assert capsys.readouterr().out.strip() == "Release"
        assert "cmake_build_type=Release" in gh_output.read_text()

    def test_main_uses_default_when_missing(
        self, tmp_path: Path, monkeypatch, capsys, gh_output: Path
    ) -> None:
        _write_cache(tmp_path)
        monkeypatch.setattr(
            "sys.argv",
            [
                "prog",
                "cache-var",
                "--build-dir",
                str(tmp_path),
                "--name",
                "ABSENT",
                "--default",
                "fallback",
                "--output-key",
                "value",
            ],
        )
        main()
        assert capsys.readouterr().out.strip() == "fallback"
        assert "value=fallback" in gh_output.read_text()

    def test_main_required_missing_exits(self, tmp_path: Path, monkeypatch) -> None:
        _write_cache(tmp_path)
        monkeypatch.setattr(
            "sys.argv",
            ["prog", "cache-var", "--build-dir", str(tmp_path), "--name", "ABSENT", "--required"],
        )
        with pytest.raises(SystemExit) as exc:
            main()
        assert exc.value.code == 1


def test_empty_ctest_selection_fails(tmp_path):
    import sys

    from _helpers import REPO_ROOT

    (tmp_path / "CTestTestfile.cmake").touch()
    result = subprocess.run(
        [
            sys.executable,
            str(REPO_ROOT / ".github/scripts/cmake_helper.py"),
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
@pytest.mark.parametrize("continue_on_error", [False, True])
def test_build_checkpoint_reports_actual_compile_result(
    monkeypatch, tmp_path, exit_code, continue_on_error
):
    output = tmp_path / "github-output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output))
    monkeypatch.setattr(
        "cmake_helper.subprocess.run",
        lambda command, **kwargs: subprocess.CompletedProcess(command, exit_code),
    )
    arguments = ["cmake_helper.py", "build"]
    if continue_on_error:
        arguments.append("--continue-on-error")
    monkeypatch.setattr("sys.argv", arguments)
    if exit_code and not continue_on_error:
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

    monkeypatch.setattr("cmake_helper.subprocess.run", build)
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
