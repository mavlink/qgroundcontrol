#!/usr/bin/env python3
"""Qt CMake discovery contracts for the configure tool."""

from __future__ import annotations

import subprocess
from pathlib import Path
from typing import TYPE_CHECKING, Any

from configure import CMakeConfig, configure, find_qt_cmake, parse_version, select_preset

if TYPE_CHECKING:
    import pytest


def test_parse_qt_version_from_install_path() -> None:
    cases = {
        "/home/user/Qt/6.8.0/gcc_64/bin/qt-cmake": (6, 8, 0),
        "/home/user/Qt/6.10.1/gcc_64/bin/qt-cmake": (6, 10, 1),
        "/usr/bin/qt-cmake": (0, 0, 0),
    }
    for path, expected in cases.items():
        assert parse_version(Path(path)) == expected


def test_find_qt_cmake_prefers_explicit_executable_and_handles_absence(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    qt_cmake = tmp_path / "qt" / "bin" / "qt-cmake"
    qt_cmake.parent.mkdir(parents=True)
    qt_cmake.touch(mode=0o755)
    assert find_qt_cmake(tmp_path / "qt") == qt_cmake

    monkeypatch.setattr("configure.Path.home", lambda: tmp_path / "empty-home")
    monkeypatch.delenv("QT_ROOT_DIR", raising=False)
    assert find_qt_cmake(tmp_path / "missing") is None


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

    monkeypatch.setattr("configure.sys.platform", "linux")
    assert select_preset(CMakeConfig(coverage=True)) == "Linux-coverage"
    assert select_preset(CMakeConfig(preset="Linux-deb")) == "Linux-deb"
    assert select_preset(CMakeConfig(use_preset=False)) is None


def test_configure_uses_preset_and_exports_qt_root(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    qt_root = tmp_path / "qt"
    qt_cmake = qt_root / "bin" / "qt-cmake"
    qt_cmake.parent.mkdir(parents=True)
    qt_cmake.touch(mode=0o755)
    invocation: dict[str, Any] = {}

    def fake_run(args: list[str], **kwargs: Any) -> subprocess.CompletedProcess[list[str]]:
        invocation.update(args=args, **kwargs)
        return subprocess.CompletedProcess(args, 0)

    monkeypatch.setattr("configure.subprocess.run", fake_run)
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

    def fake_run(args: list[str], **kwargs: Any) -> subprocess.CompletedProcess[list[str]]:
        invocation.update(args=args, **kwargs)
        return subprocess.CompletedProcess(args, 0)

    monkeypatch.setattr("configure.subprocess.run", fake_run)
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


def test_explicit_noncoverage_preset_can_enable_coverage(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    invocation: dict[str, Any] = {}

    def fake_run(args: list[str], **kwargs: Any) -> subprocess.CompletedProcess[list[str]]:
        invocation.update(args=args, **kwargs)
        return subprocess.CompletedProcess(args, 0)

    monkeypatch.setattr("configure.subprocess.run", fake_run)
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
