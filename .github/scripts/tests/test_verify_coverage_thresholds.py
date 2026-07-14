"""Coverage threshold enforcement contracts."""

from __future__ import annotations

import sys
from typing import TYPE_CHECKING

from verify_coverage_thresholds import main

if TYPE_CHECKING:
    from pathlib import Path

    import pytest


def _run(monkeypatch: pytest.MonkeyPatch, path: Path, *thresholds: str) -> int:
    monkeypatch.setattr(sys, "argv", ["prog", "--coverage-xml", str(path), *thresholds])
    return main()


def test_missing_coverage_is_non_blocking(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    assert _run(monkeypatch, tmp_path / "missing.xml") == 0


def test_thresholds_accept_healthy_coverage_and_reject_each_failure(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    coverage = tmp_path / "coverage.xml"
    thresholds = ("--line-threshold", "30", "--branch-threshold", "20")
    cases = [
        ((100, 80, 0.80, 0.60), 0),
        ((100, 10, 0.10, 0.50), 1),
        ((100, 80, 0.80, 0.10), 1),
        ((0, 0, 0.0, 0.0), 1),
    ]
    for (valid, covered, line_rate, branch_rate), expected in cases:
        coverage.write_text(
            f'<coverage lines-valid="{valid}" lines-covered="{covered}" '
            f'line-rate="{line_rate}" branch-rate="{branch_rate}"/>'
        )
        assert _run(monkeypatch, coverage, *thresholds) == expected
