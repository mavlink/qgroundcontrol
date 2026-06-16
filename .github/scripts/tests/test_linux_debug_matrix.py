"""Tests for linux_debug_matrix.py."""

from __future__ import annotations

import json

import pytest
from linux_debug_matrix import COVERAGE_JOB, SANITIZER_JOB, build_matrix, main


def test_build_matrix_pr_is_coverage_only() -> None:
    assert build_matrix(is_pr=True) == [COVERAGE_JOB]


def test_build_matrix_non_pr_adds_sanitizers() -> None:
    assert build_matrix(is_pr=False) == [COVERAGE_JOB, SANITIZER_JOB]


def test_main_writes_github_output_for_pr(
    tmp_path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    output_file = tmp_path / "gha_output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    monkeypatch.setenv("IS_PR", "1")

    assert main([]) == 0

    content = output_file.read_text()
    assert content.startswith("include=")
    payload = json.loads(content.split("=", 1)[1])
    assert payload == [COVERAGE_JOB]
    assert "include=" in capsys.readouterr().out


def test_main_non_pr_includes_sanitizers(tmp_path, monkeypatch: pytest.MonkeyPatch) -> None:
    output_file = tmp_path / "gha_output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    monkeypatch.setenv("IS_PR", "0")

    assert main([]) == 0

    payload = json.loads(output_file.read_text().split("=", 1)[1])
    assert payload == [COVERAGE_JOB, SANITIZER_JOB]


@pytest.mark.parametrize("flag", ["1", "true", "True", "yes", "YES"])
def test_main_truthy_pr_flag_values(flag: str, tmp_path, monkeypatch: pytest.MonkeyPatch) -> None:
    output_file = tmp_path / "gha_output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    monkeypatch.setenv("IS_PR", flag)
    main([])
    assert json.loads(output_file.read_text().split("=", 1)[1]) == [COVERAGE_JOB]


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
