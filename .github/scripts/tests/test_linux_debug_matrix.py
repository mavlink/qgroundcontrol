"""Linux debug matrix contracts."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

from linux_debug_matrix import COVERAGE_JOB, SANITIZER_JOB, build_matrix, main

if TYPE_CHECKING:
    from pathlib import Path

    import pytest


def test_matrix_adds_sanitizers_only_for_non_pr_builds() -> None:
    assert build_matrix(is_pr=True) == [COVERAGE_JOB]
    assert build_matrix(is_pr=False) == [COVERAGE_JOB, SANITIZER_JOB]


def test_main_parses_pr_flags_and_writes_compact_json(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    output = tmp_path / "gha-output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output))
    for flag, expected in (
        ("0", [COVERAGE_JOB, SANITIZER_JOB]),
        ("1", [COVERAGE_JOB]),
        ("true", [COVERAGE_JOB]),
        ("True", [COVERAGE_JOB]),
        ("yes", [COVERAGE_JOB]),
        ("YES", [COVERAGE_JOB]),
    ):
        output.write_text("")
        monkeypatch.setenv("IS_PR", flag)
        assert main([]) == 0
        serialized = output.read_text().split("=", 1)[1].rstrip("\n")
        assert json.loads(serialized) == expected
        assert ", " not in serialized and ": " not in serialized
        assert "include=" in capsys.readouterr().out
