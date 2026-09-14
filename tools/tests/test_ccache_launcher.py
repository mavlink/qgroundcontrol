"""Configure and invoke the cache launcher in separate processes, as CI does."""

from __future__ import annotations

import os
import shutil
import subprocess
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
