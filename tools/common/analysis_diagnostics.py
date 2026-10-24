"""Reading of analyze.py's ``*-diagnostics.json`` reports: each tool's unique warnings.

Each compiler-analysis job (clang-tidy, Clazy) writes ``<tool>-diagnostics.json``:
``{"tool": ..., "diagnostics": {file: [[line, column, severity, check], ...]}}``, plus, on pull
requests, ``"changed"`` with the same shape for the lines the pull request changed.
"""

from __future__ import annotations

import json
import logging
from dataclasses import dataclass
from typing import TYPE_CHECKING, Any

from .format import clip
from .io import read_json

if TYPE_CHECKING:
    from pathlib import Path

__all__ = ["DIAGNOSTICS_SUFFIX", "AnalysisWarning", "load_unique_warnings"]

logger = logging.getLogger(__name__)

DIAGNOSTICS_SUFFIX = "-diagnostics.json"


@dataclass(frozen=True)
class AnalysisWarning:
    tool: str
    file: str
    line: int
    column: int
    check: str


def _warnings(tool: str, by_file: Any) -> set[AnalysisWarning]:
    found: set[AnalysisWarning] = set()
    if not isinstance(by_file, dict):
        return found
    for file, rows in by_file.items():
        if not isinstance(rows, list):
            continue
        for row in rows:
            if (
                isinstance(row, list)
                and len(row) == 4
                and isinstance(row[0], int)
                and isinstance(row[1], int)
                and row[2] == "warning"
            ):
                found.add(AnalysisWarning(tool, clip(str(file)), row[0], row[1], clip(str(row[3]))))
    return found


def load_unique_warnings(
    directory: Path,
) -> tuple[dict[str, set[AnalysisWarning]], dict[str, set[AnalysisWarning]]]:
    """Union every shard's ``*-diagnostics.json`` warnings under *directory* by tool.

    A header's warnings appear in each shard that compiles a file including it, so a warning is
    identified by its location and check. Returns every warning and, for tools whose shards
    reported changed lines, the warnings on those lines. Unreadable reports are logged and skipped.
    """
    merged: dict[str, set[AnalysisWarning]] = {}
    changed: dict[str, set[AnalysisWarning]] = {}
    for path in sorted(directory.glob(f"**/*{DIAGNOSTICS_SUFFIX}")):
        try:
            data = read_json(path)
        except (OSError, UnicodeDecodeError, json.JSONDecodeError):
            logger.warning("Failed to parse analysis diagnostics from %s", path, exc_info=True)
            continue
        if not isinstance(data, dict) or not isinstance(data.get("diagnostics"), dict):
            continue
        tool = clip(data.get("tool") or path.name.removesuffix(DIAGNOSTICS_SUFFIX))
        merged.setdefault(tool, set()).update(_warnings(tool, data["diagnostics"]))
        if "changed" in data:
            changed.setdefault(tool, set()).update(_warnings(tool, data["changed"]))
    return merged, changed
