"""Tests for setup_summary.py."""

from __future__ import annotations

import json
import os
import sys
from typing import TYPE_CHECKING

import pytest
from ci.reporting import setup_summary as mod
from common.build_config import get_build_config_value

from .._helpers import get_steps, load_action

if TYPE_CHECKING:
    from pathlib import Path

    from syrupy.assertion import SnapshotAssertion

pytestmark = pytest.mark.windows

LINUX_COVERAGE = {
    "RUNNER_OS": "Linux",
    "RUNNER_ARCH": "X64",
    "RUNNER_ENVIRONMENT": "self-hosted",
    "RUNNER_NAME": "runs-on--i-0123456789abcdef0",
    "RUNS_ON_IMAGE_ID": "ubuntu24-full-x64",
    "RUNS_ON_AMI_ID": "ami-0abc1234def567890",
    "RUNS_ON_INSTANCE_TYPE": "c8i.xlarge",
    "RUNS_ON_INSTANCE_LIFECYCLE": "on-demand",
    "RUNS_ON_AWS_AZ": "us-east-1b",
    "RUNS_ON_INSTANCE_ID": "i-0123456789abcdef0",
    "SETUP_MODE": "desktop",
    "QT_VERSION": "6.12.0",
    "QT_ARCH": "linux_gcc_64",
    "QT_INSTALLER": "qgc",
    "QT_SOURCE": "cache",
    "CMAKE_VERSION": "4.1.2",
    "NINJA_VERSION": "1.13.1",
    "XCODE_VERSION": "26.x",
    "JDK_REQUESTED": "21",
    "PYTHON_GROUPS": "scripts,coverage",
    "BUILD_CACHE": "true",
    "QGC_ACTIONS_CACHE_BACKEND": "s3",
    "QGC_COMPILER_CACHE_TOOL": "ccache",
    "QGC_CCACHE_KEY": "ccache-linux-gcc_64-Debug-coverage-pr-15370-1a2b3c-v2",
    "QGC_SAVE_BUILD_CACHE": "true",
    "COMPILER_CACHE": "false",
    "MOC_CACHE": "true",
    "CPM_CACHE": "true",
}
LINUX_HOST = mod.Host(
    cpus=4, memory=8 * mod.GIB, compiler="c++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0"
)
HOST_ONLY_ENV = ("ImageOS", "ImageVersion", "VCToolsVersion", "VSCMD_VER", "CXX")


def test_setup_section_matches_snapshot(
    monkeypatch: pytest.MonkeyPatch, gh_summary: Path, snapshot: SnapshotAssertion
) -> None:
    # A hosted runner's own image, toolchain, RunsOn, and QGC cache variables would leak in.
    # Windows upper-cases os.environ keys, and its delenv matches any case.
    for key in list(os.environ):
        if key.upper().startswith(("RUNS_ON_", "QGC_")):
            monkeypatch.delenv(key)
    for key in HOST_ONLY_ENV:
        monkeypatch.delenv(key, raising=False)
    for key, value in LINUX_COVERAGE.items():
        monkeypatch.setenv(key, value)
    monkeypatch.setattr(mod, "probe_host", lambda env: LINUX_HOST)
    assert mod.main([]) == 0
    assert gh_summary.read_text(encoding="utf-8") == snapshot


def test_hosted_runners_report_their_image_and_windows_its_msvc() -> None:
    hosted = {
        "RUNNER_OS": "Windows",
        "RUNNER_ARCH": "X64",
        "RUNNER_ENVIRONMENT": "github-hosted",
        "RUNNER_NAME": "GitHub Actions 12",
        "ImageOS": "win25",
        "ImageVersion": "20261019.1",
        "VCToolsVersion": "14.44.35207",
        "VSCMD_VER": "17.14.17",
    }
    facts = dict(mod.setup_facts(hosted, mod.Host(cpus=4, memory=16 * mod.GIB, compiler="gcc")))
    assert facts["Runner"] == "Windows X64 (github-hosted)"
    assert facts["Runner name"] == "`GitHub Actions 12`"
    assert facts["Image"] == "win25 20261019.1"
    assert facts["Instance"] == ""
    assert facts["Hardware"] == "4 CPUs, 16.0 GiB RAM"
    assert facts["Compiler"] == "`MSVC 14.44.35207 (Visual Studio 17.14.17)`"
    assert facts["Xcode"] == facts["JDK"] == facts["Android NDK"] == ""


def test_macos_reports_the_selected_xcode_against_the_request() -> None:
    macos = {"RUNNER_OS": "macOS", "SETUP_MODE": "ios", "XCODE_VERSION": "26.x"}
    facts = dict(mod.setup_facts(macos, mod.Host(xcode="26.0.1", compiler="Apple clang 17")))
    assert facts["Xcode"] == "26.0.1 (requested 26.x)"
    assert facts["Compiler"] == "`Apple clang 17`"
    # setup-xcode failed: the request still shows.
    assert dict(mod.setup_facts(macos))["Xcode"] == "requested 26.x"


def test_failed_setup_steps_are_named_from_the_step_outcomes() -> None:
    steps = {
        "plan": {"outcome": "success", "conclusion": "success", "outputs": {}},
        "qt": {"outcome": "failure", "conclusion": "failure", "outputs": {}},
        "qt-ios": {"outcome": "skipped", "conclusion": "skipped", "outputs": {}},
        "new-step": {"outcome": "failure", "conclusion": "failure", "outputs": {}},
    }
    facts = mod.setup_facts({**LINUX_COVERAGE, "SETUP_STEPS": json.dumps(steps)})
    assert facts[0] == ("Failed setup step", "❌ Install Qt, new-step")
    assert dict(mod.setup_facts(LINUX_COVERAGE))["Failed setup step"] == ""
    for steps_json in ("", "not json", "[]", json.dumps({"qt": {"outcome": "success"}})):
        assert mod.failed_steps(steps_json) == []


def test_runs_on_runner_name_and_instance_region_fallbacks() -> None:
    env = {
        **LINUX_COVERAGE,
        "RUNS_ON_RUNNER_NAME": "runs-on-runner",
        "RUNS_ON_AWS_AZ": "",
        "RUNS_ON_AWS_REGION": "us-east-1",
        "RUNS_ON_IMAGE_ID": "",
        "RUNS_ON_AMI_NAME": "runs-on-v2.2-ubuntu24-full-x64-20261001",
        "ImageOS": "ubuntu24",
        "ImageVersion": "20261012.1",
    }
    facts = dict(mod.setup_facts(env))
    assert facts["Runner name"] == "`runs-on-runner`"
    assert facts["Instance"] == "c8i.xlarge, on-demand, us-east-1, i-0123456789abcdef0"
    assert facts["Image"] == (
        "runs-on-v2.2-ubuntu24-full-x64-20261001, ami-0abc1234def567890, ubuntu24 20261012.1"
    )
    assert facts["Hardware"] == ""


def test_probe_host_measures_this_machine_and_asks_the_compiler(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    # Any executable that answers --version stands in for the compiler.
    host = mod.probe_host({"RUNNER_OS": "Linux", "SETUP_MODE": "desktop", "CXX": sys.executable})
    assert host.cpus == os.cpu_count()
    assert host.memory is not None
    assert host.memory > 1 * mod.GIB
    assert host.compiler.startswith("Python 3.")
    assert host.xcode == ""
    missing = str(tmp_path / "no-such-compiler")
    assert mod.probe_host({"RUNNER_OS": "Linux", "CXX": missing}).compiler == ""
    calls: list[list[str]] = []
    monkeypatch.setattr(mod, "_first_line", lambda command: calls.append(command) or "")
    mod.probe_host({"RUNNER_OS": "Windows", "SETUP_MODE": "desktop"})
    mod.probe_host({"RUNNER_OS": "Linux", "SETUP_MODE": "android"})
    assert calls == []


def test_android_lists_target_qt_installs_and_the_gstreamer_sdk() -> None:
    facts = dict(
        mod.setup_facts(
            {
                **LINUX_COVERAGE,
                "SETUP_MODE": "android",
                "JDK_VERSION": "21.0.8+9",
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
    # The artifact is not the dependency cache, so its key would mislead.
    assert facts["ccache primary key"] == ""
    assert facts["Build cache saves"] == "restore only"
    assert facts["JDK"] == "21.0.8+9 (requested 21)"
    ndk = get_build_config_value("android.ndk_version")
    assert facts["Android NDK"] == f"{ndk} ({get_build_config_value('android.ndk_full_version')})"
    assert ndk.startswith("r")


def test_jobs_without_build_caches_and_the_sccache_pilot() -> None:
    no_cache = dict(mod.setup_facts({**LINUX_COVERAGE, "BUILD_CACHE": "false"}))
    assert "Compiler cache" not in no_cache
    assert no_cache["Cache backend"] == "RunsOn S3"
    sccache = dict(mod.setup_facts({**LINUX_COVERAGE, "QGC_COMPILER_CACHE_TOOL": "sccache"}))
    assert sccache["Compiler cache"] == "sccache reads S3 directly (no archive)"
    assert sccache["ccache primary key"] == ""


def test_fallback_restores_name_the_key_they_came_from() -> None:
    facts = dict(
        mod.setup_facts(
            {
                **LINUX_COVERAGE,
                "COMPILER_CACHE_KEY": "ccache-linux-gcc_64-Debug-coverage-master-9f8e7d-v2",
                "MOC_CACHE_KEY": "moccache-linux-v3-Debug-coverage-41-1",
            }
        )
    )
    assert facts["Compiler cache"] == "ccache restored (fallback key)"
    assert facts["ccache restored key"] == "`ccache-linux-gcc_64-Debug-coverage-master-9f8e7d-v2`"
    # An exact hit restored the primary key, already shown.
    assert facts["moc restored key"] == ""
    miss = dict(mod.setup_facts({**LINUX_COVERAGE, "COMPILER_CACHE": "", "COMPILER_CACHE_KEY": ""}))
    assert miss["ccache restored key"] == ""


def test_build_setup_summarizes_after_every_install() -> None:
    steps = get_steps(load_action("build-setup"))
    summary = steps[-1]
    assert summary["name"] == "Summarize setup"
    assert "setup_summary.py" in summary["run"]
    ids = {step.get("id") for step in steps}
    for value in summary["env"].values():
        for reference in value.split("steps.")[1:]:
            assert reference.split(".")[0] in ids
    cache_outputs = load_action("cache")["outputs"]
    assert {"compiler-cache", "moc-cache", "cpm-cache", "gstreamer-cache"} <= set(cache_outputs)
    assert cache_outputs["compiler-cache-key"]["value"] == (
        "${{ steps.ccache.outputs.cache-matched-key }}"
    )
    assert cache_outputs["moc-cache-key"]["value"] == (
        "${{ steps.moccache.outputs.cache-matched-key }}"
    )
    assert summary["env"]["COMPILER_CACHE_KEY"] == "${{ steps.cache.outputs.compiler-cache-key }}"
    assert summary["env"]["MOC_CACHE_KEY"] == "${{ steps.cache.outputs.moc-cache-key }}"
    assert {"version", "installer", "source"} <= set(load_action("qt-install")["outputs"])
