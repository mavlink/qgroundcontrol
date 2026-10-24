"""Preset-selection, qt-cmake invocation, and CLI contracts for the configure tool."""

from __future__ import annotations

import subprocess
from pathlib import Path
from typing import Any

import configure as configure_cli
import pytest
from qgc_tools.cmake_configure import CMakeConfig, configure, find_qt_cmake, select_preset

pytestmark = pytest.mark.windows

MODULE = "qgc_tools.cmake_configure"


def _capture_run(monkeypatch: pytest.MonkeyPatch, invocation: dict[str, Any]) -> None:
    def fake_run(args: list[str], **kwargs: Any) -> subprocess.CompletedProcess[list[str]]:
        invocation.update(args=args, **kwargs)
        return subprocess.CompletedProcess(args, 0)

    monkeypatch.setattr(f"{MODULE}.subprocess.run", fake_run)


def test_find_qt_cmake_prefers_explicit_root(tmp_path: Path) -> None:
    qt_cmake = tmp_path / "qt" / "bin" / "qt-cmake"
    qt_cmake.parent.mkdir(parents=True)
    qt_cmake.touch(mode=0o755)
    assert find_qt_cmake(tmp_path / "qt") == qt_cmake


def test_select_preset_maps_build_types_and_honors_explicit_preset() -> None:
    expected = {
        "Debug": "default",
        "Release": "default-release",
        "RelWithDebInfo": "default-relwithdebinfo",
        "MinSizeRel": "default-minsizerel",
    }
    for build_type, preset in expected.items():
        assert select_preset(CMakeConfig(build_type=build_type)) == preset
    assert select_preset(CMakeConfig(build_type="Release", preset="Linux-deb")) == "Linux-deb"


def test_configure_uses_preset_and_exports_qt_root(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    qt_root = tmp_path / "qt"
    qt_cmake = qt_root / "bin" / "qt-cmake"
    qt_cmake.parent.mkdir(parents=True)
    qt_cmake.touch(mode=0o755)
    invocation: dict[str, Any] = {}

    _capture_run(monkeypatch, invocation)
    assert (
        configure(
            CMakeConfig(
                source_dir=tmp_path,
                build_dir=tmp_path / "build",
                build_type="Release",
                qt_root=qt_root,
            )
        )
        == 0
    )

    args = invocation["args"]
    assert args[:3] == [str(qt_cmake), "--preset", "default-release"]
    assert args[args.index("-B") + 1] == str(tmp_path / "build")
    assert "-G" not in args
    assert not any(str(arg).startswith("-DCMAKE_BUILD_TYPE=") for arg in args)
    assert not any(str(arg).startswith("-DQGC_BUILD_TESTING=") for arg in args)
    assert invocation["env"]["QT_ROOT_DIR"] == str(qt_root.resolve())


def test_configure_without_build_dir_keeps_the_preset_binary_dir(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    # An explicit -B would move non-default presets out of build/<preset>, where
    # `cmake --build --preset` and `ctest --preset` look for the tree.
    invocation: dict[str, Any] = {}

    _capture_run(monkeypatch, invocation)
    assert (
        configure(
            CMakeConfig(source_dir=tmp_path, preset="Linux-debug", testing=True, use_qt_cmake=False)
        )
        == 0
    )

    assert invocation["args"] == [
        "cmake",
        "--preset",
        "Linux-debug",
        "-S",
        str(tmp_path),
        "-DQGC_BUILD_TESTING=ON",
    ]


def test_configure_exports_root_of_selected_qt_cmake(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    requested_qt_root = tmp_path / "requested"
    selected_qt_root = tmp_path / "selected"
    qt_cmake = selected_qt_root / "bin" / "qt-cmake"
    qt_cmake.parent.mkdir(parents=True)
    qt_cmake.touch(mode=0o755)
    invocation: dict[str, Any] = {}
    monkeypatch.setattr(f"{MODULE}.find_qt_cmake", lambda _root: qt_cmake)
    _capture_run(monkeypatch, invocation)

    assert configure(CMakeConfig(qt_root=requested_qt_root)) == 0
    assert invocation["env"]["QT_ROOT_DIR"] == str(selected_qt_root.resolve())


def test_configure_requires_qt_cmake_by_default(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    def unexpected_run(*_args: object, **_kwargs: object) -> None:
        raise AssertionError("cmake should not run when qt-cmake is unavailable")

    monkeypatch.setattr(f"{MODULE}.find_qt_cmake", lambda _root: None)
    monkeypatch.setattr(f"{MODULE}.subprocess.run", unexpected_run)

    assert configure(CMakeConfig(source_dir=tmp_path, build_dir=tmp_path / "build")) == 1


def test_configure_writes_no_github_output(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_output: Path
) -> None:
    _capture_run(monkeypatch, {})
    assert configure(CMakeConfig(source_dir=tmp_path, use_qt_cmake=False)) == 0
    assert gh_output.read_text() == ""


def test_configure_log_tees_through_a_python_pipe_into_a_new_build_dir(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    calls: list[tuple[list[str], Path, dict[str, Any]]] = []

    def fake_run_tee(args: list[str], output_file: Path, **kwargs: Any) -> int:
        assert output_file.parent.is_dir()
        calls.append((args, output_file, kwargs))
        return 2

    monkeypatch.setattr(f"{MODULE}.run_tee", fake_run_tee)
    monkeypatch.setattr(f"{MODULE}.subprocess.run", lambda *_a, **_k: pytest.fail("not teed"))
    log = tmp_path / "build" / "cmake-configure.log"
    config = CMakeConfig(source_dir=tmp_path, build_dir=tmp_path / "build", use_qt_cmake=False)

    assert configure(config, log_file=log) == 2
    ((args, output_file, kwargs),) = calls
    assert args[:3] == ["cmake", "--preset", "default"]
    assert output_file == log
    assert kwargs["bash_pipeline"] is False


def test_cli_rejects_raw_unity_flags() -> None:
    # Raw CMAKE_UNITY_BUILD also unity-builds third-party targets; QGC_UNITY_BUILD is the supported switch.
    for flag in ("--unity", "--unity-batch=8"):
        with pytest.raises(SystemExit):
            configure_cli.parse_args([flag])


def test_cli_preset_without_build_dir_omits_binary_dir_override(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    invocation: dict[str, Any] = {}
    _capture_run(monkeypatch, invocation)
    assert configure_cli.main(["--preset", "Linux-debug", "--no-qt-cmake"]) == 0
    assert invocation["args"][:3] == ["cmake", "--preset", "Linux-debug"]
    assert "-B" not in invocation["args"]


def test_cli_forwards_options_and_extra_cmake_arguments(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    invocation: dict[str, Any] = {}
    _capture_run(monkeypatch, invocation)
    assert (
        configure_cli.main(
            [
                "-B",
                str(tmp_path / "out"),
                "--release",
                "--no-qt-cmake",
                "--",
                "-DQGC_UNITY_BUILD=ON",
            ]
        )
        == 0
    )
    args = invocation["args"]
    assert args[:3] == ["cmake", "--preset", "default-release"]
    assert args[args.index("-S") + 1] == str(Path(configure_cli.__file__).resolve().parents[1])
    assert args[args.index("-B") + 1] == str(tmp_path / "out")
    assert args[-1] == "-DQGC_UNITY_BUILD=ON"
    assert not any("CMAKE_UNITY_BUILD" in arg for arg in args)
