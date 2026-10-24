"""Typed reading of gcovr JSON coverage summaries (``gcovr --json-summary``)."""

from __future__ import annotations

import math
from dataclasses import dataclass, replace
from pathlib import Path
from typing import TYPE_CHECKING, Any

from .io import read_json

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
    Function counts are optional and default to zero when a summary omits them.
    """

    line_percent: float
    line_total: int
    line_covered: int
    branch_percent: float
    branch_total: int
    branch_covered: int
    function_percent: float = 0.0
    function_total: int = 0
    function_covered: int = 0


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


def _derived_percent(covered: int, total: int) -> float:
    """gcovr's rounding: one decimal, and 100.0 only when everything is covered."""
    if not total:
        return 0.0
    percent = round(100.0 * covered / total, 1)
    return 99.9 if percent == 100.0 and covered < total else percent


def _files_under(summary: dict[str, Any], prefix: str) -> dict[str, Any]:
    """Return totals summed over the per-file entries whose filename starts with *prefix*."""
    files = summary.get("files")
    if not isinstance(files, list):
        raise CoverageSummaryError("coverage summary files must be a list")
    counts = dict.fromkeys(
        (
            f"{metric}_{kind}"
            for metric in ("line", "branch", "function")
            for kind in ("total", "covered")
        ),
        0,
    )
    for entry in files:
        if not isinstance(entry, dict) or not str(entry.get("filename", "")).startswith(prefix):
            continue
        for key in counts:
            counts[key] += _count(entry, key) if key in entry else 0
    percents = {
        f"{metric}_percent": _derived_percent(
            counts[f"{metric}_covered"], counts[f"{metric}_total"]
        )
        for metric in ("line", "branch", "function")
    }
    return {**counts, **percents}


def read_gcovr_summary(path: str | PathLike[str], *, path_prefix: str = "") -> CoverageSummary:
    """Read the project totals from a gcovr JSON summary report.

    With *path_prefix* (such as ``src/``), sum only the files under it instead.
    """
    try:
        summary = read_json(Path(path))
    except (OSError, UnicodeDecodeError, ValueError) as exc:
        raise CoverageSummaryError(f"failed to parse {path}: {exc}") from exc
    if not isinstance(summary, dict):
        raise CoverageSummaryError(f"{path} is not a gcovr JSON summary object")
    if path_prefix:
        summary = _files_under(summary, path_prefix)

    metrics = CoverageSummary(
        line_percent=_percent(summary, "line_percent"),
        line_total=_count(summary, "line_total"),
        line_covered=_count(summary, "line_covered"),
        branch_percent=_percent(summary, "branch_percent"),
        branch_total=_count(summary, "branch_total"),
        branch_covered=_count(summary, "branch_covered"),
    )
    if "function_total" in summary:
        metrics = replace(
            metrics,
            function_percent=_percent(summary, "function_percent"),
            function_total=_count(summary, "function_total"),
            function_covered=_count(summary, "function_covered"),
        )
    if (
        metrics.line_covered > metrics.line_total
        or metrics.branch_covered > metrics.branch_total
        or metrics.function_covered > metrics.function_total
    ):
        raise CoverageSummaryError("coverage summary reports more covered than total elements")
    return metrics
