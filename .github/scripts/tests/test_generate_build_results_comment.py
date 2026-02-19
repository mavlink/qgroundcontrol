"""Tests for generate_build_results_comment.py."""

from __future__ import annotations

import json
from datetime import UTC, datetime
from pathlib import Path

from generate_build_results_comment import generate_comment


def test_generate_comment_minimal(tmp_path: Path) -> None:
    env = {
        "BUILD_TABLE": "| Platform | Status | Details |\n|----------|--------|--------|\n| Linux | Passed | [View](https://example.test) |",
        "BUILD_SUMMARY": "All builds passed.",
        "PRECOMMIT_STATUS": "Not Triggered",
        "PRECOMMIT_URL": "",
        "TRIGGERED_BY": "Linux",
    }

    out = generate_comment(env, tmp_path, now_utc=datetime(2026, 2, 17, 12, 0, 0, tzinfo=UTC))

    assert "## Build Results" in out
    assert "| Linux | Passed | [View](https://example.test) |" in out
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

    out = generate_comment(env, tmp_path, now_utc=datetime(2026, 2, 17, 12, 0, 0, tzinfo=UTC))

    assert "| pre-commit | Failed (non-blocking) | [View](https://example.test/precommit) |" in out
    assert "Pre-commit hooks: 12 passed, 3 failed, 1 skipped." in out


def test_generate_comment_test_coverage_and_sizes(tmp_path: Path) -> None:
    test_file = tmp_path / "artifacts" / "test-results-linux_gcc_64" / "test-output.txt"
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

    coverage_path = tmp_path / "artifacts" / "coverage-report" / "coverage.xml"
    coverage_path.parent.mkdir(parents=True)
    coverage_path.write_text('<coverage line-rate="0.75" />', encoding="utf-8")

    baseline_coverage = tmp_path / "baseline-coverage.xml"
    baseline_coverage.write_text('<coverage line-rate="0.70" />', encoding="utf-8")

    pr_sizes = tmp_path / "pr-sizes.json"
    pr_sizes.write_text(
        json.dumps({"artifacts": [{"name": "QGroundControl.dmg", "size_bytes": 10485760, "size_human": "10.00 MB"}]}),
        encoding="utf-8",
    )
    baseline_sizes = tmp_path / "baseline-sizes.json"
    baseline_sizes.write_text(
        json.dumps({"artifacts": [{"name": "QGroundControl.dmg", "size_bytes": 9437184, "size_human": "9.00 MB"}]}),
        encoding="utf-8",
    )

    env = {
        "BUILD_TABLE": "| Platform | Status | Details |\n|----------|--------|--------|",
        "BUILD_SUMMARY": "Some builds failed.",
        "PRECOMMIT_STATUS": "Not Triggered",
        "PRECOMMIT_URL": "",
        "COVERAGE_XML": "artifacts/coverage-report/coverage.xml",
        "BASELINE_COVERAGE_XML": "baseline-coverage.xml",
        "PR_SIZES_JSON": "pr-sizes.json",
        "BASELINE_SIZES_JSON": "baseline-sizes.json",
        "TRIGGERED_BY": "MacOS",
    }

    out = generate_comment(env, tmp_path, now_utc=datetime(2026, 2, 17, 12, 0, 0, tzinfo=UTC))

    assert "### Test Results" in out
    assert "**linux_gcc_64**: 1 passed, 1 failed, 0 skipped" in out
    assert "| 75.0% | 70.0% | +5.0% |" in out
    assert "| QGroundControl.dmg | 10.00 MB | +1.00 MB (increase) |" in out
    assert "**Total size increased by 1.00 MB**" in out
