"""Typed reading of gcovr JSON coverage summaries (``gcovr --json-summary``)."""

from __future__ import annotations

import json
import math
from dataclasses import dataclass
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from os import PathLike

__all__ = ["SUMMARY_FILENAME", "CoverageSummary", "CoverageSummaryError", "read_gcovr_summary"]

# Written next to coverage.xml by the coverage and coverage-report CMake targets.
SUMMARY_FILENAME = "coverage-summary.json"


class CoverageSummaryError(ValueError):
    """Raised when a gcovr summary cannot provide valid metrics."""


@dataclass(frozen=True)
class CoverageSummary:
    """Project-wide gcovr metrics.

    gcovr rounds percentages to one decimal and reports 100.0 only when everything is covered;
    a metric with no elements reports 0.0, so check the total before showing its percentage.
    """

    line_percent: float
    line_total: int
    line_covered: int
    branch_percent: float
    branch_total: int
    branch_covered: int


def _count(summary: dict[str, Any], key: str) -> int:
    value = summary.get(key)
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise CoverageSummaryError(f"coverage summary {key} must be a non-negative integer")
    return value


def _percent(summary: dict[str, Any], key: str) -> float:
    value = summary.get(key)
    if (
        isinstance(value, bool)
        or not isinstance(value, (int, float))
        or not math.isfinite(value)
        or not 0.0 <= value <= 100.0
    ):
        raise CoverageSummaryError(f"coverage summary {key} must be a number from 0 through 100")
    return float(value)


def read_gcovr_summary(path: str | PathLike[str]) -> CoverageSummary:
    """Read the project totals from a gcovr JSON summary report."""
    try:
        with open(path, encoding="utf-8") as stream:
            summary = json.load(stream)
    except (OSError, UnicodeDecodeError, ValueError) as exc:
        raise CoverageSummaryError(f"failed to parse {path}: {exc}") from exc
    if not isinstance(summary, dict):
        raise CoverageSummaryError(f"{path} is not a gcovr JSON summary object")

    metrics = CoverageSummary(
        line_percent=_percent(summary, "line_percent"),
        line_total=_count(summary, "line_total"),
        line_covered=_count(summary, "line_covered"),
        branch_percent=_percent(summary, "branch_percent"),
        branch_total=_count(summary, "branch_total"),
        branch_covered=_count(summary, "branch_covered"),
    )
    if metrics.line_covered > metrics.line_total or metrics.branch_covered > metrics.branch_total:
        raise CoverageSummaryError("coverage summary reports more covered than total elements")
    return metrics
