"""End-to-end contracts for the generated PR build-results comment."""

from __future__ import annotations

import json
from datetime import datetime, timezone
from typing import TYPE_CHECKING

from generate_build_results_comment import generate_comment

if TYPE_CHECKING:
    from pathlib import Path

NOW = datetime(2026, 2, 17, 12, 0, tzinfo=timezone.utc)
TABLE = "| Platform | Status | Details |\n|----------|--------|--------|"


def _env(**overrides: str) -> dict[str, str]:
    return {
        "BUILD_TABLE": TABLE,
        "BUILD_SUMMARY": "All builds passed.",
        "PRECOMMIT_STATUS": "Not Triggered",
        "PRECOMMIT_URL": "",
        "TRIGGERED_BY": "Linux",
        **overrides,
    }


def _write(path: Path, content: object) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content if isinstance(content, str) else json.dumps(content))
    return path


def _generate(tmp_path: Path, **environment: str) -> str:
    return generate_comment(_env(**environment), tmp_path, now_utc=NOW)


def test_minimal_comment_has_build_precommit_and_provenance_sections(tmp_path: Path) -> None:
    build_row = "| Linux | Passed | [View](https://example.test) |"
    output = _generate(tmp_path, BUILD_TABLE=f"{TABLE}\n{build_row}")
    assert "## Build Results" in output
    assert f"{build_row}\n\n**All builds passed.**" in output
    assert "| pre-commit | Not Triggered | - |" in output
    assert "<sub>Updated: 2026-02-17 12:00:00 UTC • Triggered by: Linux</sub>" in output


def test_missing_optional_artifacts_preserves_status_with_explanation(tmp_path: Path) -> None:
    output = _generate(
        tmp_path,
        ARTIFACT_DOWNLOAD_FAILED="true",
        BUILD_RESULTS_URL="https://example.test/actions/runs/123",
    )
    assert "**All builds passed.**" in output
    assert "Test and coverage details were unavailable for this commit" in output
    assert "[View workflow run](https://example.test/actions/runs/123)" in output
    assert "actions/runs/123).\n\n### Pre-commit" in output


def test_precommit_artifact_overrides_live_status(tmp_path: Path) -> None:
    path = _write(
        tmp_path / "artifacts/pre-commit-results/results.json",
        {
            "exit_code": "1",
            "passed": "12",
            "failed": "3",
            "skipped": "1",
            "run_url": "https://example.test/precommit",
        },
    )
    output = _generate(
        tmp_path,
        PRECOMMIT_STATUS="Running",
        PRECOMMIT_RESULTS_PATH=str(path.relative_to(tmp_path)),
    )
    assert (
        "| pre-commit | Failed (non-blocking) | [View](https://example.test/precommit) |" in output
    )
    assert "Pre-commit hooks: 12 passed, 3 failed, 1 skipped." in output


def test_comment_aggregates_tests_coverage_and_artifact_size_delta(tmp_path: Path) -> None:
    _write(
        tmp_path / "artifacts/test-results-linux_gcc_64/test-output.txt",
        "1/3 Test   #1: AlphaTest ... Passed\n"
        "2/3 Test  #12: BetaTest ... ***Failed\n"
        "3/3 Test #123: GammaTest ... ***Skipped",
    )
    _write(
        tmp_path / "artifacts/coverage-report/coverage.xml",
        '<coverage line-rate="0.75" lines-valid="100"/>',
    )
    _write(
        tmp_path / "baseline-coverage.xml",
        '<coverage line-rate="0.70" lines-valid="100"/>',
    )
    _write(
        tmp_path / "pr-sizes.json",
        {"artifacts": [{"name": "QGroundControl.dmg", "size_bytes": 10485760}]},
    )
    _write(
        tmp_path / "baseline-sizes.json",
        {"artifacts": [{"name": "QGroundControl.dmg", "size_bytes": 9437184}]},
    )

    output = _generate(
        tmp_path,
        BUILD_SUMMARY="Some builds failed.",
        COVERAGE_XML="artifacts/coverage-report/coverage.xml",
        BASELINE_COVERAGE_XML="baseline-coverage.xml",
        PR_SIZES_JSON="pr-sizes.json",
        BASELINE_SIZES_JSON="baseline-sizes.json",
    )
    assert "**linux_gcc_64**: 1 passed, 1 failed, 1 skipped" in output
    assert "2/3 Test  #12: BetaTest ... ***Failed" in output
    assert "| 75.0% | 70.0% | +5.0% |" in output
    assert "| QGroundControl.dmg | 10.00 MB | +1.00 MB (increase) |" in output
    assert "**Total size increased by 1.00 MB**" in output


def test_precommit_links_reject_schemes_and_escape_markdown(tmp_path: Path) -> None:
    output = _generate(tmp_path, PRECOMMIT_STATUS="Passed", PRECOMMIT_URL="javascript:alert(1)")
    assert "| pre-commit | Passed | - |" in output

    path = _write(
        tmp_path / "precommit.json",
        {
            "exit_code": "0",
            "passed": "1",
            "failed": "0",
            "skipped": "0",
            "run_url": "https://example.test/precommit) [oops](https://bad.test)",
        },
    )
    output = _generate(
        tmp_path,
        PRECOMMIT_STATUS="Running",
        PRECOMMIT_RESULTS_PATH=path.name,
    )
    assert "[View](https://example.test/precommit%29%20%5Boops%5D%28https://bad.test%29)" in output
    assert "[oops]" not in output


def test_malformed_size_entries_are_ignored(tmp_path: Path) -> None:
    _write(
        tmp_path / "sizes.json",
        {
            "artifacts": [
                {"name": "QGroundControl.dmg", "size_bytes": 1048576},
                {"name": "", "size_bytes": 10},
                {"size_human": "1 MB"},
                "bad-entry",
            ]
        },
    )
    assert "| QGroundControl.dmg | 1.00 MB |" in _generate(tmp_path, PR_SIZES_JSON="sizes.json")
