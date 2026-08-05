"""Tests for linux_debug_matrix.py."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

from linux_debug_matrix import COVERAGE_JOB, SANITIZER_JOB, build_matrix, main

if TYPE_CHECKING:
    import pytest


def test_build_matrix_includes_coverage_and_sanitizers() -> None:
    assert build_matrix() == [COVERAGE_JOB, SANITIZER_JOB]


def test_main_writes_github_output(
    tmp_path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    output_file = tmp_path / "gha_output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))

    assert main([]) == 0

    content = output_file.read_text()
    assert content.startswith("include=")
    payload = json.loads(content.split("=", 1)[1])
    assert payload == [COVERAGE_JOB, SANITIZER_JOB]
    assert "include=" in capsys.readouterr().out


def test_main_output_uses_compact_json_separators(
    tmp_path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """Compact JSON in $GITHUB_OUTPUT keeps log lines short and avoids ambiguity."""
    output_file = tmp_path / "gha_output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    main([])
    serialized = output_file.read_text().split("=", 1)[1].rstrip("\n")
    assert ", " not in serialized
    assert ": " not in serialized
