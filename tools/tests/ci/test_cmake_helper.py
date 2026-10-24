"""Tests for cmake_helper.py."""

from __future__ import annotations

import json
import os
import re
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import TYPE_CHECKING
from unittest.mock import patch

import pytest
from ci.compile.cmake_helper import (
    INCLUDE_PATH_VARIABLES,
    PACKAGE_INVENTORIES,
    BuildRun,
    _maybe_wrap_xvfb,
    configure_checks,
    counter_delta,
    detect_jobs,
    main,
    system_header_digest,
    write_build_summary,
)

from .._helpers import REPO_ROOT, get_step, get_steps, load_action, read_github_file
from ..test_build_errors import (
    GCC_BUILD,
    GRADLE_BUILD,
    VALIJSON_CLONE_CONFIGURE,
    ZSTD_ASM_CONFIGURE,
)

if TYPE_CHECKING:
    from qgc_tools.cmake_configure import CMakeConfig
    from syrupy.assertion import SnapshotAssertion

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

    @pytest.mark.parametrize("requested", ["abc", "0", "-1"])
    def test_invalid_raises(self, requested: str) -> None:
        with pytest.raises(ValueError, match="Invalid parallel job count"):
            detect_jobs(requested)


def _run_configure(
    monkeypatch: pytest.MonkeyPatch, venv_python: Path, *arguments: str
) -> CMakeConfig:
    configs: list[CMakeConfig] = []

    def fake_configure(config: CMakeConfig, *, log_file: Path) -> int:
        assert config.build_dir is not None
        assert log_file == config.build_dir / "cmake-configure.log"
        configs.append(config)
        return 0

    monkeypatch.setattr("ci.compile.cmake_helper.configure", fake_configure)
    monkeypatch.setattr("ci.compile.cmake_helper.python_env.executable", lambda _name: venv_python)
    assert main(["configure", *arguments]) == 0
    (config,) = configs
    return config


@pytest.fixture
def venv_python(tmp_path: Path) -> Path:
    python = tmp_path / "venv" / "bin" / "python"
    python.parent.mkdir(parents=True)
    python.touch(mode=0o755)
    return python


@pytest.fixture(autouse=True)
def _isolated_build_summary(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    """`build` writes the Build summary and its log; keep them off the checkout and host ccache."""
    monkeypatch.setenv("MOCCACHE_DIR", str(tmp_path / "moccache"))
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    monkeypatch.setattr("ci.compile.cmake_helper.ccache_stats", lambda: None)


@pytest.fixture(autouse=True)
def _clear_action_inputs(monkeypatch: pytest.MonkeyPatch) -> None:
    for name in [
        *CONFIGURE_INPUTS,
        *BUILD_INPUTS,
        *CTEST_INPUTS,
        "QGC_CACHE_PROGRAM",
        "QGC_CHECK_RESULT_CACHE_DIR",
        "QGC_CHECK_RESULT_CACHE_SALT",
    ]:
        monkeypatch.delenv(name, raising=False)


CONFIGURE_INPUTS = {
    "INPUT_BUILD_DIR": "out",
    "INPUT_PRESET": "Linux-sanitizers",
    "INPUT_EXTRA_ARGS": "-DA=1\n-DB=2",
    "INPUT_USE_QT_CMAKE": "false",
}
BUILD_INPUTS = {
    "INPUT_TARGET": "autogen",
    "INPUT_PARALLEL": "true",
    "INPUT_PARALLEL_JOBS": "3",
    "INPUT_OUTPUT_FILE": "",
    "INPUT_REVIEWDOG": "false",
}
CTEST_INPUTS = {
    "INPUT_JUNIT_OUTPUT": "junit.xml",
    "INPUT_CTEST_OUTPUT": "output.txt",
    "INPUT_PARALLEL": "2",
    "INPUT_INCLUDE_LABELS": "Integration",
    "INPUT_EXCLUDE_LABELS": "Flaky|Network",
}
STABLE_REF = {"GITHUB_REF_TYPE": "tag", "GITHUB_REF": "refs/tags/v5.0.0"}


def test_configure_defaults_to_the_checkout_and_leaves_testing_to_the_preset(
    monkeypatch: pytest.MonkeyPatch, venv_python: Path
) -> None:
    config = _run_configure(monkeypatch, venv_python, "--preset", "Linux")

    assert config.source_dir == REPO_ROOT
    assert config.build_dir == Path("build")
    assert config.preset == "Linux"
    assert config.use_qt_cmake
    assert not (config.testing or config.stable)
    assert config.extra_args == [f"-DPython3_EXECUTABLE={venv_python}"]


def test_configure_requires_a_preset(
    monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    monkeypatch.setattr("ci.compile.cmake_helper.configure", lambda _config: pytest.fail("ran"))
    assert main(["configure"]) == 1
    assert "::error::configure needs a CMake preset" in capsys.readouterr().out


@pytest.mark.parametrize("cache_program", ["", "/installed tools/bin/ccache"])
def test_configure_reads_action_inputs_and_appends_ci_cache_arguments(
    monkeypatch: pytest.MonkeyPatch, venv_python: Path, cache_program: str
) -> None:
    for name, value in {**CONFIGURE_INPUTS, **STABLE_REF}.items():
        monkeypatch.setenv(name, value)
    monkeypatch.setenv("QGC_CACHE_PROGRAM", cache_program)

    config = _run_configure(monkeypatch, venv_python)

    assert config.build_dir == Path("out")
    assert config.preset == "Linux-sanitizers"
    assert config.stable and not config.testing
    assert not config.use_qt_cmake
    expected = ["-DA=1", "-DB=2", f"-DPython3_EXECUTABLE={venv_python}"]
    if cache_program:
        expected.append(f"-DQGC_CACHE_PROGRAM:FILEPATH={cache_program}")
    assert config.extra_args == expected


def test_configure_reuses_check_results_from_the_cache_plan(
    monkeypatch: pytest.MonkeyPatch, venv_python: Path
) -> None:
    monkeypatch.setenv("QGC_CHECK_RESULT_CACHE_DIR", "/work/.cache/ccache/qgc-configure-checks")
    monkeypatch.setenv("QGC_CHECK_RESULT_CACHE_SALT", "cpm=abc,image=ubuntu24-20261019.1")
    monkeypatch.setattr("ci.compile.cmake_helper.system_header_digest", lambda: "0123abcd")

    config = _run_configure(monkeypatch, venv_python, "--preset", "Linux")

    assert config.extra_args[-2:] == [
        "-DQGC_CHECK_RESULT_CACHE_DIR:PATH=/work/.cache/ccache/qgc-configure-checks",
        "-DQGC_CHECK_RESULT_CACHE_SALT:STRING=cpm=abc,image=ubuntu24-20261019.1,system=0123abcd",
    ]


class _FakeInventory:
    """Stands in for the package manager that system_header_digest queries."""

    def __init__(self, monkeypatch: pytest.MonkeyPatch, tool: str | None, stdout: str = "") -> None:
        self.stdout = stdout
        self.commands: list[tuple[str, ...]] = []
        monkeypatch.setattr(
            "ci.compile.cmake_helper.shutil.which", lambda name: name if name == tool else None
        )
        monkeypatch.setattr("ci.compile.cmake_helper.subprocess.run", self.run)
        monkeypatch.setattr("ci.compile.cmake_helper.HOMEBREW_CELLARS", ())
        for name in INCLUDE_PATH_VARIABLES:
            monkeypatch.delenv(name, raising=False)

    def run(self, command, **_kwargs) -> subprocess.CompletedProcess[str]:
        self.commands.append(tuple(command))
        return subprocess.CompletedProcess(command, 0, self.stdout, "")


def test_system_header_digest_tracks_installed_packages(monkeypatch: pytest.MonkeyPatch) -> None:
    inventory = _FakeInventory(monkeypatch, "dpkg-query", "libc6-dev=2.39\nlibudev-dev=255\n")
    with_udev = system_header_digest()
    assert inventory.commands == [PACKAGE_INVENTORIES[0]]

    inventory.stdout = "libudev-dev=255\nlibc6-dev=2.39\n"
    assert system_header_digest() == with_udev, "package order must not matter"

    inventory.stdout = "libc6-dev=2.39\n"
    assert system_header_digest() != with_udev


def test_system_header_digest_tracks_include_paths_and_homebrew(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    _FakeInventory(monkeypatch, None)
    baseline = system_header_digest()

    monkeypatch.setenv(
        "INCLUDE", r"C:\Program Files (x86)\Windows Kits\10\include\10.0.26100.0\ucrt"
    )
    windows_sdk = system_header_digest()
    assert windows_sdk != baseline

    cellar = tmp_path / "Cellar"
    (cellar / "libusb/1.0.29").mkdir(parents=True)
    monkeypatch.setattr("ci.compile.cmake_helper.HOMEBREW_CELLARS", (cellar,))
    assert system_header_digest() != windows_sdk


@pytest.mark.parametrize(
    ("ref_type", "ref", "stable"),
    [
        ("tag", "refs/tags/v5.0.0", True),
        ("branch", "refs/heads/Stable_V5.0", True),
        ("branch", "refs/heads/master", False),
        ("branch", "refs/heads/fix-unstable-test", False),
        ("branch", "refs/heads/stable-docs", False),
        ("branch", "refs/pull/7/merge", False),
    ],
)
def test_configure_stable_build_follows_release_refs(
    monkeypatch: pytest.MonkeyPatch, venv_python: Path, ref_type: str, ref: str, stable: bool
) -> None:
    monkeypatch.setenv("GITHUB_REF_TYPE", ref_type)
    monkeypatch.setenv("GITHUB_REF", ref)
    assert _run_configure(monkeypatch, venv_python, "--preset", "Linux").stable is stable


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
    config = _run_configure(monkeypatch, missing, "--preset", "Linux")

    assert config.extra_args == []
    assert f"::warning::venv python not found at {missing}" in capsys.readouterr().out


def test_configure_propagates_failure(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    monkeypatch.chdir(tmp_path)
    monkeypatch.setattr("ci.compile.cmake_helper.configure", lambda _config, **_kwargs: 3)
    assert main(["configure", "--preset", "Linux"]) == 3


def test_build_reads_action_inputs(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    monkeypatch.chdir(tmp_path)
    for name, value in BUILD_INPUTS.items():
        monkeypatch.setenv(name, value)
    build = _fake_build(monkeypatch, 0)
    assert main(["build"]) == 0

    assert build.commands == [["cmake", "--build", ".", "--target", "autogen", "--parallel", "3"]]
    assert build.logs == [str(tmp_path / "build-output.log")]


@pytest.mark.parametrize(
    ("cache", "explicit", "expected"),
    [
        # Xcode (iOS-appstore) and other multi-config trees build the preset's configuration.
        (
            "CMAKE_BUILD_TYPE:STRING=Release\nCMAKE_CONFIGURATION_TYPES:STRING=Debug;Release\n",
            "",
            "Release",
        ),
        # Single-config (Ninja) trees ignore --config and -C, so none is passed.
        ("CMAKE_BUILD_TYPE:STRING=Release\n", "", None),
        (None, "", None),
        (
            "CMAKE_BUILD_TYPE:STRING=Release\nCMAKE_CONFIGURATION_TYPES:STRING=Debug;Release\n",
            "Debug",
            "Debug",
        ),
    ],
)
def test_build_and_ctest_select_a_multi_config_trees_build_type(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
    cache: str | None,
    explicit: str,
    expected: str | None,
) -> None:
    monkeypatch.chdir(tmp_path)
    if cache is not None:
        (tmp_path / "CMakeCache.txt").write_text(f"# comment\n{cache}")
    selection = ["--build-type", explicit] if explicit else []
    fake = _fake_build(monkeypatch, 0)
    assert main(["build", *selection]) == 0
    (build,) = fake.commands

    ctest = _run_ctest(monkeypatch, *selection)

    assert build[3:] == (["--config", expected] if expected else [])
    assert ("-C" in ctest) == bool(expected)
    if expected:
        assert ctest[ctest.index("-C") + 1] == expected


def _run_ctest(monkeypatch: pytest.MonkeyPatch, *arguments: str, exit_code: int = 0) -> list[str]:
    invocation: list[str] = []

    def fake_run_tee(command: list[str], output_file: str) -> int:
        assert output_file == "output.txt"
        invocation.extend(command)
        return exit_code

    monkeypatch.setattr("ci.compile.cmake_helper.run_tee", fake_run_tee)
    monkeypatch.setattr("ci.compile.cmake_helper._maybe_wrap_xvfb", lambda command: command)
    ctest = ["ctest", "--junit-output", "junit.xml", "--ctest-output", "output.txt", *arguments]
    assert main(ctest) == exit_code
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


def test_ctest_reads_action_inputs(monkeypatch: pytest.MonkeyPatch) -> None:
    for name, value in CTEST_INPUTS.items():
        monkeypatch.setenv(name, value)
    invocation: list[str] = []

    def fake_run_tee(command: list[str], output_file: str) -> int:
        assert output_file == "output.txt"
        invocation.extend(command)
        return 0

    monkeypatch.setattr("ci.compile.cmake_helper.run_tee", fake_run_tee)
    monkeypatch.setattr("ci.compile.cmake_helper._maybe_wrap_xvfb", lambda command: command)
    assert main(["ctest"]) == 0
    assert invocation[3:] == [
        "--output-junit",
        "junit.xml",
        "--parallel",
        "2",
        "-L",
        "Integration",
        "-LE",
        "Flaky|Network",
    ]


def test_ctest_requires_its_output_files(capsys: pytest.CaptureFixture[str]) -> None:
    with pytest.raises(SystemExit) as exc:
        main(["ctest", "--junit-output", "junit.xml"])
    assert exc.value.code == 2
    assert "--junit-output and --ctest-output" in capsys.readouterr().err


def test_ctest_rejects_invalid_jobs_before_running(
    monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    monkeypatch.setattr("ci.compile.cmake_helper.run_tee", lambda *_args: pytest.fail("ctest ran"))
    ctest = ["ctest", "--junit-output", "junit.xml", "--ctest-output", "output.txt", "--jobs", "0"]
    assert main(ctest) == 1
    assert "::error::Invalid parallel job count: 0" in capsys.readouterr().out


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
def test_build_checkpoint_reports_actual_compile_result(
    monkeypatch, tmp_path, gh_output, exit_code
):
    monkeypatch.chdir(tmp_path)
    _fake_build(monkeypatch, exit_code)
    assert main(["build"]) == exit_code
    assert f"build_success={str(exit_code == 0).lower()}" in gh_output.read_text()


def test_failed_build_still_preserves_current_invocation_timings(monkeypatch, tmp_path, gh_output):
    monkeypatch.chdir(tmp_path)
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    log = tmp_path / ".ninja_log"
    old = "# ninja log v5\n0\t100\t1\told.o\ta\n"
    log.write_text(old)

    def build(command, output_file):
        log.write_text(old + "0\t250\t2\tnew.o\tb\n")
        return 1

    monkeypatch.setattr("ci.compile.cmake_helper.run_tee", build)
    assert main(["build"]) == 1
    reports = list(tmp_path.glob("build-profile-*/report.json"))
    assert len(reports) == 1
    report = json.loads(reports[0].read_text())
    assert report["edge_count"] == 1
    assert report["slowest_edges"][0]["output"] == "new.o"
    assert "profile_path=" in gh_output.read_text()
    assert "build_success=false" in gh_output.read_text()


def _build_tree(build_dir: Path) -> Path:
    (build_dir / "CMakeFiles" / "4.1.2").mkdir(parents=True)
    (build_dir / "CMakeCache.txt").write_text(
        "CMAKE_BUILD_TYPE:STRING=Debug\nCMAKE_GENERATOR:INTERNAL=Ninja\nQGC_UNITY_BUILD:BOOL=ON\n"
    )
    (build_dir / "CMakeFiles" / "4.1.2" / "CMakeCXXCompiler.cmake").write_text(
        'set(CMAKE_CXX_COMPILER "/usr/bin/c++")\n'
        'set(CMAKE_CXX_COMPILER_ID "GNU")\n'
        'set(CMAKE_CXX_COMPILER_VERSION "13.3.0")\n'
    )
    profile = build_dir / "profile"
    profile.mkdir()
    (profile / "report.json").write_text(
        json.dumps(
            {
                "edge_count": 2345,
                "categories": {
                    "compile": {"count": 2100, "duration_ms": 5_400_000},
                    "link/archive": {"count": 12, "duration_ms": 95_000},
                    "autogen/moc": {"count": 233, "duration_ms": 120_000},
                },
                "slowest_edges": [
                    {
                        "output": "src/Vehicle/CMakeFiles/Vehicle.dir/Vehicle.cc.o",
                        "duration_ms": 61_200,
                        "category": "compile",
                    },
                    {
                        "output": "Debug/QGroundControl",
                        "duration_ms": 41_500,
                        "category": "link/archive",
                    },
                ],
            }
        )
    )
    return profile


CCACHE_JSON = {
    "direct_cache_hit": 1900,
    "preprocessed_cache_hit": 34,
    "cache_miss": 66,
    "cache_size_kibibyte": 1_258_291,
    "max_cache_size_kibibyte": 3_145_728,
    "cleanups_performed": 0,
}


def test_summary_writes_one_build_section(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_summary: Path, snapshot: SnapshotAssertion
) -> None:
    profile = _build_tree(tmp_path)
    monkeypatch.setattr("ci.compile.cmake_helper.ccache_stats", lambda: CCACHE_JSON)
    monkeypatch.setattr(
        "ci.compile.cmake_helper.collect_stats",
        lambda build_dir: type(
            "Stats",
            (),
            {
                "hits": 812,
                "misses": 8,
                "scope": "this build",
                "counts": {"miss-first-seen": 5, "miss-include-changed": 3},
                "size": lambda self: "212 MiB of 1.0 GiB",
            },
        )(),
    )
    write_build_summary(tmp_path, "all", "passed", BuildRun(754, str(profile)))
    assert gh_summary.read_text(encoding="utf-8") == snapshot


@dataclass
class _FakeBuild:
    """Stands in for run_tee: records each build and writes *output* to its log."""

    result: int | BaseException
    output: str = ""
    commands: list[list[str]] = field(default_factory=list)
    logs: list[str] = field(default_factory=list)

    def __call__(self, command: list[str], output_file: str) -> int:
        self.commands.append(command)
        self.logs.append(output_file)
        if isinstance(self.result, BaseException):
            raise self.result
        Path(output_file).write_text(self.output, encoding="utf-8")
        return self.result


def _fake_build(
    monkeypatch: pytest.MonkeyPatch, result: int | BaseException, output: str = ""
) -> _FakeBuild:
    fake = _FakeBuild(result, output)
    monkeypatch.setattr("ci.compile.cmake_helper.run_tee", fake)
    return fake


@pytest.mark.parametrize(("exit_code", "result"), [(0, "passed"), (2, "failed")])
def test_build_writes_its_summary_section(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    gh_summary: Path,
    exit_code: int,
    result: str,
) -> None:
    monkeypatch.chdir(tmp_path)
    _fake_build(monkeypatch, exit_code)
    assert main(["build", "--target", "qgc-analysis-headers"]) == exit_code
    summary = gh_summary.read_text(encoding="utf-8")
    assert summary.startswith("### Build (qgc-analysis-headers)\n")
    assert f"| Result | {result} |\n| Duration | " in summary


def test_build_that_cannot_start_still_reports_its_failure(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_summary: Path
) -> None:
    monkeypatch.chdir(tmp_path)
    _fake_build(monkeypatch, FileNotFoundError("cmake"))
    with pytest.raises(FileNotFoundError):
        main(["build"])
    assert gh_summary.read_text(encoding="utf-8") == (
        "### Build\n\n| Item | Value |\n| --- | --- |\n| Result | failed |\n\n"
    )


def test_cancelled_build_writes_no_summary(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_summary: Path
) -> None:
    monkeypatch.chdir(tmp_path)
    _fake_build(monkeypatch, KeyboardInterrupt())
    with pytest.raises(KeyboardInterrupt):
        main(["build"])
    assert gh_summary.read_text(encoding="utf-8") == ""


def test_summary_failure_only_warns(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    gh_output: Path,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    monkeypatch.chdir(tmp_path)
    _fake_build(monkeypatch, 0)
    monkeypatch.setattr(
        "ci.compile.cmake_helper.collect_stats",
        lambda _build_dir: (_ for _ in ()).throw(OSError("unreadable")),
    )
    assert main(["build"]) == 0
    assert "build_success=true" in gh_output.read_text()
    assert gh_summary.read_text(encoding="utf-8") == ""
    assert "::warning::Cannot write the Build summary: unreadable" in capsys.readouterr().out


def test_moc_cache_statistics_default_to_the_checkout(monkeypatch: pytest.MonkeyPatch) -> None:
    """Without MOCCACHE_DIR, the summary reads the cache the CMake moc launcher writes."""
    from moccache.core import _cache_dir

    monkeypatch.delenv("MOCCACHE_DIR")
    assert _cache_dir() == REPO_ROOT / ".cache" / "moccache"
    build = get_step(get_steps(load_action("cmake-build")), "compile")
    assert "MOCCACHE_DIR" not in build["env"]


def test_summary_reports_sccache_and_its_error_log(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_summary: Path
) -> None:
    log = tmp_path / "sccache.log"
    log.write_text("".join(f"line {number}\n" for number in range(60)))
    monkeypatch.setenv("QGC_COMPILER_CACHE_TOOL", "sccache")
    monkeypatch.setenv("SCCACHE_ERROR_LOG", str(log))
    report = {
        "stats": {"cache_hits": {"counts": {"C/C++": 95}}, "cache_misses": {"counts": {"C/C++": 5}}}
    }
    monkeypatch.setattr("ci.compile.cmake_helper.sccache_stats", lambda: report)
    monkeypatch.setattr("ci.compile.cmake_helper.ccache_stats", lambda: pytest.fail("ccache read"))
    write_build_summary(tmp_path, "", "passed", BuildRun())
    summary = gh_summary.read_text(encoding="utf-8")
    assert (
        "| Compiler cache | sccache: 95 of 100 (95.0%) hits, 0 writes, 0 not cacheable |" in summary
    )
    assert "<details><summary>sccache error log (last 50 lines)</summary>" in summary
    assert "line 59" in summary
    assert "line 9\n" not in summary


@pytest.mark.parametrize(
    ("xvfb", "expected", "annotation"),
    [
        ("/usr/bin/xvfb-run", ["/usr/bin/xvfb-run", "-a", "ctest"], "No DISPLAY; running CTest"),
        (None, ["ctest"], "::warning::xvfb-run not found"),
    ],
)
def test_headless_linux_ctest_wraps_xvfb(
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


def _annotations(output: str) -> list[str]:
    return [line for line in output.splitlines() if line.startswith("::")]


def _configure_with_output(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
    venv_python: Path,
    output: str,
    exit_code: int,
) -> int:
    """Run `configure` against a fake CMake that writes *output* and a cache, then exits."""

    def fake_configure(config: CMakeConfig, *, log_file: Path) -> int:
        assert config.build_dir is not None
        log_file.parent.mkdir(parents=True, exist_ok=True)
        log_file.write_text(output, encoding="utf-8")
        (config.build_dir / "CMakeCache.txt").write_text(
            "CMAKE_GENERATOR:INTERNAL=Ninja\nCMAKE_BUILD_TYPE:STRING=Release\n"
        )
        return exit_code

    monkeypatch.setenv("GITHUB_JOB", "build")
    monkeypatch.setattr("ci.compile.cmake_helper.configure", fake_configure)
    monkeypatch.setattr("ci.compile.cmake_helper.python_env.executable", lambda _name: venv_python)
    return main(["configure", "--preset", "Windows", "--build-dir", str(tmp_path / "build")])


def test_failed_configure_annotates_and_summarizes_the_first_cmake_error(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
    venv_python: Path,
    gh_output: Path,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
    snapshot: SnapshotAssertion,
) -> None:
    assert _configure_with_output(monkeypatch, tmp_path, venv_python, ZSTD_ASM_CONFIGURE, 1) == 1

    out = capsys.readouterr().out
    assert _annotations(out) == [
        "::error file=.cache/CPM/zstd/dfd2/build/cmake/CMakeLists.txt,line=36,"
        "title=CMake configure failed::No CMAKE_ASM_COMPILER could be found.%0A%0A"
        "Tell CMake where to find the compiler by setting either the environment%0A"
        'variable "ASM" or the CMake cache entry CMAKE_ASM_COMPILER to the full path%0A'
        "to the compiler, or to the compiler name if it is in the PATH."
    ]
    assert "Configure failed after 0s" in out
    assert "completed" not in out
    assert gh_summary.read_text(encoding="utf-8") == snapshot
    artifact = read_github_file(gh_output)["logs_artifact"]
    assert re.fullmatch(r"cmake-configure-build-Windows-[0-9a-f]{6}", artifact)


def test_failed_configure_outside_the_checkout_names_the_location(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
    venv_python: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    _configure_with_output(monkeypatch, tmp_path, venv_python, VALIJSON_CLONE_CONFIGURE, 1)
    assert _annotations(capsys.readouterr().out) == [
        "::error title=CMake configure failed::/usr/local/share/cmake-4.1/Modules/ExternalProject/"
        "shared_internal_commands.cmake:943: Failed to clone repository:%0A"
        "'https://github.com/tristanpenman/valijson.git'"
    ]


def test_failed_configure_without_a_cmake_error_quotes_the_end_of_its_output(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
    venv_python: Path,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    output = "".join(f"-- step {n}\n" for n in range(12)) + "Segmentation fault (core dumped)\n"
    assert _configure_with_output(monkeypatch, tmp_path, venv_python, output, 139) == 139
    (annotation,) = _annotations(capsys.readouterr().out)
    assert annotation.split("%0A") == [
        "::error title=CMake configure failed::CMake exited with status 139:",
        *(f"-- step {n}" for n in range(3, 12)),
        "Segmentation fault (core dumped)",
    ]
    summary = gh_summary.read_text(encoding="utf-8")
    assert "<details><summary>CMake error</summary>" not in summary
    assert "<details><summary>Configure output (last 40 lines)</summary>" in summary


def test_successful_configure_logs_plainly_and_reports_check_reuse(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
    venv_python: Path,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    output = (
        "-- QGC: Reused 1412 configure-check results from /cache/qgc-configure-checks/ab12.cmake\n"
        "-- Configuring done (35.2s)\n"
        "-- QGC: Saved 3 configure-check results to /cache/qgc-configure-checks/ab12.cmake\n"
    )
    assert _configure_with_output(monkeypatch, tmp_path, venv_python, output, 0) == 0
    out = capsys.readouterr().out
    assert "Configure completed in 0s" in out.splitlines()
    assert _annotations(out) == []
    assert gh_summary.read_text(encoding="utf-8") == (
        "### Configure\n\n| Item | Value |\n| --- | --- |\n| Result | passed |\n"
        "| Duration | 0.0s |\n| Preset | `Windows` |\n| Generator | Ninja |\n"
        "| Configure checks | reused 1,412 saved results, saved 3 results |\n\n"
    )


@pytest.mark.parametrize(
    ("line", "expected"),
    [
        (
            "-- QGC: No saved configure-check results for this toolchain; running the checks",
            "none saved for this toolchain",
        ),
        (
            "-- QGC: Ignoring configure-check results with a different fingerprint: /c/x.cmake",
            "saved results ignored (different fingerprint)",
        ),
        ("-- Configuring done", ""),
    ],
)
def test_configure_check_reuse(line: str, expected: str) -> None:
    assert configure_checks(f"{line}\n") == expected


def test_configure_that_raises_still_reports_its_failure_and_logs_artifact(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
    venv_python: Path,
    gh_output: Path,
    gh_summary: Path,
) -> None:
    def missing_cmake(_config: CMakeConfig, *, log_file: Path) -> int:
        raise FileNotFoundError("cmake")

    monkeypatch.setattr("ci.compile.cmake_helper.configure", missing_cmake)
    monkeypatch.setattr("ci.compile.cmake_helper.python_env.executable", lambda _name: venv_python)
    with pytest.raises(FileNotFoundError):
        main(["configure", "--preset", "Linux", "--build-dir", str(tmp_path / "build")])
    assert gh_summary.read_text(encoding="utf-8").startswith(
        "### Configure\n\n| Item | Value |\n| --- | --- |\n| Result | failed |\n| Preset | `Linux` |"
    )
    assert read_github_file(gh_output)["logs_artifact"].startswith("cmake-configure-local-Linux-")


def _from_build_dir(build_dir: Path, path: str) -> str:
    """*path* in the checkout as a compiler run from *build_dir* reports it."""
    try:
        return Path(os.path.relpath(REPO_ROOT / path, build_dir)).as_posix()
    except ValueError:
        # Another Windows drive.
        return (REPO_ROOT / path).as_posix()


def test_failed_build_annotates_and_summarizes_compiler_errors(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
    snapshot: SnapshotAssertion,
) -> None:
    monkeypatch.chdir(tmp_path)
    test_dir = _from_build_dir(tmp_path, "test/MAVLink")
    _fake_build(
        monkeypatch,
        1,
        GCC_BUILD.replace("../../qgroundcontrol/qgroundcontrol/test/MAVLink", test_dir),
    )

    assert main(["build"]) == 1
    out = capsys.readouterr().out
    message = "no matching function for call to \u2018EventHandler::EventHandler(std::nullptr_t, QString)\u2019"
    assert _annotations(out) == [
        f"::error file=test/MAVLink/HealthAndArmingCheckReportTest.cc,line={line},col=31,"
        f"title=Compile error::{message}"
        for line in (83, 144)
    ]
    assert "Build failed after 0s" in out.splitlines()
    assert gh_summary.read_text(encoding="utf-8") == snapshot


def test_failed_gradle_build_annotates_outside_files_by_location(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    monkeypatch.chdir(tmp_path)
    _fake_build(monkeypatch, 1, GRADLE_BUILD)
    assert main(["build"]) == 1
    assert _annotations(capsys.readouterr().out) == [
        "::error title=Compile error::/home/runner/_work/_temp/build/android-build/src/org/mavlink/"
        "qgroundcontrol/QGCActivity.java:42: cannot find symbol",
        "::error title=Gradle build failed::Execution failed for task "
        "':compileReleaseJavaWithJavac'.%0A> Compilation failed; see the compiler error output "
        "for details.",
    ]
    summary = gh_summary.read_text(encoding="utf-8")
    assert "Failed targets:\n\n- `android-build/QGroundControl.apk`" in summary
    assert "* What went wrong:\nExecution failed for task" in summary


def test_failed_build_without_a_recognized_error_still_annotates(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    monkeypatch.chdir(tmp_path)
    _fake_build(monkeypatch, 2, "Killed\n")
    assert main(["build"]) == 2
    log = tmp_path / "build-output.log"
    assert _annotations(capsys.readouterr().out) == [
        f"::error::Build failed after 0s; no error recognized in {log}"
    ]
    assert "Failed targets / first errors" not in gh_summary.read_text(encoding="utf-8")


def test_successful_build_logs_plainly(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    monkeypatch.chdir(tmp_path)
    _fake_build(monkeypatch, 0, "[1/1] Linking CXX executable QGroundControl\n")
    assert main(["build"]) == 0
    out = capsys.readouterr().out
    assert "Build completed in 0s" in out.splitlines()
    assert _annotations(out) == []
    assert "<details>" not in gh_summary.read_text(encoding="utf-8")


def test_build_output_file_and_reviewdog_log(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_env: Path
) -> None:
    monkeypatch.chdir(tmp_path)
    build = _fake_build(monkeypatch, 0)
    assert main(["build", "--reviewdog"]) == 0
    assert main(["build", "--output-file", "qgc-build.log"]) == 0
    assert build.logs == [str(tmp_path / "build-output.log"), "qgc-build.log"]
    assert read_github_file(gh_env) == {"REVIEWDOG_LOG": str(tmp_path / "build-output.log")}


def test_package_build_reports_only_its_own_compiler_cache_use(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_summary: Path
) -> None:
    monkeypatch.chdir(tmp_path)
    after = {
        **CCACHE_JSON,
        "direct_cache_hit": CCACHE_JSON["direct_cache_hit"] + 45,
        "cache_miss": CCACHE_JSON["cache_miss"] + 5,
        "cache_size_kibibyte": 1_300_000,
    }
    readings = iter([CCACHE_JSON, after])
    monkeypatch.setattr("ci.compile.cmake_helper.ccache_stats", lambda: next(readings))
    _fake_build(monkeypatch, 0)

    assert main(["build", "--target", "qgc-package"]) == 0
    summary = gh_summary.read_text(encoding="utf-8")
    assert summary.startswith("### Build (qgc-package)\n")
    assert (
        "| Compiler cache | ccache: 45 of 50 (90.0%) hits (45 direct, 0 preprocessed) |" in summary
    )
    assert "| Compiler cache size | 1.2 GiB of 3.0 GiB, 0 LRU cleanups |" in summary


def test_counter_delta_recurses_and_survives_zeroed_statistics() -> None:
    before = {
        "stats": {"cache_hits": {"counts": {"C/C++": 90}}, "cache_writes": 4},
        "cache_size": 9,
    }
    after = {
        "stats": {"cache_hits": {"counts": {"C/C++": 95}}, "cache_writes": 2},
        "cache_size": 10,
    }
    assert counter_delta(after, before) == {
        "stats": {"cache_hits": {"counts": {"C/C++": 5}}, "cache_writes": 2},
        "cache_size": 10,
    }
    assert counter_delta(after, None) is after
