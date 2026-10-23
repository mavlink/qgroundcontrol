"""CMake configure-preset selection and ``qt-cmake`` invocation.

Shared by the ``tools/configure.py`` developer CLI and CI's ``cmake_helper.py configure``.
"""

from __future__ import annotations

import os
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

from common.proc import run_tee
from common.qt import find_qt_tool

__all__ = ["LOCAL_PRESETS", "CMakeConfig", "configure", "find_qt_cmake", "select_preset"]

LOCAL_PRESETS = {
    "Debug": "default",
    "Release": "default-release",
    "RelWithDebInfo": "default-relwithdebinfo",
    "MinSizeRel": "default-minsizerel",
}


@dataclass
class CMakeConfig:
    """CMake invocation options."""

    source_dir: Path = field(default_factory=lambda: Path("."))
    # None keeps the preset's binaryDir, so `cmake --build --preset` finds the tree.
    build_dir: Path | None = None
    build_type: str = "Debug"
    testing: bool = False
    preset: str | None = None
    stable: bool = False
    use_qt_cmake: bool = True
    qt_root: Path | None = None
    extra_args: list[str] = field(default_factory=list)


def select_preset(config: CMakeConfig) -> str:
    """Return the explicit preset, or the canonical local preset for the build type."""
    return config.preset or LOCAL_PRESETS[config.build_type]


def find_qt_cmake(qt_root: Path | None = None) -> Path | None:
    """Find ``qt-cmake`` in *qt_root*, ``$QT_ROOT_DIR``, or the newest standard Qt install."""
    return find_qt_tool("qt-cmake", qt_root=qt_root)


def configure(config: CMakeConfig, *, log_file: Path | None = None) -> int:
    """Run CMake configuration and return its exit code.

    With *log_file*, CMake's combined output also goes to that file as it streams.
    """
    preset = select_preset(config)

    qt_cmake: Path | None = None
    if config.use_qt_cmake:
        qt_cmake = find_qt_cmake(config.qt_root)
        if not qt_cmake:
            print("Error: qt-cmake not found; pass --no-qt-cmake to use cmake", file=sys.stderr)
            return 1
        print(f"Using: {qt_cmake}")
    cmake_cmd = str(qt_cmake) if qt_cmake else "cmake"

    args = [cmake_cmd, "--preset", preset, "-S", str(config.source_dir)]
    if config.build_dir is not None:
        args += ["-B", str(config.build_dir)]
    if config.testing:
        args.append("-DQGC_BUILD_TESTING=ON")
    if config.stable:
        args.append("-DQGC_STABLE_BUILD=ON")
    args.extend(config.extra_args)

    build_dir = config.build_dir or "the preset's binaryDir"
    print(f"Preset: {preset}")
    print(f"Build dir: {build_dir}")

    env = os.environ.copy()
    if qt_cmake:
        env["QT_ROOT_DIR"] = str(qt_cmake.parent.parent.resolve())
    elif config.qt_root:
        env["QT_ROOT_DIR"] = str(config.qt_root.resolve())
    if log_file is None:
        returncode = subprocess.run(args, env=env, check=False).returncode
    else:
        log_file.parent.mkdir(parents=True, exist_ok=True)
        # Windows runs qt-cmake.bat, whose arguments a Bash pipeline would re-quote.
        returncode = run_tee(args, log_file, env=env, bash_pipeline=False)
    if returncode != 0:
        return returncode

    print(f"Configured: {build_dir}")
    return 0
