"""CMake configure-preset selection and ``qt-cmake`` invocation.

Shared by the ``tools/configure.py`` developer CLI and CI's ``cmake_helper.py configure``.
"""

from __future__ import annotations

import os
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

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
    build_dir: Path = field(default_factory=lambda: Path("build"))
    build_type: str = "Debug"
    generator: str = "Ninja"
    testing: bool = False
    coverage: bool = False
    preset: str | None = None
    use_preset: bool = True
    stable: bool = False
    use_qt_cmake: bool = True
    qt_root: Path | None = None
    extra_args: list[str] = field(default_factory=list)


def select_preset(config: CMakeConfig) -> str | None:
    """Select the canonical local preset for a configuration."""
    if not config.use_preset:
        return None
    if config.preset:
        return config.preset
    if config.generator != "Ninja":
        return None
    if config.coverage:
        if not sys.platform.startswith("linux"):
            raise ValueError("Coverage builds require Linux; use --no-preset for a custom setup")
        return "Linux-coverage"
    return LOCAL_PRESETS[config.build_type]


def find_qt_cmake(qt_root: Path | None = None) -> Path | None:
    """Find ``qt-cmake`` in *qt_root*, ``$QT_ROOT_DIR``, or the newest standard Qt install."""
    return find_qt_tool("qt-cmake", qt_root=qt_root)


def configure(config: CMakeConfig) -> int:
    """Run CMake configuration and return its exit code."""
    preset = select_preset(config)

    qt_cmake: Path | None = None
    if config.use_qt_cmake:
        qt_cmake = find_qt_cmake(config.qt_root)
        if not qt_cmake:
            print("Error: qt-cmake not found; pass --no-qt-cmake to use cmake", file=sys.stderr)
            return 1
        print(f"Using: {qt_cmake}")
    cmake_cmd = str(qt_cmake) if qt_cmake else "cmake"

    args = [cmake_cmd]
    if preset:
        args += ["--preset", preset]
    args += ["-S", str(config.source_dir), "-B", str(config.build_dir)]
    if not preset:
        args += ["-G", config.generator, f"-DCMAKE_BUILD_TYPE={config.build_type}"]

    if config.testing:
        args.append("-DQGC_BUILD_TESTING=ON")
    elif not preset:
        args.append("-DQGC_BUILD_TESTING=OFF")
    if config.coverage and preset != "Linux-coverage":
        args.append("-DQGC_ENABLE_COVERAGE=ON")
    if config.stable:
        args.append("-DQGC_STABLE_BUILD=ON")
    args.extend(config.extra_args)

    print(f"Preset: {preset}" if preset else f"Build type: {config.build_type}")
    print(f"Build dir: {config.build_dir}")

    env = os.environ.copy()
    if qt_cmake:
        env["QT_ROOT_DIR"] = str(qt_cmake.parent.parent.resolve())
    elif config.qt_root:
        env["QT_ROOT_DIR"] = str(config.qt_root.resolve())
    result = subprocess.run(args, env=env, check=False)
    if result.returncode != 0:
        return result.returncode

    print(f"Configured: {config.build_dir}")
    return 0
