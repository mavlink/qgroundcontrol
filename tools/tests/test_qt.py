"""Qt kit discovery contracts shared by configure and lupdate."""

from __future__ import annotations

import sys
from pathlib import Path

import pytest
from common.qt import find_qt_tool, qt_version_key

pytestmark = pytest.mark.windows


def _tool(path: Path) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.touch(mode=0o755)
    return path


def _isolate(monkeypatch: pytest.MonkeyPatch, home: Path) -> None:
    monkeypatch.setattr("common.qt.Path.home", lambda: home)
    monkeypatch.setattr("common.qt.is_windows", lambda: False)
    monkeypatch.setattr("common.qt.is_macos", lambda: False)
    monkeypatch.setattr("common.qt.is_linux", lambda: False)
    monkeypatch.setattr("common.qt.host_arch", lambda: "x86_64")
    monkeypatch.delenv("QT_ROOT_DIR", raising=False)


def test_qt_version_key_parses_install_paths() -> None:
    cases = {
        "/home/user/Qt/6.8.0/gcc_64/bin/qt-cmake": (6, 8, 0),
        "/home/user/Qt/6.10.1/gcc_64/bin": (6, 10, 1),
        r"C:\Qt\6.11.1\msvc2022_64\bin\qt-cmake.bat": (6, 11, 1),
        "/usr/bin/qt-cmake": (0,),
    }
    for path, expected in cases.items():
        assert qt_version_key(Path(path)) == expected
    assert qt_version_key(Path("/opt/Qt/6.10.0/gcc_64")) > qt_version_key(
        Path("/opt/Qt/6.9.3/gcc_64")
    )


def test_explicit_root_then_qt_root_dir_take_precedence(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _isolate(monkeypatch, tmp_path)
    _tool(tmp_path / "Qt" / "6.12.0" / "gcc_64" / "bin" / "qt-cmake")
    explicit = _tool(tmp_path / "explicit" / "bin" / "qt-cmake")
    from_env = _tool(tmp_path / "env" / "bin" / "qt-cmake")
    monkeypatch.setenv("QT_ROOT_DIR", str(tmp_path / "env"))

    assert find_qt_tool("qt-cmake", qt_root=tmp_path / "explicit") == explicit
    assert find_qt_tool("qt-cmake", qt_root=tmp_path / "missing") == from_env


@pytest.mark.skipif(sys.platform == "win32", reason="Windows has no executable mode bit")
def test_discovers_newest_standard_install_and_skips_non_executables(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _isolate(monkeypatch, tmp_path)
    older = _tool(tmp_path / "Qt" / "6.8.3" / "gcc_64" / "bin" / "lupdate")
    newest = _tool(tmp_path / "Qt" / "6.11.1" / "gcc_64" / "bin" / "lupdate")
    assert find_qt_tool("lupdate") == newest

    newest.chmod(0o644)
    assert find_qt_tool("lupdate") == older


def test_pinned_version_ignores_other_kits(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    _isolate(monkeypatch, tmp_path)
    _tool(tmp_path / "Qt" / "6.11.1" / "gcc_64" / "bin" / "lupdate")
    pinned = _tool(tmp_path / "Qt" / "6.10.0" / "gcc_64" / "bin" / "lupdate")

    assert find_qt_tool("lupdate", version="6.10.0") == pinned
    assert find_qt_tool("lupdate", version="6.9.0") is None


def test_uses_native_linux_arm64_kit(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    _isolate(monkeypatch, tmp_path)
    monkeypatch.setattr("common.qt.host_arch", lambda: "aarch64")
    _tool(tmp_path / "Qt" / "6.12.0" / "gcc_64" / "bin" / "qt-cmake")
    arm64 = _tool(tmp_path / "Qt" / "6.12.0" / "gcc_arm64" / "bin" / "qt-cmake")

    assert find_qt_tool("qt-cmake") == arm64


def test_windows_accepts_future_msvc_toolsets_and_script_suffixes(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _isolate(monkeypatch, tmp_path / "empty-home")
    monkeypatch.setattr("common.qt.is_windows", lambda: True)

    for architecture, kit in (("x86_64", "msvc2025_64"), ("aarch64", "msvc2025_arm64")):
        bin_dir = tmp_path / "Qt" / "6.12.0" / kit / "bin"
        qt_cmake = _tool(bin_dir / "qt-cmake.bat")
        _tool(bin_dir / "qt-cmake")
        lupdate = _tool(bin_dir / "lupdate.exe")

        monkeypatch.setattr("common.qt.host_arch", lambda architecture=architecture: architecture)
        monkeypatch.setattr(
            "common.qt.glob",
            lambda pattern, kit=kit, bin_dir=bin_dir: (
                [str(bin_dir)] if f"msvc*_{kit.rsplit('_', 1)[-1]}" in pattern else []
            ),
        )

        assert find_qt_tool("qt-cmake") == qt_cmake
        assert find_qt_tool("lupdate") == lupdate


def test_returns_none_without_any_install(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    _isolate(monkeypatch, tmp_path)
    assert find_qt_tool("qt-cmake", qt_root=tmp_path / "missing") is None
