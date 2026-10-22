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


def test_select_preset_covers_local_build_types_and_linux_coverage(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    expected = {
        "Debug": "default",
        "Release": "default-release",
        "RelWithDebInfo": "default-relwithdebinfo",
        "MinSizeRel": "default-minsizerel",
    }
    for build_type, preset in expected.items():
        assert select_preset(CMakeConfig(build_type=build_type)) == preset

    monkeypatch.setattr(f"{MODULE}.sys.platform", "linux")
    assert select_preset(CMakeConfig(coverage=True)) == "Linux-coverage"
    assert select_preset(CMakeConfig(preset="Linux-deb")) == "Linux-deb"
    assert select_preset(CMakeConfig(use_preset=False)) is None
    assert select_preset(CMakeConfig(generator="Unix Makefiles")) is None
    assert select_preset(CMakeConfig(generator="Unix Makefiles", coverage=True)) is None
    assert select_preset(CMakeConfig(generator="Unix Makefiles", preset="default")) == "default"


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
    assert "-G" not in args
    assert not any(str(arg).startswith("-DCMAKE_BUILD_TYPE=") for arg in args)
    assert not any(str(arg).startswith("-DQGC_BUILD_TESTING=") for arg in args)
    assert invocation["env"]["QT_ROOT_DIR"] == str(qt_root.resolve())


def test_configure_legacy_escape_retains_command_line_configuration(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    invocation: dict[str, Any] = {}

    _capture_run(monkeypatch, invocation)
    assert (
        configure(
            CMakeConfig(
                source_dir=tmp_path,
                build_dir=tmp_path / "legacy",
                use_preset=False,
                use_qt_cmake=False,
            )
        )
        == 0
    )

    args = invocation["args"]
    assert "--preset" not in args
    assert "-G" in args
    assert "-DCMAKE_BUILD_TYPE=Debug" in args
    assert "-DQGC_BUILD_TESTING=OFF" in args


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


def test_explicit_noncoverage_preset_can_enable_coverage(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    invocation: dict[str, Any] = {}

    _capture_run(monkeypatch, invocation)
    assert (
        configure(
            CMakeConfig(
                source_dir=tmp_path,
                build_dir=tmp_path / "coverage",
                preset="default",
                coverage=True,
                use_qt_cmake=False,
            )
        )
        == 0
    )
    assert "-DQGC_ENABLE_COVERAGE=ON" in invocation["args"]


def test_configure_writes_no_github_output(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    output = tmp_path / "github-output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output))
    _capture_run(monkeypatch, {})
    assert configure(CMakeConfig(source_dir=tmp_path, use_qt_cmake=False)) == 0
    assert not output.exists()


def test_cli_rejects_raw_unity_flags() -> None:
    # Raw CMAKE_UNITY_BUILD also unity-builds third-party targets; QGC_UNITY_BUILD is the supported switch.
    for flag in ("--unity", "--unity-batch=8"):
        with pytest.raises(SystemExit):
            configure_cli.parse_args([flag])


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
                "--stable",
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
    assert args[-2:] == ["-DQGC_STABLE_BUILD=ON", "-DQGC_UNITY_BUILD=ON"]
    assert not any("CMAKE_UNITY_BUILD" in arg for arg in args)
