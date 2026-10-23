"""Tests for tools/setup/qt_installer/patch.py on synthetic trees shaped like Qt 6.12's archives.

The expected edits are aqtinstall's (aqt/updater.py), including its newline handling: an edited
file is written back with the platform's line endings.
"""

from __future__ import annotations

import os
import stat
import sys
from typing import TYPE_CHECKING

import pytest
from setup.qt_installer.patch import patch_cross, patch_desktop

from ._helpers import write_files

if TYPE_CHECKING:
    from pathlib import Path

pytestmark = pytest.mark.windows

POSIX_ONLY = pytest.mark.skipif(sys.platform == "win32", reason="Windows has no execute bits")


def _tree(root: Path, files: dict[str, str]) -> Path:
    write_files(root, files)
    return root


def _text(prefix: Path, rel: str) -> str:
    return (prefix / rel).read_text(encoding="utf-8")


def test_linux_desktop(tmp_path: Path) -> None:
    prefix = _tree(
        tmp_path / "Qt/6.12.0/gcc_64",
        {
            "bin/qmake": "",
            "lib/pkgconfig/Qt6Core.pc": "prefix=/home/qt/work/install\nlibdir=${prefix}/lib\n",
            "lib/libQt6Core.la": (
                "libdir='/home/qt/work/install/lib'\ndependency_libs=' -L/home/qt/work/install/lib'\n"
            ),
            "lib/libQt6Core.prl": "QMAKE_PRL_LIBS = /home/qt/work/install/lib/libQt6Dep.so\n",
            "lib/libQt6Gui.prl": "QMAKE_PRL_TARGET = libQt6Gui.so\n",
        },
    )

    patched = patch_desktop(prefix, "linux")

    lib = prefix / "lib"
    assert patched == [
        "bin/qt.conf",
        "lib/pkgconfig/Qt6Core.pc",
        "lib/libQt6Core.la",
        "lib/libQt6Core.prl",
    ]
    assert _text(prefix, "bin/qt.conf") == "[Paths]\nPrefix=..\n"
    assert _text(prefix, "lib/pkgconfig/Qt6Core.pc") == f"prefix={prefix}\nlibdir=${{prefix}}/lib\n"
    assert _text(prefix, "lib/libQt6Core.la") == f"libdir='{lib}'\ndependency_libs=' -L{lib}'\n"
    assert (
        _text(prefix, "lib/libQt6Core.prl") == "QMAKE_PRL_LIBS = $$[QT_INSTALL_LIBS]/libQt6Dep.so\n"
    )
    assert not (prefix / "bin/qtenv2.bat").exists()


def test_linux_arm64_desktop_gets_the_linux_edits_aqt_skips(tmp_path: Path) -> None:
    prefix = _tree(
        tmp_path / "gcc_arm64",
        {"bin/qmake": "", "lib/pkgconfig/Qt6Core.pc": "prefix=/home/qt/work/install\n"},
    )
    assert patch_desktop(prefix, "linux_arm64") == ["bin/qt.conf", "lib/pkgconfig/Qt6Core.pc"]
    assert _text(prefix, "lib/pkgconfig/Qt6Core.pc") == f"prefix={prefix}\n"


def test_mac_desktop_patches_framework_paths(tmp_path: Path) -> None:
    prefix = _tree(
        tmp_path / "macos",
        {
            "bin/qmake": "",
            "lib/pkgconfig/Qt6Core.pc": (
                "prefix=/Users/qt/work/install\nLibs: -F/Users/qt/work/install/lib -framework QtCore\n"
            ),
            "lib/QtCore.la": "libdir='=/Users/qt/work/install/lib'\nlibs=' -F/Users/qt/work/install/lib'\n",
        },
    )

    patch_desktop(prefix, "mac")

    lib = prefix / "lib"
    assert _text(prefix, "lib/pkgconfig/Qt6Core.pc") == (
        f"prefix={prefix}\nLibs: -F{lib} -framework QtCore\n"
    )
    assert _text(prefix, "lib/QtCore.la") == f"libdir='={lib}'\nlibs=' -F{lib}'\n"


@pytest.mark.parametrize("host", ["windows", "windows_arm64"])
def test_windows_desktop_writes_qtenv2_and_skips_libtool(tmp_path: Path, host: str) -> None:
    la = "libdir='c:/Users/qt/work/install/lib'\n"
    prefix = _tree(
        tmp_path / "msvc2022_64",
        {
            "bin/qmake.exe": "",
            "lib/pkgconfig/Qt6Core.pc": "prefix=c:/Users/qt/work/install\n",
            "lib/Qt6Core.prl": "QMAKE_PRL_LIBS = c:/Users/qt/work/install/lib/Qt6Dep.lib\n",
            "lib/Qt6Core.la": la,
        },
    )

    patched = patch_desktop(prefix, host)

    assert patched == [
        "bin/qt.conf",
        "bin/qtenv2.bat",
        "lib/pkgconfig/Qt6Core.pc",
        "lib/Qt6Core.prl",
    ]
    assert _text(prefix, "lib/pkgconfig/Qt6Core.pc") == f"prefix={prefix}\n"
    assert _text(prefix, "lib/Qt6Core.prl") == "QMAKE_PRL_LIBS = $$[QT_INSTALL_LIBS]/Qt6Dep.lib\n"
    assert _text(prefix, "lib/Qt6Core.la") == la
    assert _text(prefix, "bin/qtenv2.bat") == (
        "@echo off\necho Setting up environment for Qt usage...\n"
        f"set PATH={prefix / 'bin'};%PATH%\ncd /D {prefix}\n"
        "echo Remember to call vcvarsall.bat to complete environment setup!\n"
    )


def test_unknown_host_gets_only_qt_conf(tmp_path: Path) -> None:
    prefix = _tree(
        tmp_path / "x", {"bin/qmake": "", "lib/pkgconfig/a.pc": "prefix=/home/qt/work/install\n"}
    )
    assert patch_desktop(prefix, "solaris") == ["bin/qt.conf"]
    assert _text(prefix, "lib/pkgconfig/a.pc") == "prefix=/home/qt/work/install\n"


def test_edited_files_get_platform_line_endings_like_aqt(tmp_path: Path) -> None:
    prefix = tmp_path / "gcc_64"
    (prefix / "bin").mkdir(parents=True)
    (prefix / "lib").mkdir()
    (prefix / "lib/libQt6Core.prl").write_bytes(b"QMAKE_PRL_TARGET = libQt6Core.so\r\nX = 1\r\n")

    assert patch_desktop(prefix, "linux") == ["bin/qt.conf"]

    newline = os.linesep.encode()
    assert (prefix / "lib/libQt6Core.prl").read_bytes() == (
        b"QMAKE_PRL_TARGET = libQt6Core.so" + newline + b"X = 1" + newline
    )
    assert (prefix / "bin/qt.conf").read_bytes() == b"[Paths]" + newline + b"Prefix=.." + newline


TARGET_QT_CONF = """[DevicePaths]
Prefix=/usr/local/Qt-6.12.0
[Paths]
Prefix=../
HostPrefix=../../
HostData=target
HostLibraryExecutables=./bin/host
TargetSpec=android-clang
"""


def _script(build_prefix: str) -> str:
    return f'#!/bin/sh\n{build_prefix}bin/qmake6 -qtconf "$script_dir_path/target_qt.conf" $*\n'


def test_android_on_linux(tmp_path: Path) -> None:
    outdir = tmp_path / "Qt"
    scripts = {
        "bin/qmake": _script("/Users/qt/work/install/"),
        "bin/qmake6": _script("/Users/qt/work/install/"),
        "bin/qtpaths": _script("/home/qt/work/install/"),
        "bin/qtpaths6": _script("/home/qt/work/install/"),
    }
    prefix = _tree(
        outdir / "6.12.0/android_arm64_v8a",
        {
            **scripts,
            "bin/target_qt.conf": TARGET_QT_CONF,
            "mkspecs/qdevice.pri": "DEFAULT_ANDROID_PLATFORM = android-28\nDEFAULT_ANDROID_NDK_HOST = darwin-x86_64\n",
        },
    )

    patched = patch_cross(prefix, outdir, "6.12.0", "linux", "gcc_64")

    assert patched == [
        "bin/qmake",
        "bin/qtpaths",
        "bin/qmake6",
        "bin/qtpaths6",
        "bin/target_qt.conf",
        "mkspecs/qdevice.pri",
    ]
    host_bin = f"{outdir}/6.12.0/gcc_64/bin"
    for rel in scripts:
        assert (
            _text(prefix, rel)
            == f'#!/bin/sh\n{host_bin}/qmake6 -qtconf "$script_dir_path/target_qt.conf" $*\n'
        )
    assert _text(prefix, "bin/target_qt.conf") == TARGET_QT_CONF.replace(
        "HostPrefix=../../", "HostPrefix=../../gcc_64"
    ).replace("HostData=target", "HostData=../android_arm64_v8a").replace("./bin/host", "./libexec")
    assert _text(prefix, "mkspecs/qdevice.pri").endswith(
        "DEFAULT_ANDROID_NDK_HOST = linux-x86_64\n"
    )


@POSIX_ONLY
def test_cross_scripts_become_executable(tmp_path: Path) -> None:
    # Windows-built Android archives store the host-tool scripts as 0644.
    prefix = _tree(
        tmp_path / "Qt/6.12.0/android_armv7", {"bin/qmake": _script("/home/qt/work/install/")}
    )
    (prefix / "bin/qmake").chmod(0o644)

    patch_cross(prefix, tmp_path / "Qt", "6.12.0", "linux", "gcc_64")

    assert stat.S_IMODE((prefix / "bin/qmake").stat().st_mode) == 0o755


def test_ios_on_mac_points_at_the_macos_desktop(tmp_path: Path) -> None:
    outdir = tmp_path / "Qt"
    prefix = _tree(
        outdir / "6.12.0/ios_device",
        {"bin/qmake": _script("/Users/qt/work/install/"), "bin/target_qt.conf": TARGET_QT_CONF},
    )

    assert patch_cross(prefix, outdir, "6.12.0", "mac", "macos") == [
        "bin/qmake",
        "bin/target_qt.conf",
    ]
    assert f"{outdir}/6.12.0/macos/bin/qmake6" in _text(prefix, "bin/qmake")
    conf = _text(prefix, "bin/target_qt.conf")
    assert "HostPrefix=../../macos\n" in conf
    assert "HostData=../ios_device\n" in conf


def test_windows_arm64_cross_uses_host_prefixed_batch_files(tmp_path: Path) -> None:
    outdir = tmp_path / "Qt"
    batch = (
        '@echo off\n\\home\\qt\\work\\install\\bin\\qmake6.exe -qtconf "%~dp0\\target_qt.conf" %*\n'
    )
    prefix = _tree(
        outdir / "6.12.0/msvc2022_arm64",
        {
            "bin/host-qmake.bat": batch,
            "bin/host-qtpaths.bat": batch,
            "bin/qmake6.bat": batch,
            "bin/target_qt.conf": TARGET_QT_CONF,
        },
    )

    patched = patch_cross(prefix, outdir, "6.12.0", "windows", "msvc2022_64")

    assert patched == [
        "bin/host-qmake.bat",
        "bin/host-qtpaths.bat",
        "bin/qmake6.bat",
        "bin/target_qt.conf",
    ]
    host_bin = "\\".join([str(outdir), "6.12.0", "msvc2022_64", "bin"])
    assert _text(prefix, "bin/host-qmake.bat") == (
        f'@echo off\n{host_bin}\\qmake6.exe -qtconf "%~dp0\\target_qt.conf" %*\n'
    )
    conf = _text(prefix, "bin/target_qt.conf")
    assert "HostLibraryExecutables=./bin\n" in conf
    assert "HostPrefix=../../msvc2022_64\n" in conf


def test_build_machine_target_prefix_is_rewritten(tmp_path: Path) -> None:
    prefix = _tree(
        tmp_path / "Qt/6.12.0/android_x86_64",
        {"bin/target_qt.conf": "[Paths]\nPrefix=/home/qt/work/install/target\n"},
    )
    patch_cross(prefix, tmp_path / "Qt", "6.12.0", "linux", "gcc_64")
    assert _text(prefix, "bin/target_qt.conf") == f"[Paths]\nPrefix={prefix / 'target'}\n"
