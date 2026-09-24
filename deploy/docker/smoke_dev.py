#!/usr/bin/env python3
"""Exercise the prebuilt development environment without provisioning anything."""

from __future__ import annotations

import json
import os
import platform
import subprocess
import zipfile
from pathlib import Path

from packaging.version import Version


def run(*args: str) -> None:
    subprocess.run(args, check=True, timeout=180)


def android_deployment_tool_smoke(output: Path) -> None:
    result = subprocess.run(
        ["/opt/qt/bin/androiddeployqt", "--output", str(output), "--help"],
        capture_output=True,
        text=True,
        timeout=30,
    )
    # Qt returns SyntaxErrorOrHelpRequested (1) even for a valid --help invocation.
    if result.returncode != 1 or "Syntax: androiddeployqt --output" not in result.stderr:
        raise RuntimeError(
            f"androiddeployqt help failed ({result.returncode}): {result.stdout}{result.stderr}"
        )


def android_smoke(config: dict, work: Path) -> None:
    if platform.machine() == "aarch64":
        if os.environ.get("ANDROID_SDK_ROOT") or Path("/opt/android-ndk").exists():
            raise RuntimeError("Native ARM64 must not advertise an unusable Android toolchain")
        result = subprocess.run(
            ["qgc-android", "arm64-v8a", "true"], capture_output=True, text=True, timeout=30
        )
        if result.returncode == 0 or "--platform linux/amd64" not in result.stderr:
            raise RuntimeError("Native ARM64 must explain how to select Android support")
        print("Android omitted on native ARM64: use --platform linux/amd64", flush=True)
        return
    if platform.machine() != "x86_64":
        raise RuntimeError(f"Unsupported Android host: {platform.machine()}")
    android = config["android"]
    sdk = Path(os.environ["ANDROID_SDK_ROOT"])
    ndk = Path(os.environ["ANDROID_NDK_ROOT"])
    build_tools = Path(os.environ["ANDROID_BUILD_TOOLS_DIR"])
    if ndk.resolve() != sdk / "ndk" / android["ndk_full_version"]:
        raise RuntimeError("Android NDK does not match build-config")
    if build_tools.resolve() != sdk / "build-tools" / android["build_tools"]:
        raise RuntimeError("Android build-tools do not match build-config")
    java_version = subprocess.check_output(
        ["javac", "-version"], text=True, stderr=subprocess.STDOUT, timeout=30
    )
    if not java_version.startswith(f"javac {android['java_version']}."):
        raise RuntimeError(f"Java does not match build-config: {java_version}")
    run("sdkmanager", "--version")
    run("adb", "version")
    run(str(ndk / "toolchains/llvm/prebuilt/linux-x86_64/bin/clang++"), "--version")
    android_deployment_tool_smoke(work / "deployment")
    work.mkdir(exist_ok=True)
    (work / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.25)\n"
        "project(android_smoke LANGUAGES CXX)\n"
        f"find_package(Qt6 {config['qt']['version']} EXACT REQUIRED COMPONENTS Core TaskTree Qml)\n"
        "set(CMAKE_AUTOMOC ON)\n"
        "add_library(android_smoke SHARED main.cpp)\n"
        "target_link_libraries(android_smoke PRIVATE Qt6::Core)\n"
    )
    (work / "main.cpp").write_text(
        "#include <QObject>\n#include <QString>\n"
        "class Probe : public QObject {\n    Q_OBJECT\n};\n"
        'extern "C" int smoke() { Probe probe; return QStringLiteral("Qt").size(); }\n'
        '#include "main.moc"\n'
    )
    libraries = []
    for abi, machine in (("arm64-v8a", 183), ("armeabi-v7a", 40), ("x86_64", 62)):
        build = work / abi
        run(
            "qgc-android",
            abi,
            "cmake",
            "-S",
            str(work),
            "-B",
            str(build),
            "-G",
            "Ninja",
            f"-DCMAKE_TOOLCHAIN_FILE=/opt/qt-android/{abi}/lib/cmake/Qt6/qt.toolchain.cmake",
            f"-DANDROID_ABI={abi}",
            f"-DCMAKE_SYSTEM_VERSION={android['min_sdk']}",
            f"-DANDROID_NDK={ndk}",
            "-DQT_HOST_PATH=/opt/qt",
            "-DCMAKE_BUILD_TYPE=Release",
        )
        run("cmake", "--build", str(build), "--parallel", "2")
        library = build / "libandroid_smoke.so"
        header = library.read_bytes()[:20]
        if header[:4] != b"\x7fELF" or int.from_bytes(header[18:20], "little") != machine:
            raise RuntimeError(f"Wrong Android target architecture: {abi}")
        libraries.append((library, f"lib/{abi}/libandroid_smoke.so"))

    # Exercise packaging offline, without Gradle/Maven downloads or an emulator.
    resources = work / "res/values"
    resources.mkdir(parents=True, exist_ok=True)
    (resources / "strings.xml").write_text(
        '<resources><string name="app_name">QGC toolchain smoke</string></resources>\n'
    )
    manifest = work / "AndroidManifest.xml"
    manifest.write_text(
        '<manifest xmlns:android="http://schemas.android.com/apk/res/android" '
        'package="org.qgroundcontrol.toolchainsmoke">'
        f'<uses-sdk android:minSdkVersion="{android["min_sdk"]}" '
        f'android:targetSdkVersion="{android["platform"]}"/>'
        '<application android:label="@string/app_name" android:hasCode="false"/>'
        "</manifest>\n"
    )
    android_jar = sdk / f"platforms/android-{android['platform']}/android.jar"
    run("aapt2", "compile", "--dir", str(work / "res"), "-o", str(work / "resources.zip"))
    run(
        "aapt2",
        "link",
        "-I",
        str(android_jar),
        "--manifest",
        str(manifest),
        "-o",
        str(work / "unsigned.apk"),
        str(work / "resources.zip"),
    )
    with zipfile.ZipFile(work / "unsigned.apk", "a") as apk:
        for library, destination in libraries:
            apk.write(library, destination)
    run("zipalign", "-f", "4", str(work / "unsigned.apk"), str(work / "aligned.apk"))
    key = work / "smoke.keystore"
    key.unlink(missing_ok=True)
    run(
        "keytool",
        "-genkeypair",
        "-keystore",
        str(key),
        "-storepass",
        "android",
        "-keypass",
        "android",
        "-alias",
        "smoke",
        "-dname",
        "CN=Toolchain smoke",
        "-keyalg",
        "RSA",
        "-validity",
        "1",
        "-noprompt",
    )
    run(
        "apksigner",
        "sign",
        "--ks",
        str(key),
        "--ks-pass",
        "pass:android",
        "--key-pass",
        "pass:android",
        "--out",
        str(work / "signed.apk"),
        str(work / "aligned.apk"),
    )
    run("apksigner", "verify", "--verbose", str(work / "signed.apk"))
    run("zipalign", "-c", "4", str(work / "signed.apk"))
    print("Android Qt compile/link and offline APK packaging passed for all three ABIs", flush=True)


def main() -> None:
    if os.getuid() == 0:
        raise RuntimeError("Development smoke must run as the non-root image user")
    config = json.loads(Path("/opt/qgc-bootstrap/tools/setup/build-config.json").read_text())
    machine = {"x86_64": 62, "aarch64": 183}[platform.machine()]
    for executable in (
        "/opt/llvm/bin/clang",
        "/opt/llvm/bin/clangd",
        "/opt/llvm/bin/clang-tidy",
        "/opt/llvm/bin/clang-scan-deps",
        "/opt/clazy/bin/clazy-standalone",
        "/opt/qt/libexec/moc",
        "/opt/qgc-venv/bin/python",
        "/usr/bin/g++",
        "/usr/local/bin/ccache",
    ):
        header = Path(executable).read_bytes()[:20]
        if header[:4] != b"\x7fELF" or int.from_bytes(header[18:20], "little") != machine:
            raise RuntimeError(f"Not a native {platform.machine()} executable: {executable}")
    run("python3", "/opt/qgc-bootstrap/deploy/docker/install_analysis.py", "--verify")
    run("clangd", "--version")
    run("ccache", "--version")
    run("python3", "-c", "import jinja2, httpx, defusedxml, lxml, fastcrc, pytest, pymavlink")
    run("just", "--version")
    run("gh", "--version")
    run(
        "uv",
        "run",
        "--offline",
        "--project",
        "/opt/qgc-tools",
        "--group",
        "build",
        "cmake",
        "--version",
    )
    work = Path.home() / "qgc-dev-smoke"
    work.mkdir(exist_ok=True)
    (work / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.25)\n"
        "project(dev_smoke LANGUAGES CXX)\n"
        f"find_package(Qt6 {config['qt']['version']} EXACT REQUIRED COMPONENTS Core TaskTree Qml)\n"
        "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n"
        "add_executable(dev_smoke main.cpp)\n"
        "target_link_libraries(dev_smoke PRIVATE Qt6::Core)\n"
    )
    source = work / "main.cpp"
    source.write_text(
        '#include <QString>\nint main() { return QStringLiteral("Qt").size() != 2; }\n'
    )
    run("qt-cmake", "-S", str(work), "-B", str(work / "build"), "-G", "Ninja")
    run("cmake", "--build", str(work / "build"), "--parallel", str(os.cpu_count() or 1))
    run(str(work / "build/dev_smoke"))
    run("clang-tidy", str(source), "-p", str(work / "build"))
    run("clazy-standalone", str(source), "-p", str(work / "build"))
    gst_version = subprocess.check_output(
        ["pkg-config", "--modversion", "gstreamer-1.0"], text=True
    ).strip()
    if Version(gst_version) < Version(config["gstreamer"]["version"]["minimum"]):
        raise RuntimeError(f"GStreamer {gst_version} is below the configured minimum")
    print(f"GStreamer {gst_version}")
    android_smoke(config, work / "android")
    print(f"qgc-dev native {platform.machine()} non-root smoke passed", flush=True)


if __name__ == "__main__":
    main()
