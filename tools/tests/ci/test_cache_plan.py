"""Tests for cache_plan.py."""

from __future__ import annotations

import json
from dataclasses import replace
from typing import TYPE_CHECKING

import pytest
from ci.cache.build_cache_artifact import artifact_name
from ci.cache.cache_plan import (
    CacheInputs,
    compiler_cache_tool,
    determine_cache_scope,
    main,
    plan_caches,
    read_max_size,
    save_policy,
    scoped_save_policy,
)
from ci.cache.cpm_helper import compute_cpm_fingerprint, compute_gstreamer_fingerprint

if TYPE_CHECKING:
    from pathlib import Path

    from ci.cache.cache_plan import CachePlan

CCACHE_HASH = "c" * 64
MOC_HASH = "m" * 64
GST_FILES_HASH = "g" * 64
BASE = CacheInputs(
    host="linux",
    target="linux_gcc_64",
    build_type="Release",
    cpm_modules=".cache/CPM",
    pr_save_min_misses="20",
    event="push",
    ref_name="master",
    run_id="7",
    run_attempt="1",
    ccache_hash=CCACHE_HASH,
    moccache_hash=MOC_HASH,
    gst_files_hash=GST_FILES_HASH,
)


@pytest.fixture
def workspace(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    root = tmp_path / "workspace"
    (root / "tools/configs").mkdir(parents=True)
    (root / "tools/configs/ccache.conf").write_text("max_size = 5G\n")
    (root / ".github").mkdir()
    config = {"qt": {"version": "6.10"}, "gstreamer": {"version": {"windows": "1.28.4"}}}
    (root / ".github/build-config.json").write_text(json.dumps(config))
    (root / "CMakeLists.txt").write_text("CPMAddPackage(NAME dependency)\n")
    monkeypatch.setenv("GITHUB_WORKSPACE", str(root))
    monkeypatch.setenv("RUNNER_ARCH", "X64")
    return root


def plan(workspace: Path, **changes: str) -> CachePlan:
    return plan_caches(replace(BASE, **changes), workspace)


def restore_keys(value: str) -> list[str]:
    return value.split("\n")


def test_pull_request_keys_match_the_previous_action(workspace) -> None:
    result = plan(workspace, event="pull_request", ref_name="42/merge", pr_number="42")
    fingerprint = compute_cpm_fingerprint(workspace.resolve())
    ccache = "ccache-linux-linux_gcc_64-Release"
    moc = "moccache-linux-linux_gcc_64"
    cpm = "cpm-sources-v2-linux-linux_gcc_64"

    assert result.outputs == {
        "ccache_key": f"{ccache}-pr-42-{CCACHE_HASH}-7-1",
        "ccache_restore_keys": "\n".join(
            [
                f"{ccache}-pr-42-{CCACHE_HASH}-",
                f"{ccache}-pr-42-",
                f"{ccache}-pr-42-",
                f"{ccache}-shared-",
                f"{ccache}-shared-",
            ]
        ),
        "moccache_key": f"{moc}-pr-42-{MOC_HASH}-Release-7-1",
        "moccache_restore_keys": "\n".join(
            [f"{moc}-pr-42-{MOC_HASH}-", f"{moc}-pr-42-", f"{moc}-shared-"]
        ),
        "gst_mode": "",
        "cpm_mode": "restore",
        "cpm_shared_probe": "true",
        "cpm_key": f"{cpm}-pr-42-{fingerprint}",
        "cpm_shared_key": f"{cpm}-shared-{fingerprint}",
        "cpm_restore_keys": "\n".join(
            [f"{cpm}-pr-42-", f"{cpm}-shared-", f"{cpm}-pr-42-", f"{cpm}-shared-"]
        ),
    }
    cpm_cache = (workspace / ".cache/CPM").resolve()
    assert result.env == {
        "CCACHE_DIR": f"{workspace.as_posix()}/.ccache",
        "CCACHE_BASEDIR": workspace.as_posix(),
        "CCACHE_CONFIGPATH": f"{workspace.as_posix()}/tools/configs/ccache.conf",
        "CCACHE_MAXSIZE": "1G",
        "CCACHE_NOFILECLONE": "1",
        "CPM_SOURCE_CACHE": cpm_cache.as_posix(),
        "CPM_CACHE_PATH": ".cache/CPM",
        "QGC_BUILD_CACHE_ARTIFACT": "true",
        "QGC_BUILD_CACHE_ARTIFACT_NAME": artifact_name(f"{ccache}-pr-42", f"{moc}-pr-42-Release"),
        "QGC_BUILD_CACHE_SHARED_ARTIFACT": artifact_name(
            f"{ccache}-shared", f"{moc}-shared-Release"
        ),
        "MOCCACHE_STATS": "1",
        "QGC_SAVE_BUILD_CACHE": "false",
        "QGC_CCACHE_KEY": f"{ccache}-pr-42-{CCACHE_HASH}-7-1",
        "QGC_MOCCACHE_KEY": f"{moc}-pr-42-{MOC_HASH}-Release-7-1",
        "QGC_PR_CACHE_SAVE_MIN_MISSES": "20",
        "QGC_COMPILER_CACHE_TOOL": "ccache",
    }
    assert (workspace / ".ccache").is_dir()
    assert cpm_cache.is_dir()


def test_variant_and_suffix_keys_match_the_previous_action(workspace) -> None:
    result = plan(workspace, build_type="Debug", variant="custom", key_suffix="custom-debug")
    fingerprint = compute_cpm_fingerprint(workspace.resolve())
    ccache = "ccache-linux-linux_gcc_64-Debug-custom"
    moc = "moccache-linux-linux_gcc_64-shared"
    cpm = "cpm-sources-v2-linux-linux_gcc_64"

    assert result.outputs["ccache_key"] == f"{ccache}-custom-debug-shared-{CCACHE_HASH}-7-1"
    assert restore_keys(result.outputs["ccache_restore_keys"]) == [
        f"{ccache}-custom-debug-shared-{CCACHE_HASH}-",
        f"{ccache}-custom-debug-shared-",
        f"{ccache}-shared-",
        f"{ccache}-custom-debug-shared-",
        f"{ccache}-shared-",
        "ccache-linux-linux_gcc_64-Debug-shared-",
    ]
    assert result.outputs["moccache_key"] == f"{moc}-{MOC_HASH}-Debug-custom-custom-debug-7-1"
    assert result.outputs["cpm_key"] == f"{cpm}-custom-debug-shared-{fingerprint}"
    assert restore_keys(result.outputs["cpm_restore_keys"]) == [
        f"{cpm}-custom-debug-shared-",
        f"{cpm}-custom-debug-shared-",
        f"{cpm}-shared-",
        f"{cpm}-shared-",
    ]
    assert result.env["CCACHE_MAXSIZE"] == "2G"
    own = artifact_name(f"{ccache}-custom-debug-shared", f"{moc}-Debug-custom-custom-debug")
    assert result.env["QGC_BUILD_CACHE_ARTIFACT_NAME"] == own
    assert result.env["QGC_BUILD_CACHE_SHARED_ARTIFACT"] == own


def test_windows_release_on_s3_saves_sdk_and_scoped_snapshots(workspace) -> None:
    result = plan(
        workspace,
        host="windows",
        target="win64_msvc2022_64",
        event="push",
        ref_name="feature/x",
        backend="s3",
    )
    gst = compute_gstreamer_fingerprint(workspace.resolve())

    assert result.outputs["moccache_key"].startswith(
        "moccache-windows-win64_msvc2022_64-branch-feature-x-"
    )
    assert result.outputs["gst_mode"] == "save"
    assert result.outputs["gst_key"] == (
        f"gst-sdk-v1-windows-win64_msvc2022_64-{gst}-{GST_FILES_HASH}"
    )
    assert result.outputs["cpm_mode"] == "save"
    assert result.outputs["cpm_shared_probe"] == "true"
    assert result.env["CCACHE_MAXSIZE"] == "5G"
    assert result.env["QGC_BUILD_CACHE_ARTIFACT"] == "false"
    assert result.env["QGC_SAVE_BUILD_CACHE"] == "true"


@pytest.mark.parametrize(
    ("requested", "event", "save"),
    [
        ("true", "pull_request_target", "true"),
        ("false", "push", "false"),
        ("auto", "push", "true"),
        ("auto", "pull_request", "true"),
        ("auto", "schedule", "true"),
        ("auto", "pull_request_target", "false"),
    ],
)
def test_save_policy(requested, event, save) -> None:
    assert save_policy(requested, event) == save


@pytest.mark.parametrize(
    ("event", "backend", "requested", "scoped_save"),
    [
        ("pull_request", "github", "auto", "false"),
        ("pull_request", "", "auto", "false"),
        ("pull_request", "s3", "auto", "true"),
        ("push", "github", "auto", "true"),
        ("pull_request", "github", "true", "true"),
        ("pull_request_target", "s3", "auto", "false"),
    ],
)
def test_scoped_caches_are_restore_only_for_prs_on_github_storage(
    event, backend, requested, scoped_save
) -> None:
    save = save_policy(requested, event)
    assert scoped_save_policy(requested, save, event, backend) == scoped_save


@pytest.mark.parametrize(
    ("changes", "gst_mode", "cpm_mode", "probe"),
    [
        ({}, "", "save", "false"),
        ({"build_type": "Debug"}, "", "restore", "false"),
        ({"event": "pull_request", "pr_number": "1"}, "", "restore", "true"),
        ({"event": "pull_request", "pr_number": "1", "backend": "s3"}, "", "save", "true"),
        ({"event": "pull_request_target"}, "", "restore", "false"),
        ({"save_cache": "false", "ref_name": "feature"}, "", "restore", "false"),
        ({"target": "android"}, "save", "save", "false"),
        ({"target": "ios", "build_type": "Debug"}, "restore", "restore", "false"),
        ({"target": "win64_msvc2022_arm64", "event": "pull_request"}, "save", "restore", "true"),
        (
            {"target": "Android", "build_type": "release", "save_cache": "True"},
            "save",
            "save",
            "false",
        ),
        ({"target": "android", "cpm_modules": ""}, "", "", "false"),
    ],
)
def test_dependency_cache_modes(workspace, changes, gst_mode, cpm_mode, probe) -> None:
    outputs = plan(workspace, **changes).outputs
    assert (outputs["gst_mode"], outputs["cpm_mode"], outputs["cpm_shared_probe"]) == (
        gst_mode,
        cpm_mode,
        probe,
    )
    assert ("gst_key" in outputs) == bool(gst_mode)
    assert ("cpm_key" in outputs) == bool(cpm_mode)


def test_without_cpm_modules_no_cpm_environment_is_exported(workspace) -> None:
    result = plan(workspace, cpm_modules="")
    assert "CPM_SOURCE_CACHE" not in result.env
    assert not (workspace / ".cache/CPM").exists()


@pytest.mark.parametrize(
    ("backend", "event", "host", "build_type", "variant", "expected"),
    [
        ("", "push", "linux", "Release", "", "1G"),
        ("github", "pull_request", "linux", "Debug", "coverage", "3G"),
        ("github", "pull_request", "linux", "Debug", "sanitizers", "3G"),
        ("github", "pull_request", "windows", "Release", "", "3G"),
        ("github", "push", "mac", "Release", "", "2G"),
        ("s3", "push", "linux", "Release", "", "5G"),
        ("s3", "pull_request", "windows", "Release", "", "5G"),
    ],
)
def test_compiler_limit_follows_resolved_backend(
    workspace, backend, event, host, build_type, variant, expected
) -> None:
    env = plan(
        workspace, backend=backend, event=event, host=host, build_type=build_type, variant=variant
    ).env
    assert env["CCACHE_MAXSIZE"] == expected


@pytest.mark.parametrize(
    ("content", "expected"),
    [
        ("max_size = 5G\n", "5G"),
        ("  max_size=10G  \ncompiler_check = content\n", "10G"),
        ("# max_size = 9G\ncompression_level = 5\n", "2G"),
        ("max_size =\n", "2G"),
    ],
)
def test_read_max_size(tmp_path, content, expected) -> None:
    config = tmp_path / "ccache.conf"
    config.write_text(content)
    assert read_max_size(config) == expected


def test_missing_ccache_conf_uses_default_max_size(tmp_path, capsys) -> None:
    assert read_max_size(tmp_path / "missing.conf") == "2G"
    assert "using default max_size" in capsys.readouterr().err


@pytest.mark.parametrize(
    ("event", "ref", "pr", "scope"),
    [
        ("pull_request", "feature/test", "42", "pr-42"),
        ("pull_request", "feature/test", "", "pr-unknown"),
        ("push", "feature/test", "", "branch-feature-test"),
        ("push", "master", "", "shared"),
        ("schedule", "master", "", "shared"),
        ("workflow_dispatch", "master", "", "shared"),
        ("workflow_dispatch", "feature/test", "", "manual-feature-test"),
        ("pull_request_target", "master", "42", "pull_request_target-master"),
    ],
)
def test_cache_scope(event, ref, pr, scope) -> None:
    # Only default-branch pushes, schedules and dispatches populate the shared baseline.
    assert determine_cache_scope(event, ref, pr) == scope


@pytest.mark.parametrize(
    ("value", "exported"), [("20", "20"), ("0", "0"), ("", "0"), ("abc", "0"), ("7\nEVIL=1", "0")]
)
def test_pr_save_threshold_is_sanitized(workspace, value, exported) -> None:
    env = plan(workspace, pr_save_min_misses=value).env
    assert env["QGC_PR_CACHE_SAVE_MIN_MISSES"] == exported


@pytest.mark.parametrize(
    ("backend", "event", "enabled"),
    [
        ("github", "pull_request", "true"),
        ("github", "workflow_dispatch", "true"),
        ("github", "merge_group", "false"),
        ("github", "pull_request_target", "false"),
        ("s3", "push", "false"),
    ],
)
def test_artifact_snapshots_follow_backend_and_event(workspace, backend, event, enabled) -> None:
    env = plan(workspace, backend=backend, event=event).env
    assert env["QGC_BUILD_CACHE_ARTIFACT"] == enabled


SCCACHE = {"compiler_cache": "sccache", "sccache_bucket": "runs-on-cache"}


def test_sccache_pilot_replaces_the_ccache_archive(workspace, tmp_path) -> None:
    result = plan(workspace, backend="github", runner_temp=str(tmp_path / "temp"), **SCCACHE)
    assert not (workspace / ".ccache").exists()
    ccache = plan(workspace, backend="github")

    assert result.outputs["ccache_key"] == result.outputs["ccache_restore_keys"] == ""
    for name in ("moccache_key", "moccache_restore_keys", "cpm_mode", "cpm_key"):
        assert result.outputs[name] == ccache.outputs[name]
    assert not [name for name in result.env if name.startswith("CCACHE_")]
    assert {
        "QGC_COMPILER_CACHE_TOOL": "sccache",
        "QGC_CCACHE_KEY": "",
        "QGC_MOCCACHE_KEY": ccache.env["QGC_MOCCACHE_KEY"],
        "QGC_SAVE_BUILD_CACHE": "true",
        # The workflow artifact bundles a ccache directory; moc output uses the dependency cache.
        "QGC_BUILD_CACHE_ARTIFACT": "false",
        "SCCACHE_BASEDIRS": workspace.as_posix(),
        "SCCACHE_IDLE_TIMEOUT": "0",
        "SCCACHE_ERROR_LOG": (tmp_path / "temp/sccache.log").as_posix(),
        "SCCACHE_LOG": "warn",
        "SCCACHE_S3_RW_MODE": "READ_WRITE",
    }.items() <= result.env.items()
    assert ccache.env["QGC_COMPILER_CACHE_TOOL"] == "ccache"


@pytest.mark.parametrize(
    ("changes", "mode"),
    [
        ({}, "READ_WRITE"),
        ({"event": "workflow_dispatch", "ref_name": "feature"}, "READ_WRITE"),
        ({"event": "merge_group", "ref_name": "gh-readonly-queue/master/pr-1"}, "READ_WRITE"),
        ({"event": "pull_request", "pr_number": "1", "backend": "s3"}, "READ_ONLY"),
        ({"event": "pull_request", "pr_number": "1", "save_cache": "true"}, "READ_ONLY"),
        ({"event": "pull_request_target", "save_cache": "true"}, "READ_ONLY"),
        ({"save_cache": "false"}, "READ_ONLY"),
    ],
)
def test_only_trusted_events_write_the_shared_sccache_prefix(workspace, changes, mode) -> None:
    assert plan(workspace, **SCCACHE, **changes).env["SCCACHE_S3_RW_MODE"] == mode


@pytest.mark.parametrize(
    ("requested", "bucket", "tool", "warned"),
    [
        ("sccache", "bucket", "sccache", False),
        (" SCCache ", "bucket", "sccache", False),
        ("sccache", "", "ccache", True),
        ("", "bucket", "ccache", False),
        ("ccache", "bucket", "ccache", False),
        ("distcc", "bucket", "ccache", False),
    ],
)
def test_compiler_cache_tool(requested, bucket, tool, warned, capsys) -> None:
    assert compiler_cache_tool(requested, bucket) == tool
    assert ("::warning::" in capsys.readouterr().out) is warned


def test_sccache_without_its_bucket_keeps_the_ccache_plan(workspace) -> None:
    fallback = plan(workspace, compiler_cache="sccache")
    assert fallback == plan(workspace)
    assert fallback.env["QGC_COMPILER_CACHE_TOOL"] == "ccache"


def test_multiline_environment_values_are_rejected(workspace) -> None:
    with pytest.raises(ValueError, match="single line"):
        plan(workspace, key_suffix="leg\nQGC_SAVE_BUILD_CACHE=true")


def test_main_exports_outputs_and_environment(workspace, monkeypatch, gh_output) -> None:
    github_env = workspace.parent / "github_env"
    for name, value in {
        "GITHUB_ENV": str(github_env),
        "INPUT_HOST": "mac",
        "INPUT_TARGET": "ios",
        "INPUT_BUILD_TYPE": "Debug",
        "INPUT_KEY_SUFFIX": "simulator",
        "INPUT_CPM_MODULES": ".cache/CPM",
        "INPUT_SAVE_CACHE": "auto",
        "INPUT_PR_SAVE_MIN_MISSES": "20",
        "PR_NUMBER": "9",
        "GITHUB_EVENT_NAME": "pull_request",
        "GITHUB_REF_NAME": "9/merge",
        "GITHUB_RUN_ID": "11",
        "GITHUB_RUN_ATTEMPT": "3",
        "QGC_ACTIONS_CACHE_BACKEND": "s3",
        "COMPILER_CACHE_HASH": CCACHE_HASH,
        "MOC_CACHE_HASH": MOC_HASH,
        "GST_SDK_FILES_HASH": GST_FILES_HASH,
    }.items():
        monkeypatch.setenv(name, value)

    assert main() == 0

    output = gh_output.read_text()
    ccache = "ccache-mac-ios-Debug"
    assert f"ccache_key={ccache}-simulator-pr-9-{CCACHE_HASH}-11-3\n" in output
    assert "ccache_restore_keys<<" in output
    assert f"\n{ccache}-simulator-shared-\n{ccache}-shared-\n" in output
    assert "gst_mode=restore\n" in output
    assert "cpm_mode=restore\n" in output
    exported = dict(line.split("=", 1) for line in github_env.read_text().splitlines())
    assert exported["QGC_SAVE_BUILD_CACHE"] == "true"
    assert exported["QGC_CCACHE_KEY"] == f"{ccache}-simulator-pr-9-{CCACHE_HASH}-11-3"
    assert exported["CPM_CACHE_PATH"] == ".cache/CPM"
    assert exported["MOCCACHE_STATS"] == "1"


def test_main_selects_sccache_from_the_job_environment(workspace, monkeypatch, gh_output) -> None:
    github_env = workspace.parent / "github_env"
    for name, value in {
        "GITHUB_ENV": str(github_env),
        "INPUT_HOST": "linux",
        "INPUT_TARGET": "linux_gcc_64",
        "INPUT_BUILD_TYPE": "Release",
        "GITHUB_EVENT_NAME": "pull_request",
        "GITHUB_REF_NAME": "9/merge",
        "PR_NUMBER": "9",
        "QGC_ACTIONS_CACHE_BACKEND": "s3",
        "QGC_COMPILER_CACHE": "sccache",
        "SCCACHE_BUCKET": "runs-on-cache",
        "RUNNER_TEMP": str(workspace.parent / "temp"),
    }.items():
        monkeypatch.setenv(name, value)

    assert main() == 0

    assert "ccache_key=\n" in gh_output.read_text()
    exported = dict(line.split("=", 1) for line in github_env.read_text().splitlines())
    assert exported["QGC_COMPILER_CACHE_TOOL"] == "sccache"
    assert exported["QGC_CCACHE_KEY"] == ""
    assert exported["SCCACHE_S3_RW_MODE"] == "READ_ONLY"
    assert exported["SCCACHE_ERROR_LOG"] == (workspace.parent / "temp/sccache.log").as_posix()
