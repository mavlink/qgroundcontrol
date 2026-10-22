"""Tests for generate_build_results_comment.py."""

from __future__ import annotations

import json
from datetime import datetime, timezone
from typing import TYPE_CHECKING

import pytest
from ci.reporting.generate_build_results_comment import generate_comment

if TYPE_CHECKING:
    from pathlib import Path

    from syrupy.assertion import SnapshotAssertion


def test_generate_comment_minimal(tmp_path: Path) -> None:
    env = {
        "BUILD_TABLE": "| Platform | Status | Details |\n|----------|--------|--------|\n| Linux | Passed | [View](https://example.test) |",
        "BUILD_SUMMARY": "All builds passed.",
        "PRECOMMIT_STATUS": "Not Triggered",
        "PRECOMMIT_URL": "",
        "TRIGGERED_BY": "Linux",
    }

    out = generate_comment(
        env, tmp_path, now_utc=datetime(2026, 2, 17, 12, 0, 0, tzinfo=timezone.utc)
    )

    assert "## Build Results" in out
    assert "| Linux | Passed | [View](https://example.test) |" in out
    assert "| Linux | Passed | [View](https://example.test) |\n\n**All builds passed.**" in out
    assert "| pre-commit | Not Triggered | - |" in out
    assert "<sub>Updated: 2026-02-17 12:00:00 UTC • Triggered by: Linux</sub>" in out


def test_generate_comment_precommit_artifact_overrides_status(tmp_path: Path) -> None:
    result_path = tmp_path / "artifacts" / "pre-commit-results" / "pre-commit-results.json"
    result_path.parent.mkdir(parents=True)
    result_path.write_text(
        json.dumps(
            {
                "exit_code": "1",
                "passed": "12",
                "failed": "3",
                "skipped": "1",
                "run_url": "https://example.test/precommit",
            }
        ),
        encoding="utf-8",
    )

    env = {
        "BUILD_TABLE": "| Platform | Status | Details |\n|----------|--------|--------|",
        "BUILD_SUMMARY": "Some builds failed.",
        "PRECOMMIT_STATUS": "Running",
        "PRECOMMIT_URL": "",
        "PRECOMMIT_RESULTS_PATH": "artifacts/pre-commit-results/pre-commit-results.json",
        "TRIGGERED_BY": "Android",
    }

    out = generate_comment(
        env, tmp_path, now_utc=datetime(2026, 2, 17, 12, 0, 0, tzinfo=timezone.utc)
    )

    assert "| pre-commit | Failed | [View](https://example.test/precommit) |" in out
    assert "Pre-commit hooks: 12 passed, 3 failed, 1 skipped." in out


def test_generate_comment_test_coverage_and_sizes(tmp_path: Path) -> None:
    # test-phase names CTest logs test-output-<suite>-<phase>.txt.
    test_file = (
        tmp_path / "artifacts" / "test-results-linux_gcc_64" / "test-output-linux_gcc_64-unit.txt"
    )
    test_file.parent.mkdir(parents=True)
    test_file.write_text(
        "\n".join(
            [
                "1/2 Test #1: AlphaTest ... Passed",
                "2/2 Test #2: BetaTest ... ***Failed",
            ]
        ),
        encoding="utf-8",
    )

    coverage_path = tmp_path / "artifacts" / "coverage-report" / "coverage-summary.json"
    coverage_path.parent.mkdir(parents=True)
    for path, covered in ((coverage_path, 75), (tmp_path / "baseline-coverage-summary.json", 70)):
        path.write_text(
            json.dumps(
                {
                    "line_total": 100,
                    "line_covered": covered,
                    "line_percent": float(covered),
                    "branch_total": 0,
                    "branch_covered": 0,
                    "branch_percent": 0.0,
                }
            ),
            encoding="utf-8",
        )

    pr_sizes = tmp_path / "pr-sizes.json"
    pr_sizes.write_text(
        json.dumps(
            {
                "artifacts": [
                    {"name": "QGroundControl.dmg", "size_bytes": 10485760, "size_human": "10.00 MB"}
                ]
            }
        ),
        encoding="utf-8",
    )
    baseline_sizes = tmp_path / "baseline-sizes.json"
    baseline_sizes.write_text(
        json.dumps(
            {
                "artifacts": [
                    {"name": "QGroundControl.dmg", "size_bytes": 9437184, "size_human": "9.00 MB"}
                ]
            }
        ),
        encoding="utf-8",
    )

    env = {
        "BUILD_TABLE": "| Platform | Status | Details |\n|----------|--------|--------|",
        "BUILD_SUMMARY": "Some builds failed.",
        "PRECOMMIT_STATUS": "Not Triggered",
        "PRECOMMIT_URL": "",
        "COVERAGE_SUMMARY": "artifacts/coverage-report/coverage-summary.json",
        "BASELINE_COVERAGE_SUMMARY": "baseline-coverage-summary.json",
        "PR_SIZES_JSON": "pr-sizes.json",
        "BASELINE_SIZES_JSON": "baseline-sizes.json",
        "TRIGGERED_BY": "MacOS",
    }

    out = generate_comment(
        env, tmp_path, now_utc=datetime(2026, 2, 17, 12, 0, 0, tzinfo=timezone.utc)
    )

    assert "### Test Results" in out
    assert "**linux_gcc_64**: 1 passed, 1 failed, 0 skipped" in out
    assert "| 75.0% | 70.0% | +5.0% |" in out
    assert "| QGroundControl.dmg | 10.00 MB | +1.00 MB (increase) |" in out
    assert "**Total size increased by 1.00 MB**" in out


def test_generate_comment_rejects_non_http_precommit_url(tmp_path: Path) -> None:
    env = {
        "BUILD_TABLE": "| Platform | Status | Details |\n|----------|--------|--------|",
        "BUILD_SUMMARY": "All builds passed.",
        "PRECOMMIT_STATUS": "Passed",
        "PRECOMMIT_URL": "javascript:alert(1)",
        "TRIGGERED_BY": "Linux",
    }

    out = generate_comment(
        env, tmp_path, now_utc=datetime(2026, 2, 17, 12, 0, 0, tzinfo=timezone.utc)
    )

    assert "| pre-commit | Passed | - |" in out


def test_generate_comment_sanitizes_precommit_artifact_url(tmp_path: Path) -> None:
    result_path = tmp_path / "artifacts" / "pre-commit-results" / "pre-commit-results.json"
    result_path.parent.mkdir(parents=True)
    result_path.write_text(
        json.dumps(
            {
                "exit_code": "0",
                "passed": "1",
                "failed": "0",
                "skipped": "0",
                "run_url": "https://example.test/precommit) [oops](https://bad.test)",
            }
        ),
        encoding="utf-8",
    )

    env = {
        "BUILD_TABLE": "| Platform | Status | Details |\n|----------|--------|--------|",
        "BUILD_SUMMARY": "All builds passed.",
        "PRECOMMIT_STATUS": "Running",
        "PRECOMMIT_URL": "",
        "PRECOMMIT_RESULTS_PATH": "artifacts/pre-commit-results/pre-commit-results.json",
        "TRIGGERED_BY": "Linux",
    }

    out = generate_comment(
        env, tmp_path, now_utc=datetime(2026, 2, 17, 12, 0, 0, tzinfo=timezone.utc)
    )

    assert "[View](https://example.test/precommit%29%20%5Boops%5D%28https://bad.test%29)" in out
    assert "[oops]" not in out


def test_generate_comment_handles_malformed_size_entries(tmp_path: Path) -> None:
    pr_sizes = tmp_path / "pr-sizes.json"
    pr_sizes.write_text(
        json.dumps(
            {
                "artifacts": [
                    {"name": "QGroundControl.dmg", "size_bytes": 1048576},
                    {"name": "", "size_bytes": 10},
                    {"size_human": "1 MB"},
                    "bad-entry",
                ]
            }
        ),
        encoding="utf-8",
    )

    env = {
        "BUILD_TABLE": "| Platform | Status | Details |\n|----------|--------|--------|",
        "BUILD_SUMMARY": "All builds passed.",
        "PRECOMMIT_STATUS": "Not Triggered",
        "PRECOMMIT_URL": "",
        "PR_SIZES_JSON": "pr-sizes.json",
        "TRIGGERED_BY": "Linux",
    }

    out = generate_comment(
        env, tmp_path, now_utc=datetime(2026, 2, 17, 12, 0, 0, tzinfo=timezone.utc)
    )

    assert "| QGroundControl.dmg | 1.00 MB |" in out


def test_android_abi_variants_are_not_reported_as_size_reductions(tmp_path: Path) -> None:
    mib = 1024 * 1024
    base_sha = "a" * 40
    (tmp_path / "pr-sizes.json").write_text(
        json.dumps(
            {
                "artifacts": [
                    {"name": "QGroundControl-linux-arm64-v8a", "size_bytes": 85 * mib},
                    {"name": "QGroundControl-mac-arm64-v8a", "size_bytes": 85 * mib},
                ]
            }
        )
    )
    (tmp_path / "baseline-sizes.json").write_text(
        json.dumps(
            {
                "head_sha": base_sha,
                "artifacts": [
                    {
                        "name": "QGroundControl-linux-arm64-v8a-armeabi-v7a",
                        "size_bytes": 162 * mib,
                    },
                    {"name": "QGroundControl-mac-arm64-v8a", "size_bytes": 87 * mib},
                ],
            }
        )
    )
    out = generate_comment({"BASELINE_SHA": base_sha}, tmp_path)
    assert "| QGroundControl-linux-arm64-v8a | 85.00 MB | N/A (no matching artifact) |" in out
    assert "| QGroundControl-mac-arm64-v8a | 85.00 MB | -2.00 MB (decrease) |" in out
    assert "Total size decreased by 2.00 MB" in out
    assert "77.00 MB" not in out
    assert "PR base `aaaaaaa`" in out
    assert "Δ from master" not in out


@pytest.mark.parametrize("cached_sha", ["older-master", None])
def test_size_delta_rejects_stale_or_unidentified_baseline(
    tmp_path: Path, cached_sha: str | None, caplog: pytest.LogCaptureFixture
) -> None:
    (tmp_path / "pr-sizes.json").write_text(
        json.dumps({"artifacts": [{"name": "QGroundControl", "size_bytes": 1024 * 1024}]})
    )
    baseline: dict[str, object] = {
        "artifacts": [{"name": "QGroundControl", "size_bytes": 78 * 1024 * 1024}]
    }
    if cached_sha is not None:
        baseline["head_sha"] = cached_sha
    (tmp_path / "baseline-sizes.json").write_text(json.dumps(baseline))
    out = generate_comment({"BASELINE_SHA": "a" * 40}, tmp_path)
    assert "Ignoring size baseline" in caplog.text
    assert "| QGroundControl | 1.00 MB | N/A (no matching artifact) |" in out
    assert "No comparable artifact baseline available for PR base `aaaaaaa`" in out
    assert "Total size decreased" not in out


def test_empty_size_snapshot_does_not_render_stale_artifacts(tmp_path: Path) -> None:
    (tmp_path / "pr-sizes.json").write_text(json.dumps({"head_sha": "new", "artifacts": []}))
    out = generate_comment({}, tmp_path)
    assert "### Artifact Sizes" not in out


def _write_full_report_inputs(base: Path) -> dict[str, str]:
    files = {
        "artifacts/test-results-linux_gcc_64/test-output-linux_gcc_64-unit.txt": (
            "1/3 Test #1: AlphaTest ... Passed\n"
            "2/3 Test #2: BetaTest ... ***Failed\n"
            "3/3 Test #3: GammaTest ... ***Skipped\n"
        ),
        "artifacts/pre-commit-results/pre-commit-results.json": json.dumps(
            {"exit_code": "1", "passed": "40", "failed": "2", "skipped": "3"}
            | {"run_url": "https://example.test/precommit"}
        ),
        "coverage-summary.json": json.dumps(
            {"line_total": 200, "line_covered": 150, "line_percent": 75.0}
            | {"branch_total": 80, "branch_covered": 40, "branch_percent": 50.0}
        ),
        "baseline-coverage-summary.json": json.dumps(
            {"line_total": 200, "line_covered": 140, "line_percent": 70.0}
            | {"branch_total": 80, "branch_covered": 44, "branch_percent": 55.0}
        ),
        "pr-sizes.json": json.dumps(
            {
                "head_sha": "b" * 40,
                "artifacts": [{"name": "QGroundControl.dmg", "size_bytes": 10 * 2**20}],
            }
        ),
        "baseline-sizes.json": json.dumps(
            {
                "head_sha": "a" * 40,
                "artifacts": [{"name": "QGroundControl.dmg", "size_bytes": 9 * 2**20}],
            }
        ),
    }
    for name, content in files.items():
        (base / name).parent.mkdir(parents=True, exist_ok=True)
        (base / name).write_text(content, encoding="utf-8")
    return {
        "BUILD_TABLE": "| Platform | Status | Details |\n|----------|--------|--------|\n"
        "| Linux | Failed | [View](https://example.test/linux) |\n"
        "| MacOS | Passed | [View](https://example.test/macos) |",
        "BUILD_SUMMARY": "Some builds failed.",
        "PRECOMMIT_STATUS": "Running",
        "PRECOMMIT_RESULTS_PATH": "artifacts/pre-commit-results/pre-commit-results.json",
        "COVERAGE_SUMMARY": "coverage-summary.json",
        "BASELINE_COVERAGE_SUMMARY": "baseline-coverage-summary.json",
        "PR_SIZES_JSON": "pr-sizes.json",
        "BASELINE_SIZES_JSON": "baseline-sizes.json",
        "BASELINE_SHA": "a" * 40,
        "TRIGGERED_BY": "MacOS",
    }


@pytest.mark.parametrize("scenario", ["all-passed", "full-report"])
def test_rendered_comment_matches_snapshot(
    tmp_path: Path, scenario: str, snapshot: SnapshotAssertion
) -> None:
    """Review layout changes in the .ambr diff; refresh with --snapshot-update."""
    if scenario == "full-report":
        env = _write_full_report_inputs(tmp_path)
    else:
        env = {
            "BUILD_TABLE": "| Platform | Status | Details |\n|----------|--------|--------|\n"
            "| Linux | Passed | [View](https://example.test/linux) |",
            "BUILD_SUMMARY": "All builds passed.",
            "PRECOMMIT_STATUS": "Passed",
            "PRECOMMIT_URL": "https://example.test/precommit",
            "TRIGGERED_BY": "Linux",
        }
    now = datetime(2026, 2, 17, 12, 0, 0, tzinfo=timezone.utc)
    assert generate_comment(env, tmp_path, now_utc=now) == snapshot
