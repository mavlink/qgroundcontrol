"""Tests for setup_summary.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from ci.reporting import setup_summary as mod

from .._helpers import get_steps, load_action

if TYPE_CHECKING:
    from pathlib import Path

    from syrupy.assertion import SnapshotAssertion

pytestmark = pytest.mark.windows

LINUX_COVERAGE = {
    "RUNNER_OS": "Linux",
    "RUNNER_ARCH": "X64",
    "RUNNER_ENVIRONMENT": "self-hosted",
    "QT_VERSION": "6.12.0",
    "QT_ARCH": "linux_gcc_64",
    "QT_INSTALLER": "qgc",
    "QT_SOURCE": "cache",
    "CMAKE_VERSION": "4.1.2",
    "NINJA_VERSION": "1.13.1",
    "PYTHON_GROUPS": "scripts,coverage",
    "BUILD_CACHE": "true",
    "QGC_ACTIONS_CACHE_BACKEND": "s3",
    "QGC_COMPILER_CACHE_TOOL": "ccache",
    "QGC_SAVE_BUILD_CACHE": "true",
    "COMPILER_CACHE": "false",
    "MOC_CACHE": "true",
    "CPM_CACHE": "true",
}


def test_setup_section_matches_snapshot(
    monkeypatch: pytest.MonkeyPatch, gh_summary: Path, snapshot: SnapshotAssertion
) -> None:
    for key, value in LINUX_COVERAGE.items():
        monkeypatch.setenv(key, value)
    assert mod.main([]) == 0
    assert gh_summary.read_text(encoding="utf-8") == snapshot


def test_android_lists_target_qt_installs_and_the_gstreamer_sdk() -> None:
    facts = dict(
        mod.setup_facts(
            {
                **LINUX_COVERAGE,
                "QT_ANDROID_ARM64_V8A_SOURCE": "installed",
                "QT_ANDROID_ARMV7_SOURCE": "cache",
                "GSTREAMER_MODE": "restore",
                "GSTREAMER_CACHE": "",
                "QGC_SAVE_BUILD_CACHE": "false",
                "COMPILER_CACHE": "artifact",
            }
        )
    )
    assert facts["Qt for Android arm64-v8a"] == "downloaded"
    assert facts["Qt for Android armeabi-v7a"] == "restored from cache"
    assert facts["Qt for iOS"] == ""
    assert facts["GStreamer SDK"] == "miss: built from scratch"
    assert facts["Compiler cache"] == "ccache restored from the workflow artifact"
    assert facts["Build cache saves"] == "restore only"


def test_jobs_without_build_caches_and_the_sccache_pilot() -> None:
    no_cache = dict(mod.setup_facts({**LINUX_COVERAGE, "BUILD_CACHE": "false"}))
    assert "Compiler cache" not in no_cache
    assert no_cache["Cache backend"] == "RunsOn S3"
    sccache = dict(mod.setup_facts({**LINUX_COVERAGE, "QGC_COMPILER_CACHE_TOOL": "sccache"}))
    assert sccache["Compiler cache"] == "sccache reads S3 directly (no archive)"


def test_build_setup_summarizes_after_every_install() -> None:
    steps = get_steps(load_action("build-setup"))
    summary = steps[-1]
    assert summary["name"] == "Summarize setup"
    assert "setup_summary.py" in summary["run"]
    ids = {step.get("id") for step in steps}
    for value in summary["env"].values():
        for reference in value.split("steps.")[1:]:
            assert reference.split(".")[0] in ids
    cache_outputs = set(load_action("cache")["outputs"])
    assert {"compiler-cache", "moc-cache", "cpm-cache", "gstreamer-cache"} <= cache_outputs
    assert {"version", "installer", "source"} <= set(load_action("qt-install")["outputs"])
