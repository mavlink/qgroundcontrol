#!/usr/bin/env python3
"""Write the job summary's Setup section for build-setup.

Reads the runner, Qt, CMake, Python, and cache-restore results that build-setup passes in its
step environment, so each build job's summary starts with how its toolchain and caches arrived.
"""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path
from typing import TYPE_CHECKING

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import parse_bool, parse_csv_list, write_step_summary
from common.markdown import summary_section

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


def restore_result(value: str) -> str:
    """Describe a cache action restore output: artifact, true, false, or empty."""
    return {
        "artifact": "restored from the workflow artifact",
        "true": "restored (exact key)",
        "false": "restored (fallback key)",
    }.get(value.strip(), "miss: built from scratch")


def setup_facts(env: Mapping[str, str]) -> list[tuple[str, object]]:
    def get(name: str) -> str:
        return env.get(name, "").strip()

    runner = " ".join(part for part in (get("RUNNER_OS"), get("RUNNER_ARCH")) if part)
    if get("RUNNER_ENVIRONMENT"):
        runner += f" ({get('RUNNER_ENVIRONMENT')})"
    qt = " ".join(part for part in (get("QT_VERSION"), get("QT_ARCH")) if part)
    how = [
        INSTALLERS.get(get("QT_INSTALLER"), get("QT_INSTALLER")),
        QT_SOURCES.get(get("QT_SOURCE"), ""),
    ]
    if qt and any(how):
        qt += f": {', '.join(part for part in how if part)}"
    facts: list[tuple[str, object]] = [
        ("Runner", runner.strip()),
        ("Qt", qt),
        *((label, QT_SOURCES.get(get(name), get(name))) for name, label in EXTRA_QT),
        (
            "CMake",
            f"{get('CMAKE_VERSION')}, Ninja {get('NINJA_VERSION')}" if get("CMAKE_VERSION") else "",
        ),
        ("Python groups", ", ".join(parse_csv_list(get("PYTHON_GROUPS")))),
        (
            "Cache backend",
            BACKENDS.get(get("QGC_ACTIONS_CACHE_BACKEND"), get("QGC_ACTIONS_CACHE_BACKEND")),
        ),
    ]
    if not parse_bool(get("BUILD_CACHE") or "true"):
        return facts
    if get("QGC_COMPILER_CACHE_TOOL") == "sccache":
        compiler = "sccache reads S3 directly (no archive)"
    else:
        compiler = f"ccache {restore_result(get('COMPILER_CACHE'))}"
    saves = get("QGC_SAVE_BUILD_CACHE")
    facts += [
        ("Compiler cache", compiler),
        ("moc cache", restore_result(get("MOC_CACHE"))),
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
    section = summary_section("Setup", setup_facts(os.environ))
    print(section)
    write_step_summary(section)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
