#!/usr/bin/env python3
"""Exercise the prebuilt development environment without provisioning anything."""

from __future__ import annotations

import json
import os
import platform
import subprocess
from pathlib import Path

from packaging.version import Version


def run(*args: str) -> None:
    subprocess.run(args, check=True)


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
        f"find_package(Qt6 {config['qt']['version']} EXACT REQUIRED COMPONENTS Core)\n"
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
    print(f"qgc-dev native {platform.machine()} non-root smoke passed", flush=True)


if __name__ == "__main__":
    main()
