"""Tests for android_matrix.py."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

from android_matrix import (
    LINUX_EMULATOR_JOB,
    LINUX_JOB,
    MAC_JOB,
    WINDOWS_JOB,
    build_matrix,
    main,
)

if TYPE_CHECKING:
    import pytest


def test_build_matrix_pr_skips_windows() -> None:
    assert build_matrix(is_pr=True) == [LINUX_JOB, MAC_JOB, LINUX_EMULATOR_JOB]


def test_build_matrix_non_pr_includes_all_legs() -> None:
    assert build_matrix(is_pr=False) == [LINUX_JOB, MAC_JOB, WINDOWS_JOB, LINUX_EMULATOR_JOB]


def test_legs_carry_runner_fields() -> None:
    for leg in build_matrix(is_pr=False):
        assert "runson_runner" in leg
        assert leg["fallback_runner"]


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
    assert payload == [LINUX_JOB, MAC_JOB, LINUX_EMULATOR_JOB]
    assert "include=" in capsys.readouterr().out


def test_main_non_pr_includes_windows(tmp_path, monkeypatch: pytest.MonkeyPatch) -> None:
    output_file = tmp_path / "gha_output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    monkeypatch.setenv("IS_PR", "0")

    assert main([]) == 0

    payload = json.loads(output_file.read_text().split("=", 1)[1])
    assert payload == [LINUX_JOB, MAC_JOB, WINDOWS_JOB, LINUX_EMULATOR_JOB]


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
