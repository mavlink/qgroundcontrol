"""Tests for android_matrix.py."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

import pytest
import yaml
from _helpers import REPO_ROOT
from android_matrix import (
    LINUX_EMULATOR_JOB,
    LINUX_JOB,
    MAC_JOB,
    WINDOWS_JOB,
    build_matrix,
    main,
)

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
    workflow = yaml.safe_load((REPO_ROOT / ".github/workflows/android.yml").read_text())
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
    assert steps["Deploy to Play Store"]["with"]["artifact"] == (
        "${{ runner.temp }}/build/${{ env.PACKAGE }}.apk"
    )
    matrix_steps = workflow["jobs"]["build-matrix"]["steps"]
    generate = next(step for step in matrix_steps if step.get("id") == "build")
    assert generate["env"]["IS_PR"] == "${{ github.event_name == 'pull_request' && '1' || '0' }}"
    assert generate["run"] == "python3 .github/scripts/android_matrix.py"
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


def test_main_writes_github_output_for_pr(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    output_file = tmp_path / "gha_output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    monkeypatch.setenv("IS_PR", "1")

    assert main([]) == 0

    content = output_file.read_text()
    assert content.startswith("include=")
    payload = json.loads(content.split("=", 1)[1])
    assert payload == build_matrix(is_pr=True)
    assert "include=" in capsys.readouterr().out


def test_main_non_pr_includes_windows(tmp_path, monkeypatch: pytest.MonkeyPatch) -> None:
    output_file = tmp_path / "gha_output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    monkeypatch.setenv("IS_PR", "0")

    assert main([]) == 0

    payload = json.loads(output_file.read_text().split("=", 1)[1])
    assert payload == build_matrix(is_pr=False)


def test_main_output_uses_compact_json_separators(
    tmp_path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """Compact JSON in $GITHUB_OUTPUT keeps log lines short and avoids ambiguity."""
    output_file = tmp_path / "gha_output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    monkeypatch.setenv("IS_PR", "0")
    main([])
    serialized = output_file.read_text().split("=", 1)[1].rstrip("\n")
    assert ", " not in serialized
    assert ": " not in serialized
