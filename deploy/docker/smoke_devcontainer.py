#!/usr/bin/env python3
"""Smoke-test a devcontainer image from inside it, against this checkout's build-config.json.

devcontainer.yml runs it in each image it builds:
    docker run --rm -v "$PWD:/workspaces/qgroundcontrol:ro" qgc-dev:ci \
        bash -lc 'python3 deploy/docker/smoke_devcontainer.py'

It checks the pinned tool versions, builds and runs a small Qt program with the default compiler
and with the analysis Clang, runs clang-tidy and Clazy over it, and on x86_64 configures and
compiles it for Android arm64 through qgc-android. On arm64, qgc-android must explain that
Android needs the x86_64 image. Standard library only: it runs with the image's own Python.
"""

from __future__ import annotations

import json
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

CONFIG = json.loads(
    (Path(__file__).resolve().parents[2] / ".github/build-config.json").read_text(encoding="utf-8")
)
TIMEOUT_SECONDS = 900

CMAKELISTS = """cmake_minimum_required(VERSION 3.25)
project(smoke LANGUAGES CXX)
find_package(Qt6 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup()
qt_add_executable(smoke main.cpp)
target_link_libraries(smoke PRIVATE Qt6::Core Qt6::Qml)
"""
MAIN_CPP = """#include <QtCore/QCoreApplication>
#include <QtQml/QJSEngine>
#include <cstdio>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QJSEngine engine;
    std::printf("%d\\n", engine.evaluate(QStringLiteral("6 * 7")).toInt());
    return 0;
}
"""


def run(*command: str, cwd: Path | None = None, check: bool = True) -> str:
    print(f"$ {' '.join(command)}", flush=True)
    result = subprocess.run(
        command, cwd=cwd, capture_output=True, text=True, timeout=TIMEOUT_SECONDS, check=False
    )
    if check and result.returncode:
        raise RuntimeError(
            f"{command[0]} exited {result.returncode}:\n{result.stdout}{result.stderr}"
        )
    return result.stdout + result.stderr


def expect(output: str, pattern: str, what: str) -> None:
    if not re.search(pattern, output, re.MULTILINE):
        raise RuntimeError(f"{what}: expected /{pattern}/ in:\n{output}")


def check_tools() -> None:
    if os.getuid() == 0:
        raise RuntimeError("the devcontainer must run as its non-root user")
    analysis = CONFIG["analysis"]
    llvm = re.escape(analysis["llvm_version"])
    expect(run("qtpaths", "--qt-version"), rf"^{re.escape(CONFIG['qt']['version'])}$", "Qt")
    expect(
        run("ccache", "--version"),
        rf"^ccache version {re.escape(CONFIG['tools']['ccache']['version'])}$",
        "ccache",
    )
    expect(
        run("clang-tidy", "--version"),
        rf"LLVM version {re.escape(analysis['clang_tidy_version'])}\b",
        "clang-tidy",
    )
    expect(run("clazy-standalone", "--version"), rf"LLVM version {llvm}\.", "Clazy")
    expect(run("clang-scan-deps", "--version"), rf"LLVM version {llvm}\.", "clang-scan-deps")
    expect(run("clangd", "--version"), rf"clangd version {llvm}\.", "clangd")
    run("cmake", "--version")
    run("ninja", "--version")


def build_and_run(source: Path, build: Path, *compiler: str) -> None:
    run(
        "cmake",
        "-S",
        str(source),
        "-B",
        str(build),
        "-G",
        "Ninja",
        "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        *compiler,
    )
    run("cmake", "--build", str(build))
    expect(run(str(build / "smoke")), r"^42$", f"Qt program built in {build.name}")


def check_desktop(work: Path, source: Path) -> None:
    build_and_run(source, work / "gcc")
    clang = work / "clang"
    build_and_run(source, clang, "-DCMAKE_CXX_COMPILER=clang++")
    run(
        "clang-tidy",
        "--quiet",
        "-p",
        str(clang),
        "--checks=-*,readability-*",
        str(source / "main.cpp"),
    )
    run("clazy-standalone", "-p", str(clang), str(source / "main.cpp"))


def check_android(work: Path, source: Path) -> None:
    if platform.machine() != "x86_64":
        refusal = subprocess.run(
            ["qgc-android", "arm64-v8a", "true"], capture_output=True, text=True, check=False
        )
        if refusal.returncode == 0 or "--platform linux/amd64" not in refusal.stderr:
            raise RuntimeError(
                f"qgc-android must explain the x86_64 requirement:\n{refusal.stderr}"
            )
        if Path("/opt/android-sdk").exists():
            raise RuntimeError("only the x86_64 image may carry the Android SDK")
        print("Android omitted on this architecture, as expected", flush=True)
        return
    android = CONFIG["android"]
    environment = run("qgc-android", "arm64-v8a", "env")
    for name, value in (
        ("ANDROID_MIN_SDK", android["min_sdk"]),
        ("ANDROID_NDK_ROOT", f"/opt/android-sdk/ndk/{android['ndk_full_version']}"),
        ("ANDROID_BUILD_TOOLS_DIR", f"/opt/android-sdk/build-tools/{android['build_tools']}"),
    ):
        expect(environment, rf"^{name}={re.escape(value)}$", name)
    expect(
        run("qgc-android", "x86_64", "javac", "-version"),
        rf"^javac {android['java_version']}\.",
        "JDK",
    )
    run("qgc-android", "x86_64", "sdkmanager", "--version")
    for abi in ("arm64-v8a", "armeabi-v7a", "x86_64"):
        run("qgc-android", abi, "true")
    build = work / "android"
    run(
        "qgc-android",
        "arm64-v8a",
        "bash",
        "-c",
        f'"$QT_TARGET_ROOT_DIR/bin/qt-cmake" -S {source} -B {build} -G Ninja'
        ' -DQT_HOST_PATH="$QT_HOST_PATH" -DANDROID_SDK_ROOT="$ANDROID_SDK_ROOT"'
        ' -DANDROID_NDK_ROOT="$ANDROID_NDK_ROOT"',
    )
    run("qgc-android", "arm64-v8a", "cmake", "--build", str(build), "--target", "smoke")
    if not list(build.glob("**/libsmoke_arm64-v8a.so")):
        raise RuntimeError("the Android build produced no arm64-v8a library")


def main(argv: list[str] | None = None) -> int:
    if argv:
        print(f"Usage: {Path(__file__).name} (no arguments)", file=sys.stderr)
        return 2
    if not shutil.which("qgc-android"):
        print("qgc-android is missing from PATH", file=sys.stderr)
        return 1
    with tempfile.TemporaryDirectory(prefix="qgc-smoke-") as temporary:
        work = Path(temporary)
        source = work / "source"
        source.mkdir()
        (source / "CMakeLists.txt").write_text(CMAKELISTS, encoding="utf-8")
        (source / "main.cpp").write_text(MAIN_CPP, encoding="utf-8")
        try:
            check_tools()
            check_desktop(work, source)
            check_android(work, source)
        except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
            print(f"Smoke test failed: {error}", file=sys.stderr)
            return 1
    print("Devcontainer smoke test passed", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
