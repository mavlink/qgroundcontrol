"""Configure and invoke the cache launcher in separate processes, as CI does."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]


@pytest.mark.skipif(shutil.which("cmake") is None, reason="CMake is required")
def test_cache_settings_survive_configure_and_allow_build_time_overrides(tmp_path):
    source = tmp_path / "source with spaces"
    source.mkdir()
    build = tmp_path / "build with spaces"
    config = source / "tools/configs/ccache.conf"
    config.parent.mkdir(parents=True)
    config.write_text("max_size = 5G\n")
    windows = os.name == "nt"
    fake = source / ("ccache.cmd" if windows else "ccache")
    fake.write_text(
        "@echo off\necho %CCACHE_CONFIGPATH%\necho %CCACHE_DIR%\necho %CCACHE_BASEDIR%\necho %~1\nexit /b 37\n"
        if windows
        else '#!/bin/sh\nprintf "%s\\n" "$CCACHE_CONFIGPATH" "$CCACHE_DIR" "$CCACHE_BASEDIR" "$1"\nexit 37\n'
    )
    fake.chmod(0o755)
    (source / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.25)\nproject(Launcher NONE)\n"
        f'include("{(ROOT / "cmake/Helpers.cmake").as_posix()}")\nqgc_config_caching()\n'
    )
    subprocess.run(
        ["cmake", "-S", str(source), "-B", str(build), f"-DQGC_CACHE_PROGRAM={fake.as_posix()}"],
        check=True,
        capture_output=True,
    )
    launcher = build / ("ccache-launcher.cmd" if windows else "ccache-launcher")
    env = {key: value for key, value in os.environ.items() if not key.startswith("CCACHE_")}
    for override in (False, True):
        expected = [config.as_posix(), (source / ".ccache").as_posix(), source.as_posix()]
        if override:
            expected = ["custom config", "custom cache", "custom base"]
            env.update(
                zip(["CCACHE_CONFIGPATH", "CCACHE_DIR", "CCACHE_BASEDIR"], expected, strict=True)
            )
        result = subprocess.run(
            [str(launcher), "argument with spaces"],
            env=env,
            shell=windows,
            capture_output=True,
            text=True,
            check=False,
        )
        assert result.returncode == 37
        assert result.stdout.splitlines() == [*expected, "argument with spaces"]


@pytest.mark.skipif(
    shutil.which("cmake") is None or shutil.which("ninja") is None,
    reason="CMake and Ninja are required",
)
@pytest.mark.parametrize("generator", ["Ninja", "Ninja Multi-Config"])
def test_windows_cache_launcher_uses_response_files_for_long_compile_commands(tmp_path, generator):
    source = tmp_path / "source with spaces"
    source.mkdir()
    build = tmp_path / "build with spaces"
    # A recording compiler keeps the fixture independent of an installed Windows SDK.
    compiler = source / "compiler.py"
    compiler.write_text(
        "import pathlib, sys\n"
        "response = next(arg[1:] for arg in sys.argv[1:] if arg.startswith('@'))\n"
        "pathlib.Path('received.txt').write_text(pathlib.Path(response).read_text())\n"
    )
    (source / "main.cpp").write_text("int probe;\n")
    cache = source / "ccache.cmd"
    cache.write_text("@echo off\n%*\nexit /b %errorlevel%\n")
    (source / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.25)\n"
        f'set(CMAKE_CXX_COMPILER "{Path(sys.executable).as_posix()}")\n'
        "set(CMAKE_CXX_COMPILER_ID GNU)\n"
        "set(CMAKE_CXX_COMPILER_FORCED TRUE)\n"
        "set(CMAKE_CXX_COMPILER_ID_RUN TRUE)\n"
        "project(Launcher CXX)\n"
        f'set(CMAKE_CXX_COMPILE_OBJECT [[<CMAKE_CXX_COMPILER> "{compiler.as_posix()}" '
        "<DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>]])\n"
        "set(CMAKE_HOST_WIN32 TRUE)\n"
        f'include("{(ROOT / "cmake/Helpers.cmake").as_posix()}")\n'
        "qgc_config_caching()\n"
        "add_library(probe OBJECT main.cpp)\n"
        "foreach(i RANGE 1 500)\n"
        "  target_compile_definitions(probe PRIVATE LONG_COMPILE_DEFINITION_${i}=1)\n"
        "endforeach()\n"
    )
    subprocess.run(
        [
            "cmake",
            "-S",
            str(source),
            "-B",
            str(build),
            "-G",
            generator,
            f"-DQGC_CACHE_PROGRAM={cache.as_posix()}",
        ],
        check=True,
        capture_output=True,
    )
    commands = subprocess.run(
        ["ninja", "-C", str(build), "-t", "commands", "probe"],
        check=True,
        capture_output=True,
        text=True,
    ).stdout.splitlines()
    assert len(commands) == 1
    assert len(commands[0]) < 8191
    assert "ccache-launcher.cmd" in commands[0]
    if os.name != "nt":
        # Exercise Ninja's generated response file on Unix too; Windows runs the real batch launcher.
        launcher = build / "ccache-launcher.cmd"
        launcher.write_text('#!/bin/sh\nexec "$@"\n')
        launcher.chmod(0o755)
    subprocess.run(
        ["cmake", "--build", str(build), "--target", "probe"], check=True, capture_output=True
    )
    received = (build / "received.txt").read_text()
    assert len(received) > 8191
    for index in range(1, 501):
        assert f"-DLONG_COMPILE_DEFINITION_{index}=1" in received
