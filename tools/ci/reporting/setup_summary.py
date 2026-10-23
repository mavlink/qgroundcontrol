#!/usr/bin/env python3
"""Write the job summary's Setup section for build-setup.

Reads the runner, Qt, CMake, Python, and cache-restore results that build-setup passes in its
step environment, so each build job's summary starts with the machine it ran on (image, instance,
CPUs, and memory) and how its toolchain and caches arrived. It also runs after a failed setup
step, which it names.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import TYPE_CHECKING, NamedTuple

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.build_config import get_build_config_value
from common.gh_actions import parse_bool, parse_csv_list, write_step_summary
from common.markdown import code_span, summary_section
from common.proc import run_captured

if TYPE_CHECKING:
    from collections.abc import Mapping

INSTALLERS = {"qgc": "QGC installer", "aqt": "aqtinstall"}
QT_SOURCES = {
    "preinstalled": "preinstalled in the runner image",
    "cache": "restored from cache",
    "installed": "downloaded",
}
# Extra Qt installs: (environment variable, label).
EXTRA_QT = (
    ("QT_HOST_SOURCE", "Host Qt (x64 tools)"),
    ("QT_IOS_SOURCE", "Qt for iOS"),
    ("QT_ANDROID_ARM64_V8A_SOURCE", "Qt for Android arm64-v8a"),
    ("QT_ANDROID_ARMV7_SOURCE", "Qt for Android armeabi-v7a"),
    ("QT_ANDROID_X86_64_SOURCE", "Qt for Android x86_64"),
)
BACKENDS = {"s3": "RunsOn S3", "github": "GitHub Actions cache"}
# build-setup's step ids (the summary reads their outcomes from `toJSON(steps)`), named as in its
# log; unlisted ids show as themselves.
STEP_LABELS = {
    "plan": "Plan build setup",
    "free-disk-space": "Free disk space (Linux)",
    "python": "Set up Python",
    "cmake": "Install CMake and Ninja",
    "java": "Setup Java Environment",
    "gradle": "Setup Gradle (Android)",
    "cache": "Setup caching",
    "xcode": "Setup Xcode",
    "qt": "Install Qt",
    "qt-desktop-host": "Install host Qt (Windows ARM64 cross-compile)",
    "qt-ios": "Install Qt for iOS",
    "msvc": "Setup MSVC environment (Windows desktop)",
    "rust-linker": "Fix Rust linker resolution (Windows ARM64 native)",
    "android": "Setup Android Environment",
    "android-sdk": "Install Android SDK / NDK / Tools",
    "qt-android-arm64": "Install Qt for Android (arm64_v8a)",
    "qt-android-armv7": "Install Qt for Android (armv7)",
    "qt-android-x86_64": "Install Qt for Android (x86_64)",
}
GIB = 1024**3


class Host(NamedTuple):
    """What the summary measures on the runner rather than reads from its environment."""

    cpus: int | None = None
    memory: int | None = None
    compiler: str = ""
    xcode: str = ""


def _total_memory() -> int | None:
    if sys.platform == "win32":
        import ctypes

        kilobytes = ctypes.c_ulonglong()
        if ctypes.windll.kernel32.GetPhysicallyInstalledSystemMemory(ctypes.byref(kilobytes)):
            return kilobytes.value * 1024
        return None
    try:
        return os.sysconf("SC_PAGE_SIZE") * os.sysconf("SC_PHYS_PAGES")
    except (AttributeError, OSError, ValueError):
        return None


def _first_line(command: list[str]) -> str:
    try:
        result = run_captured(command, timeout=30)
    except (OSError, subprocess.TimeoutExpired):
        return ""
    lines = result.stdout.strip().splitlines() if result.returncode == 0 else []
    return lines[0].strip() if lines else ""


def probe_host(env: Mapping[str, str]) -> Host:
    """Measure the CPUs and memory, and on macOS and Linux ask the compiler and Xcode versions.

    MSVC reports its version in the environment, and Android builds use the NDK's compiler.
    """
    runner_os = env.get("RUNNER_OS", "")
    native = env.get("SETUP_MODE") != "android"
    compiler = xcode = ""
    if native and runner_os != "Windows":
        compiler = _first_line([env.get("CXX") or "c++", "--version"])
    if native and runner_os == "macOS":
        xcode = _first_line(["xcodebuild", "-version"]).removeprefix("Xcode ")
    return Host(os.cpu_count(), _total_memory(), compiler, xcode)


def failed_steps(steps_json: str) -> list[str]:
    """Name the steps whose outcome was failure in build-setup's `toJSON(steps)`."""
    try:
        steps = json.loads(steps_json or "{}")
    except ValueError:
        return []
    if not isinstance(steps, dict):
        return []
    return [
        STEP_LABELS.get(str(step_id), str(step_id))
        for step_id, step in steps.items()
        if isinstance(step, dict) and step.get("outcome") == "failure"
    ]


def restore_result(value: str) -> str:
    """Describe a cache action restore output: artifact, true, false, or empty."""
    return {
        "artifact": "restored from the workflow artifact",
        "true": "restored (exact key)",
        "false": "restored (fallback key)",
    }.get(value.strip(), "miss: built from scratch")


def fallback_key(result: str, matched_key: str) -> str:
    """Name the restore-keys match a cache came from; an exact hit is the primary key itself."""
    return (
        code_span(matched_key.strip()) if result.strip() == "false" and matched_key.strip() else ""
    )


def _join(*parts: str, sep: str = ", ") -> str:
    return sep.join(part for part in parts if part)


def _with_request(actual: str, requested: str) -> str:
    """Return the installed version and the build-config.json request it satisfied."""
    if actual and requested and actual != requested:
        return f"{actual} (requested {requested})"
    return actual or (f"requested {requested}" if requested else "")


def setup_facts(env: Mapping[str, str], host: Host | None = None) -> list[tuple[str, object]]:
    def get(name: str) -> str:
        return env.get(name, "").strip()

    host = host or Host()
    native = get("SETUP_MODE") != "android"
    runner = " ".join(part for part in (get("RUNNER_OS"), get("RUNNER_ARCH")) if part)
    if get("RUNNER_ENVIRONMENT"):
        runner += f" ({get('RUNNER_ENVIRONMENT')})"
    runner_name = get("RUNS_ON_RUNNER_NAME") or get("RUNNER_NAME")
    runs_on_image = get("RUNS_ON_IMAGE_ID") or get("RUNS_ON_IMAGE_NAME") or get("RUNS_ON_AMI_NAME")
    if get("VCToolsVersion"):
        visual_studio = f" (Visual Studio {get('VSCMD_VER')})" if get("VSCMD_VER") else ""
        compiler = f"MSVC {get('VCToolsVersion')}{visual_studio}"
    else:
        compiler = host.compiler
    android_ndk = ""
    if not native:
        ndk = get_build_config_value("android.ndk_version")
        ndk_full = get_build_config_value("android.ndk_full_version")
        android_ndk = f"{ndk} ({ndk_full})" if ndk and ndk_full else ndk or ndk_full
    qt = " ".join(part for part in (get("QT_VERSION"), get("QT_ARCH")) if part)
    how = [
        INSTALLERS.get(get("QT_INSTALLER"), get("QT_INSTALLER")),
        QT_SOURCES.get(get("QT_SOURCE"), ""),
    ]
    if qt and any(how):
        qt += f": {', '.join(part for part in how if part)}"
    failed = failed_steps(get("SETUP_STEPS"))
    facts: list[tuple[str, object]] = [
        ("Failed setup step", f"❌ {', '.join(failed)}" if failed else ""),
        ("Runner", runner.strip()),
        ("Runner name", code_span(runner_name) if runner_name else ""),
        (
            "Image",
            _join(
                runs_on_image,
                get("RUNS_ON_AMI_ID"),
                _join(get("ImageOS"), get("ImageVersion"), sep=" "),
            ),
        ),
        (
            "Instance",
            _join(
                get("RUNS_ON_INSTANCE_TYPE"),
                get("RUNS_ON_INSTANCE_LIFECYCLE"),
                get("RUNS_ON_AWS_AZ") or get("RUNS_ON_AWS_REGION"),
                get("RUNS_ON_INSTANCE_ID"),
            ),
        ),
        (
            "Hardware",
            _join(
                f"{host.cpus} CPUs" if host.cpus else "",
                f"{host.memory / GIB:.1f} GiB RAM" if host.memory else "",
            ),
        ),
        ("Qt", qt),
        *((label, QT_SOURCES.get(get(name), get(name))) for name, label in EXTRA_QT),
        (
            "CMake",
            f"{get('CMAKE_VERSION')}, Ninja {get('NINJA_VERSION')}" if get("CMAKE_VERSION") else "",
        ),
        ("Compiler", code_span(compiler) if compiler else ""),
        (
            "Xcode",
            _with_request(host.xcode, get("XCODE_VERSION"))
            if native and get("RUNNER_OS") == "macOS"
            else "",
        ),
        ("JDK", "" if native else _with_request(get("JDK_VERSION"), get("JDK_REQUESTED"))),
        ("Android NDK", android_ndk),
        ("Python groups", ", ".join(parse_csv_list(get("PYTHON_GROUPS")))),
        (
            "Cache backend",
            BACKENDS.get(get("QGC_ACTIONS_CACHE_BACKEND"), get("QGC_ACTIONS_CACHE_BACKEND")),
        ),
    ]
    if not parse_bool(get("BUILD_CACHE") or "true"):
        return facts
    ccache_key = ""
    ccache_fallback = ""
    if get("QGC_COMPILER_CACHE_TOOL") == "sccache":
        compiler_cache = "sccache reads S3 directly (no archive)"
    else:
        compiler_cache = f"ccache {restore_result(get('COMPILER_CACHE'))}"
        if get("COMPILER_CACHE") != "artifact" and get("QGC_CCACHE_KEY"):
            ccache_key = code_span(get("QGC_CCACHE_KEY"))
        ccache_fallback = fallback_key(get("COMPILER_CACHE"), get("COMPILER_CACHE_KEY"))
    saves = get("QGC_SAVE_BUILD_CACHE")
    facts += [
        ("Compiler cache", compiler_cache),
        ("ccache primary key", ccache_key),
        ("ccache restored key", ccache_fallback),
        ("moc cache", restore_result(get("MOC_CACHE"))),
        ("moc restored key", fallback_key(get("MOC_CACHE"), get("MOC_CACHE_KEY"))),
        ("CPM sources", restore_result(get("CPM_CACHE"))),
        ("GStreamer SDK", restore_result(get("GSTREAMER_CACHE")) if get("GSTREAMER_MODE") else ""),
        (
            "Build cache saves",
            "after a successful build" if parse_bool(saves) else "restore only" if saves else "",
        ),
    ]
    return facts


def main(argv: list[str] | None = None) -> int:
    argparse.ArgumentParser(description=__doc__).parse_args(argv)
    section = summary_section("Setup", setup_facts(os.environ, probe_host(os.environ)))
    print(section)
    write_step_summary(section)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
