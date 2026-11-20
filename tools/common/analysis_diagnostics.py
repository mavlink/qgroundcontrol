"""Reading of analyze.py's ``*-diagnostics.json`` reports: each tool's unique warnings.

Each compiler-analysis job (clang-tidy, Clazy) writes ``<tool>-diagnostics.json``:
``{"tool": ..., "commit": ..., "diagnostics": {file: [[line, column, severity, check], ...]}}``,
plus, on pull requests, ``"changed"`` for the lines the pull request changed, whose rows add each
diagnostic's message and the source excerpt printed under it. ``commit`` is the analyzed
checkout, whose line numbers the rows use. ``is_bug_class`` is the one definition of the findings
that pull request reports put first.
"""

from __future__ import annotations

import json
import logging
import re
from dataclasses import dataclass
from typing import TYPE_CHECKING, Any

from .format import clip
from .io import read_json

if TYPE_CHECKING:
    from collections.abc import Iterator
    from pathlib import Path

__all__ = [
    "DIAGNOSTICS_SUFFIX",
    "MAX_CONTEXT_CHARS",
    "MAX_MESSAGE_CHARS",
    "AnalysisWarning",
    "is_bug_class",
    "load_analysis_commit",
    "load_changed_findings",
    "load_unique_warnings",
]

logger = logging.getLogger(__name__)

DIAGNOSTICS_SUFFIX = "-diagnostics.json"
# Longest message and source excerpt analyze.py records; readers recheck them, since reports
# come from pull requests.
MAX_MESSAGE_CHARS = 500
MAX_CONTEXT_CHARS = 1000


@dataclass(frozen=True)
class AnalysisWarning:
    """One diagnostic; ``severity`` is "error" only for load_changed_findings' errors."""

    tool: str
    file: str
    line: int
    column: int
    check: str
    message: str = ""
    severity: str = "warning"
    context: str = ""


def is_bug_class(finding: AnalysisWarning) -> bool:
    """Return whether *finding* reports a likely bug rather than style or cost.

    That is an error from any tool, or a clang-tidy ``bugprone-*`` or ``clang-analyzer-*``
    warning. A pull request gets these as inline review comments and first in its Build Results
    comment; every other warning (``performance-*``, Clazy) is only in that comment.
    """
    if finding.severity == "error":
        return True
    # clang-tidy joins the enabled aliases of one check with commas.
    return finding.tool == "clang-tidy" and any(
        name.startswith(("bugprone-", "clang-analyzer-")) for name in finding.check.split(",")
    )


def _warnings(
    tool: str,
    by_file: Any,
    severities: frozenset[str] = frozenset({"warning"}),
    *,
    exact_paths: bool = False,
) -> set[AnalysisWarning]:
    """Read report rows, skipping malformed ones; reports come from pull request code.

    Paths are clipped for display unless *exact_paths*, which keeps them matchable against
    GitHub's file names.
    """
    found: set[AnalysisWarning] = set()
    if not isinstance(by_file, dict):
        return found
    for file, rows in by_file.items():
        if not isinstance(rows, list):
            continue
        for row in rows:
            if (
                isinstance(row, list)
                and 4 <= len(row) <= 6
                and isinstance(row[0], int)
                and isinstance(row[1], int)
                and isinstance(row[2], str)
                and row[2] in severities
            ):
                found.add(
                    AnalysisWarning(
                        tool,
                        str(file) if exact_paths else clip(str(file)),
                        row[0],
                        row[1],
                        clip(str(row[3])),
                        clip(str(row[4]), MAX_MESSAGE_CHARS) if len(row) > 4 else "",
                        str(row[2]),
                        str(row[5])[:MAX_CONTEXT_CHARS] if len(row) > 5 else "",
                    )
                )
    return found


def _reports(directory: Path) -> Iterator[tuple[str, dict[str, Any]]]:
    """Yield each readable ``*-diagnostics.json`` under *directory* with its tool name."""
    for path in sorted(directory.glob(f"**/*{DIAGNOSTICS_SUFFIX}")):
        try:
            data = read_json(path)
        except (OSError, UnicodeDecodeError, json.JSONDecodeError):
            logger.warning("Failed to parse analysis diagnostics from %s", path, exc_info=True)
            continue
        if isinstance(data, dict) and isinstance(data.get("diagnostics"), dict):
            yield clip(data.get("tool") or path.name.removesuffix(DIAGNOSTICS_SUFFIX)), data


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
    for tool, data in _reports(directory):
        merged.setdefault(tool, set()).update(_warnings(tool, data["diagnostics"]))
        if "changed" in data:
            changed.setdefault(tool, set()).update(_warnings(tool, data["changed"]))
    return merged, changed


def load_changed_findings(directory: Path) -> set[AnalysisWarning]:
    """Return every shard's warnings and errors on the lines a pull request changed.

    Their paths are exact, for matching against the pull request's files; clip them to display.
    """
    found: set[AnalysisWarning] = set()
    for tool, data in _reports(directory):
        found |= _warnings(
            tool, data.get("changed"), frozenset({"warning", "error"}), exact_paths=True
        )
    return found


def load_analysis_commit(directory: Path) -> str:
    """Return the commit that every ``*-diagnostics.json`` under *directory* naming one analyzed.

    Returns '' when none names one or they disagree. On a pull request Code Analysis checks out
    GitHub's merge commit, whose line numbers can differ from the PR head's where the base branch
    changed the same file.
    """
    commits = {
        commit
        for _, data in _reports(directory)
        if isinstance(commit := data.get("commit"), str)
        and re.fullmatch(r"[0-9a-f]{40}|[0-9a-f]{64}", commit)
    }
    return commits.pop() if len(commits) == 1 else ""
