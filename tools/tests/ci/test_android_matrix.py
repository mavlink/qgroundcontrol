"""Tests for android_matrix.py."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

import pytest
from ci.planning import detect_changes
from ci.planning.android_matrix import (
    LINUX_EMULATOR_JOB,
    LINUX_JOB,
    MAC_JOB,
    WINDOWS_JOB,
    build_matrix,
    main,
)
from qgc_tools import release_tag_check

from .._helpers import load_workflow

if TYPE_CHECKING:
    from pathlib import Path


@pytest.mark.parametrize(
    ("is_pr", "expected"),
    [
        (
            True,
            [
                (LINUX_JOB, "arm64-v8a", "arm64-v8a"),
                (MAC_JOB, "arm64-v8a", "arm64-v8a"),
                (LINUX_EMULATOR_JOB, "x86_64", "x86_64"),
            ],
        ),
        (
            False,
            [
                (LINUX_JOB, "arm64-v8a;armeabi-v7a", "arm64-v8a-armeabi-v7a"),
                (MAC_JOB, "arm64-v8a", "arm64-v8a"),
                (WINDOWS_JOB, "arm64-v8a", "arm64-v8a"),
                (LINUX_EMULATOR_JOB, "x86_64", "x86_64"),
            ],
        ),
    ],
)
def test_build_matrix_preserves_hosts_and_shipped_abis(is_pr, expected) -> None:
    assert build_matrix(is_pr) == [
        {**leg, "android_abis": abis, "artifact_abi_suffix": suffix}
        for leg, abis, suffix in expected
    ]


def test_matrix_abi_identity_is_canonical_and_does_not_mutate_templates() -> None:
    release = build_matrix(is_pr=False)
    build_matrix(is_pr=True)
    assert build_matrix(is_pr=False) == release
    for leg in release:
        abis = leg["android_abis"]
        assert isinstance(abis, str)
        assert leg["artifact_abi_suffix"] == "-".join(sorted(abis.split(";")))
    for template in (LINUX_JOB, MAC_JOB, WINDOWS_JOB, LINUX_EMULATOR_JOB):
        assert "android_abis" not in template
        assert "artifact_abi_suffix" not in template


def test_workflow_upload_identity_tracks_configured_abis_without_renaming_payloads() -> None:
    workflow = load_workflow("android.yml")
    build = workflow["jobs"]["build"]
    env = build["env"]
    steps = {step["name"]: step for step in build["steps"]}
    upload = steps["Attest and Upload"]
    assert env["QT_ANDROID_ABIS"] == "${{ matrix.android_abis }}"
    assert steps["Build Setup (Android)"]["with"]["abis"] == "${{ env.QT_ANDROID_ABIS }}"
    assert (
        "-DQT_ANDROID_ABIS=${{ env.QT_ANDROID_ABIS }}" in steps["Configure"]["with"]["extra-args"]
    )
    assert upload["with"]["package-name"] == (
        "${{ format('{0}-{1}-{2}', env.PACKAGE, matrix.host, matrix.artifact_abi_suffix) }}"
    )
    assert upload["with"]["artifact-name"] == "${{ env.PACKAGE }}.apk"
    assert upload["with"]["subject-name"] == "${{ env.PACKAGE }}-${{ matrix.host }}"
    assert upload["with"]["upload-aws"] == "${{ matrix.primary }}"
    assert upload["if"] == "${{ !matrix.emulator }}"
    assert steps["Deploy to Play Store"]["with"]["releaseFiles"] == (
        "${{ runner.temp }}/build/${{ env.PACKAGE }}.apk"
    )
    matrix_steps = workflow["jobs"]["build-matrix"]["steps"]
    generate = next(step for step in matrix_steps if step.get("id") == "build")
    assert generate["env"]["EVENT_NAME"] == "${{ github.event_name }}"
    assert generate["run"] == "python3 tools/ci/planning/android_matrix.py"
    assert workflow["jobs"]["build-matrix"]["outputs"]["has_jobs"] == (
        "${{ steps.build.outputs.has_jobs }}"
    )
    assert build["if"] == "needs.build-matrix.outputs.has_jobs == 'true'"
    assert build["strategy"]["matrix"]["include"] == (
        "${{ fromJSON(needs.build-matrix.outputs.include) }}"
    )

    names = [
        {
            leg["host"]: f"{env['PACKAGE']}-{leg['host']}-{leg['artifact_abi_suffix']}"
            for leg in build_matrix(is_pr)
            if not leg["emulator"]
        }
        for is_pr in (True, False)
    ]
    pr, push = names
    assert pr["linux"] == "QGroundControl-linux-arm64-v8a"
    assert push["linux"] == "QGroundControl-linux-arm64-v8a-armeabi-v7a"
    assert pr["linux"] != push["linux"]
    assert pr["mac"] == push["mac"] == "QGroundControl-mac-arm64-v8a"


def test_legs_carry_runner_fields() -> None:
    for leg in build_matrix(is_pr=False):
        assert "runson_runner" in leg
        assert leg["fallback_runner"]


@pytest.fixture
def event(monkeypatch: pytest.MonkeyPatch, gh_output: Path) -> pytest.MonkeyPatch:
    monkeypatch.setenv("GITHUB_REF_TYPE", "branch")
    monkeypatch.setattr(detect_changes, "get_changed_files", lambda: ["src/Vehicle.cc"])
    return monkeypatch


def _outputs(gh_output: Path) -> dict[str, str]:
    return dict(line.split("=", 1) for line in gh_output.read_text().splitlines())


def test_main_writes_github_output_for_pr(
    event: pytest.MonkeyPatch, gh_output: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    event.setenv("EVENT_NAME", "pull_request")

    assert main([]) == 0

    outputs = _outputs(gh_output)
    assert json.loads(outputs["include"]) == build_matrix(is_pr=True)
    assert outputs["has_jobs"] == "true"
    assert "include=" in capsys.readouterr().out


@pytest.mark.parametrize("event_name", ["push", "merge_group", "workflow_dispatch"])
def test_main_non_pr_includes_windows(
    event: pytest.MonkeyPatch, gh_output: Path, event_name: str
) -> None:
    event.setenv("EVENT_NAME", event_name)

    assert main([]) == 0

    assert json.loads(_outputs(gh_output)["include"]) == build_matrix(is_pr=False)


def test_main_skips_pull_requests_without_android_changes(
    event: pytest.MonkeyPatch, gh_output: Path
) -> None:
    event.setenv("EVENT_NAME", "pull_request")
    event.setattr(detect_changes, "get_changed_files", lambda: ["docs/index.md"])

    assert main([]) == 0

    assert _outputs(gh_output) == {"include": "[]", "has_jobs": "false"}


@pytest.mark.parametrize(("release_tag", "has_jobs"), [(True, "true"), (False, "false")])
def test_main_builds_tags_only_for_stable_releases(
    event: pytest.MonkeyPatch, gh_output: Path, release_tag: bool, has_jobs: str
) -> None:
    event.setenv("EVENT_NAME", "push")
    event.setenv("GITHUB_REF_TYPE", "tag")
    event.setattr(detect_changes, "check_release_tag", lambda *_: release_tag)

    assert main([]) == 0

    assert _outputs(gh_output)["has_jobs"] == has_jobs


def test_main_fails_when_tag_branches_are_unknown(
    event: pytest.MonkeyPatch, gh_output: Path
) -> None:
    def fail(*_: str) -> bool:
        raise release_tag_check.GitQueryError("no history")

    event.setenv("EVENT_NAME", "push")
    event.setenv("GITHUB_REF_TYPE", "tag")
    event.setattr(detect_changes, "check_release_tag", fail)

    assert main([]) == 1
    assert gh_output.read_text() == ""


def test_main_output_uses_compact_json_separators(
    event: pytest.MonkeyPatch, gh_output: Path
) -> None:
    """Compact JSON in $GITHUB_OUTPUT keeps log lines short and avoids ambiguity."""
    event.setenv("EVENT_NAME", "push")
    main([])
    serialized = _outputs(gh_output)["include"]
    assert ", " not in serialized
    assert ": " not in serialized
