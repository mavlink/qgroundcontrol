#!/usr/bin/env python3
"""Configure and build the native, Android, or cross target selected by the image."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shlex
import shutil
import subprocess
import sys
import time
from pathlib import Path


def require_env(*names: str) -> None:
    missing = [name for name in names if not os.environ.get(name)]
    if missing:
        raise ValueError(f"Missing environment variables: {', '.join(missing)}")


def run_phase(name: str, command: list[str], timings: dict[str, float]) -> None:
    started = time.monotonic()
    try:
        subprocess.run(command, check=True)
    finally:
        timings[name] = round(time.monotonic() - started, 2)


def compiler_fingerprint() -> str:
    if os.environ.get("ANDROID_SDK_ROOT"):
        require_env("ANDROID_NDK_ROOT")
        compiler_dir = (
            Path(os.environ["ANDROID_NDK_ROOT"]) / "toolchains/llvm/prebuilt/linux-x86_64/bin"
        )
        compilers = [[str(compiler_dir / name)] for name in ("clang", "clang++")]
    elif os.environ.get("SYSROOT"):
        compilers = [["aarch64-linux-gnu-gcc"], ["aarch64-linux-gnu-g++"]]
    else:
        compilers = [
            shlex.split(os.environ.get(name, default))
            for name, default in (("CC", "cc"), ("CXX", "c++"))
        ]
    identity = []
    for compiler in compilers:
        for flag in ("--version", "-dumpmachine"):
            result = subprocess.run([*compiler, flag], check=True, capture_output=True, text=True)
            identity.append(result.stdout.strip())
    if shutil.which("ccache"):
        identity.append(
            subprocess.run(
                ["ccache", "--version"], check=True, capture_output=True, text=True
            ).stdout
        )
    return hashlib.sha256(json.dumps(identity).encode()).hexdigest()


def initialize_caches(source: Path) -> None:
    os.environ.update(
        {
            "CCACHE_DIR": str(source / ".ccache"),
            "CCACHE_BASEDIR": str(source),
            "CCACHE_CONFIGPATH": str(source / "tools/configs/ccache.conf"),
            "MOCCACHE_DIR": str(source / ".cache/moccache"),
            "CPM_SOURCE_CACHE": str(source / ".cache/CPM"),
        }
    )
    os.environ.setdefault("MOCCACHE_STATS", "1")
    for name in ("CCACHE_DIR", "MOCCACHE_DIR", "CPM_SOURCE_CACHE"):
        Path(os.environ[name]).mkdir(parents=True, exist_ok=True)
    if shutil.which("ccache"):
        subprocess.run(["ccache", "--zero-stats"], check=True)
    subprocess.run([sys.executable, str(source / "tools/moccache.py"), "--zero-stats"], check=True)


def write_performance_report(
    source: Path, output: Path, timings: dict[str, float], success: bool
) -> None:
    # Installation starts only after compilation succeeds; its failure must not
    # discard the compiler caches along with the failed package.
    report: dict = {
        "success": success,
        "build_success": "build" in timings and (success or "install" in timings),
        "seconds": timings,
    }
    for name, command in (
        ("ccache", ["ccache", "--show-stats"]),
        (
            "moccache",
            [
                sys.executable,
                str(source / "tools/moccache.py"),
                "--show-stats",
                "--build-dir",
                str(output),
            ],
        ),
    ):
        try:
            result = subprocess.run(command, check=False, capture_output=True, text=True)
            report[name] = result.stdout + result.stderr
        except OSError as error:
            report[name] = str(error)
    report["cpm_bytes"] = sum(
        path.stat().st_size
        for path in (source / ".cache/CPM").rglob("*")
        if path.is_file() and not path.is_symlink()
    )
    output.mkdir(parents=True, exist_ok=True)
    (output / "docker-build-report.json").write_text(json.dumps(report, indent=2) + "\n")


def build(build_type: str, timings: dict[str, float] | None = None) -> None:
    if timings is None:
        timings = {}
    os.environ["APPIMAGE_EXTRACT_AND_RUN"] = "1"
    parallel = ["--parallel", os.environ["JOBS"]] if os.environ.get("JOBS") else ["--parallel"]
    source = Path("/project/source")
    output = Path("/project/build")
    common = ["-S", str(source), "-B", str(output), "-DPython3_EXECUTABLE=/opt/qgc-venv/bin/python"]
    android = bool(os.environ.get("ANDROID_SDK_ROOT"))
    cross = not android and bool(os.environ.get("SYSROOT"))
    if android:
        require_env(
            "QT_HOST_PATH",
            "QT_ROOT_DIR_ARM64",
            "ANDROID_SDK_ROOT",
            "ANDROID_NDK_ROOT",
            "ANDROID_PLATFORM",
            "ANDROID_MIN_SDK",
        )
        preset = {"Release": "Android", "Debug": "Android-debug"}.get(build_type)
        if preset:
            os.environ["QT_TARGET_ROOT_DIR"] = os.environ["QT_ROOT_DIR_ARM64"]
            os.environ["ANDROID_NDK"] = os.environ["ANDROID_NDK_ROOT"]
            configure = ["cmake", "--preset", preset, *common]
        else:
            # preset-exception: Android has no RelWithDebInfo or MinSizeRel preset.
            configure = [
                f"{os.environ['QT_ROOT_DIR_ARM64']}/bin/qt-cmake",
                *common,
                "-G",
                "Ninja",
                f"-DCMAKE_BUILD_TYPE={build_type}",
                f"-DQT_HOST_PATH={os.environ['QT_HOST_PATH']}",
                f"-DCMAKE_SYSTEM_VERSION={os.environ['ANDROID_MIN_SDK']}",
            ]
        configure += [
            f"-DQT_ANDROID_ABIS={os.environ.get('ANDROID_ABIS', 'arm64-v8a')}",
            f"-DANDROID_SDK_ROOT={os.environ['ANDROID_SDK_ROOT']}",
            "-DQT_ANDROID_SIGN_APK=OFF",
        ]
    elif cross:
        require_env("QT_HOST_PATH", "QT_ROOT_DIR", "SYSROOT")
        toolchain = source / "cmake/platform/Linux-aarch64-toolchain.cmake"
        if not toolchain.is_file():
            raise ValueError(f"Missing toolchain: {toolchain}; mount the source tree at {source}")
        if os.environ.get("CLEAN_BUILD") == "1":
            for child in output.iterdir():
                if child.is_dir() and not child.is_symlink():
                    shutil.rmtree(child)
                else:
                    child.unlink()
        # preset-exception: the dynamic sysroot differs from the Linux-arm64 SDK preset.
        configure = [
            f"{os.environ['QT_HOST_PATH']}/bin/qt-cmake",
            *common,
            "-G",
            "Ninja",
            f"-DCMAKE_BUILD_TYPE={build_type}",
            f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
            f"-DCMAKE_PREFIX_PATH={os.environ['QT_ROOT_DIR']}",
            f"-DQT_HOST_PATH={os.environ['QT_HOST_PATH']}",
            f"-DQGC_AARCH64_SYSROOT={os.environ['SYSROOT']}",
            "-DQGC_CREATE_APPIMAGE=OFF",
        ]
    else:
        preset = {
            "Release": "Linux",
            "Debug": "Linux-debug",
            "RelWithDebInfo": "Linux-relwithdebinfo",
        }.get(build_type)
        if preset:
            configure = ["cmake", "--preset", preset, *common]
        else:
            # preset-exception: Linux has no platform-specific MinSizeRel preset.
            configure = ["qt-cmake", *common, "-G", "Ninja", f"-DCMAKE_BUILD_TYPE={build_type}"]
    run_phase("configure", configure, timings)
    run_phase(
        "build",
        ["cmake", "--build", str(output), "--target", "all", "--config", build_type, *parallel],
        timings,
    )
    if not android:
        run_phase("install", ["cmake", "--install", str(output), "--config", build_type], timings)
        if not cross:
            run_phase(
                "package", ["cmake", "--build", str(output), "--target", "qgc-package"], timings
            )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "build_type",
        nargs="?",
        default=os.environ.get("BUILD_TYPE", "Release"),
        choices=("Release", "Debug", "RelWithDebInfo", "MinSizeRel"),
    )
    parser.add_argument(
        "--cache-key",
        action="store_true",
        help="Print the compiler cache fingerprint without building",
    )
    args = parser.parse_args(argv)
    timings: dict[str, float] = {}
    success = False
    try:
        if args.cache_key:
            print(compiler_fingerprint())
            return 0
        initialize_caches(Path("/project/source"))
        build(args.build_type, timings)
        success = True
        print("Build complete!")
        return 0
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Build failed: {error}", file=sys.stderr)
        return 1
    finally:
        if not args.cache_key:
            try:
                write_performance_report(
                    Path("/project/source"), Path("/project/build"), timings, success
                )
            except OSError as error:
                print(f"Could not write Docker performance report: {error}", file=sys.stderr)


if __name__ == "__main__":
    raise SystemExit(main())
