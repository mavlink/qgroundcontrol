#!/usr/bin/env python3
"""Generate the Build Results PR comment.

Reads the workflow status that collect_build_status.py wrote (--status-out); the downloaded test
results, coverage, pre-commit, and Code Analysis artifacts; the artifact sizes; and the PR base
commit's own build artifacts, from the INPUT_PATHS that build-results.yml's steps write. The
comment leads with a verdict and a per-workflow table, lists failures, and collapses the rest.
"""

from __future__ import annotations

import argparse
import logging
import os
import re
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import TYPE_CHECKING, Any
from urllib.parse import quote, urlsplit, urlunsplit

if TYPE_CHECKING:
    from collections.abc import Mapping

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.analysis_diagnostics import DIAGNOSTICS_SUFFIX, AnalysisWarning, load_unique_warnings
from common.coverage_summary import CoverageSummary, CoverageSummaryError, read_gcovr_summary
from common.format import clip, format_bytes, format_count, format_duration, format_percent
from common.gh_actions import parse_github_timestamp, write_step_summary
from common.io import read_json
from common.junit import JUnitCase, parse_junit
from common.markdown import code_block, code_span, details, escape_text, md_table, more_items
from common.xml import XMLParseError

from ci.reporting.workflow_runs import triggering_run

logger = logging.getLogger(__name__)

# GitHub rejects comments over 65,536 characters; the margin covers the comment marker.
MAX_COMMENT_CHARS = 60_000
# An artifact that grows by more than SIZE_WARN_PERCENT of its base size, and by at least
# SIZE_WARN_MIN_BYTES, is flagged.
SIZE_WARN_PERCENT = 1.0
SIZE_WARN_MIN_BYTES = 1024 * 1024
# Coverage covers the application only; Codecov also ignores test/.
COVERAGE_PREFIX = "src/"

STATE_LABELS = {
    "success": "✅ Passed",
    "failure": "❌ Failed",
    "cancelled": "🚫 Cancelled",
    "running": "🔄 Running",
    "pending": "⏳ Pending",
    "skipped": "⏭️ Skipped",
    "action_required": "⏸️ Awaiting approval",
    "not_run": "⚪ Not run",
}
_STATE_ICONS = {state: label.split(" ", 1)[0] for state, label in STATE_LABELS.items()}
_PENDING = {"pending", "running", "action_required"}
_FAILED_JOB_CONCLUSIONS = {"failure", "timed_out", "startup_failure"}
# Inputs relative to --base-dir, where build-results.yml's steps write them; an environment
# variable of the same name overrides each.
INPUT_PATHS = {
    "BUILD_STATUS_JSON": "build-status.json",
    "ARTIFACTS_DIR": "artifacts",
    "ANALYSIS_ARTIFACTS_DIR": "analysis-artifacts",
    "PRECOMMIT_RESULTS_PATH": "artifacts/pre-commit-results/pre-commit-results.json",
    "COVERAGE_SUMMARY": "artifacts/coverage-report/coverage-summary.json",
    "PR_SIZES_JSON": "pr-sizes.json",
    "BASE_ARTIFACTS_DIR": "base-artifacts",
    "BASE_COVERAGE_SUMMARY": "base-artifacts/coverage-report/coverage-summary.json",
    "BASE_SIZES_JSON": "base-sizes.json",
}


@dataclass(frozen=True)
class Limits:
    """How much of each list the comment shows; smaller sets keep a large report under the cap."""

    failed_jobs: int = 10
    failed_tests: int = 10
    excerpt_lines: int = 8
    line_chars: int = 240
    suites: int = 30
    test_names: int = 10
    artifacts: int = 30
    analysis_warnings: int = 20


LIMIT_STEPS = (
    Limits(),
    Limits(
        failed_tests=5, excerpt_lines=4, suites=12, test_names=5, artifacts=12, analysis_warnings=10
    ),
    Limits(
        failed_jobs=5,
        failed_tests=3,
        excerpt_lines=0,
        suites=6,
        test_names=3,
        artifacts=6,
        analysis_warnings=5,
    ),
)


@dataclass(frozen=True)
class Suite:
    name: str
    workflow: str
    cases: tuple[JUnitCase, ...]
    wall_seconds: float | None

    def count(self, outcome: str) -> int:
        return sum(case.outcome == outcome for case in self.cases)

    @property
    def names(self) -> frozenset[str]:
        return frozenset(case.name for case in self.cases)


@dataclass(frozen=True)
class ArtifactSize:
    name: str
    workflow: str
    size: int
    base: int | None
    run_id: int = 0
    artifact_id: int = 0
    expires: str = ""

    @property
    def delta(self) -> int | None:
        return None if self.base is None else self.size - self.base

    @property
    def percent(self) -> float | None:
        if self.base is None or self.base <= 0:
            return None
        return 100.0 * (self.size - self.base) / self.base

    @property
    def flagged(self) -> bool:
        delta, percent = self.delta, self.percent
        return (
            delta is not None
            and percent is not None
            and delta >= SIZE_WARN_MIN_BYTES
            and percent > SIZE_WARN_PERCENT
        )


@dataclass(frozen=True)
class BaseRun:
    workflow: str
    run_number: object
    status: str
    conclusion: str
    url: str


@dataclass(frozen=True)
class Base:
    """The PR base commit's own builds; *runs* is None when they could not be read."""

    sha: str
    runs: tuple[BaseRun, ...] | None


@dataclass(frozen=True)
class Coverage:
    current: CoverageSummary
    baseline: CoverageSummary | None
    base_workflow: str


@dataclass(frozen=True)
class Precommit:
    passed: int
    failed: int
    skipped: int
    failed_hooks: tuple[str, ...]
    run_url: str


@dataclass(frozen=True)
class AnalysisTool:
    tool: str
    files: tuple[str, ...]
    findings: tuple[str, ...]
    errors: tuple[str, ...]
    # Unique warnings across shards; None without diagnostics files.
    warnings: int | None = None
    # Warnings on lines the PR changed; None when no shard reported changed lines.
    changed: tuple[AnalysisWarning, ...] | None = None


@dataclass(frozen=True)
class TestChanges:
    """Test names added and removed in suites that the base build also ran, by suite."""

    added: dict[str, tuple[str, ...]]
    removed: dict[str, tuple[str, ...]]
    compared: frozenset[str]


@dataclass(frozen=True)
class Report:
    platforms: tuple[dict[str, Any], ...]
    precommit_run: dict[str, Any] | None
    analysis_run: dict[str, Any] | None
    others: tuple[dict[str, Any], ...]
    suites: tuple[Suite, ...]
    test_changes: TestChanges | None
    coverage: Coverage | None
    sizes: tuple[ArtifactSize, ...]
    base: Base | None
    precommit: Precommit | None
    analysis: tuple[AnalysisTool, ...]
    missing_downloads: tuple[str, ...]
    expired_downloads: tuple[str, ...]
    repo: str
    server_url: str
    commit: str
    # The commit Code Analysis checked out, whose line numbers its warnings use; '' when unknown.
    analysis_commit: str
    pr_number: str
    triggered_by: str
    now: datetime


def _env(env: Mapping[str, str], key: str, default: str = "") -> str:
    return str(env.get(key, "") or default).strip()


def _input_path(env: Mapping[str, str], base_dir: Path, key: str) -> Path:
    return base_dir / _env(env, key, INPUT_PATHS[key])


def _plural(count: int, noun: str) -> str:
    return f"{format_count(count)} {noun}{'' if count == 1 else 's'}"


def _int(value: Any) -> int:
    try:
        return int(str(value).strip())
    except (TypeError, ValueError):
        return 0


def _sanitize_external_url(url: str) -> str | None:
    value = url.strip()
    if not value:
        return None
    if any(ch in value for ch in ("\r", "\n", "\t")):
        return None

    parsed = urlsplit(value)
    if parsed.scheme not in {"http", "https"} or not parsed.netloc:
        return None

    path = quote(parsed.path, safe="/-._~!$&'*,;=:@%+")
    query = quote(parsed.query, safe="&=:@/?-._~!$'*,;%+")
    fragment = quote(parsed.fragment, safe="-._~!$&'*,;=:@/?%+")
    return urlunsplit((parsed.scheme, parsed.netloc, path, query, fragment))


def _link(text: str, url: str) -> str:
    """Return a Markdown link with already-escaped *text*, or the text alone for unsafe URLs."""
    safe_url = _sanitize_external_url(url)
    return f"[{text}]({safe_url})" if safe_url else text


def _delta_size(delta: int) -> str:
    """Return a signed size change in B, KB, or MB (1024-based, like format_bytes)."""
    if delta == 0:
        return "unchanged"
    sign = "+" if delta > 0 else "-"
    size = abs(delta)
    if size < 1024:
        return f"{sign}{size} B"
    if size < 1024 * 1024:
        return f"{sign}{size / 1024:.1f} KB"
    return f"{sign}{format_bytes(size)}"


def _clock(timestamp: str) -> str:
    parsed = parse_github_timestamp(timestamp)
    return parsed.strftime("%H:%M UTC") if parsed else ""


# ---------------------------------------------------------------------------
# Inputs
# ---------------------------------------------------------------------------


def _load_json(path: Path, what: str) -> Any:
    if not path.is_file():
        return None
    try:
        return read_json(path)
    except (OSError, ValueError):
        logger.warning("Failed to parse %s from %s", what, path, exc_info=True)
        return None


def _manifest(artifacts_dir: Path) -> list[dict[str, Any]]:
    data = _load_json(artifacts_dir / "download-manifest.json", "the download manifest")
    return [entry for entry in data if isinstance(entry, dict)] if isinstance(data, list) else []


def load_suites(artifacts_dir: Path) -> list[Suite]:
    """Return one suite per downloaded test-results-* artifact with readable JUnit XML."""
    workflows = {
        str(entry.get("name", "")): clip(entry.get("workflow", ""))
        for entry in _manifest(artifacts_dir)
    }
    suites: list[Suite] = []
    for directory in sorted(artifacts_dir.glob("test-results-*")):
        if not directory.is_dir():
            continue
        name = clip(directory.name.removeprefix("test-results-"))
        cases: list[JUnitCase] = []
        wall: float | None = None
        for junit in sorted(directory.glob("*.xml")):
            try:
                found, seconds = parse_junit(junit, name)
            except (OSError, XMLParseError):
                logger.warning("Failed to parse JUnit results from %s", junit, exc_info=True)
                continue
            cases += found
            if seconds is not None:
                wall = (wall or 0.0) + seconds
        if cases:
            suites.append(Suite(name, workflows.get(directory.name, ""), tuple(cases), wall))
    return suites


def compare_tests(suites: list[Suite], base_suites: list[Suite]) -> TestChanges | None:
    """Return the tests each suite added or removed versus the same suite in the base build."""
    if not base_suites:
        return None
    base = {suite.name: suite.names for suite in base_suites}
    added: dict[str, list[str]] = {}
    removed: dict[str, list[str]] = {}
    compared = set()
    for suite in suites:
        if suite.name not in base:
            continue
        compared.add(suite.name)
        for name in suite.names - base[suite.name]:
            added.setdefault(name, []).append(suite.name)
        for name in base[suite.name] - suite.names:
            removed.setdefault(name, []).append(suite.name)
    return TestChanges(
        added={name: tuple(sorted(found)) for name, found in sorted(added.items())},
        removed={name: tuple(sorted(found)) for name, found in sorted(removed.items())},
        compared=frozenset(compared),
    )


def _read_coverage(path: Path) -> CoverageSummary | None:
    if not path.is_file():
        return None
    try:
        summary = read_gcovr_summary(path, path_prefix=COVERAGE_PREFIX)
    except CoverageSummaryError:
        logger.warning("Failed to parse coverage from %s", path, exc_info=True)
        return None
    return summary if summary.line_total else None


def load_coverage(env: Mapping[str, str], base_dir: Path) -> Coverage | None:
    current = _read_coverage(_input_path(env, base_dir, "COVERAGE_SUMMARY"))
    if current is None:
        return None
    base_manifest = _manifest(_input_path(env, base_dir, "BASE_ARTIFACTS_DIR"))
    workflow = next(
        (str(e.get("workflow", "")) for e in base_manifest if e.get("name") == "coverage-report"),
        "",
    )
    baseline = _read_coverage(_input_path(env, base_dir, "BASE_COVERAGE_SUMMARY"))
    return Coverage(current=current, baseline=baseline, base_workflow=clip(workflow))


def load_base(env: Mapping[str, str], base_dir: Path) -> Base | None:
    sha = _env(env, "BASE_SHA")
    if not sha:
        return None
    data = _load_json(_input_path(env, base_dir, "BASE_SIZES_JSON"), "base builds")
    if not isinstance(data, dict) or data.get("head_sha") != sha:
        return Base(sha, None)
    runs = tuple(
        BaseRun(
            workflow=clip(run.get("workflow", "")),
            run_number=run.get("run_number"),
            status=str(run.get("status", "")),
            conclusion=str(run.get("conclusion", "")),
            url=str(run.get("url", "")),
        )
        for run in data.get("runs") or []
        if isinstance(run, dict)
    )
    return Base(sha, runs)


def _artifact_sizes(data: Any) -> list[dict[str, Any]]:
    if not isinstance(data, dict) or not isinstance(data.get("artifacts"), list):
        return []
    return [artifact for artifact in data["artifacts"] if isinstance(artifact, dict)]


def load_sizes(env: Mapping[str, str], base_dir: Path) -> list[ArtifactSize]:
    pr_data = _load_json(_input_path(env, base_dir, "PR_SIZES_JSON"), "PR sizes")
    base_sha = _env(env, "BASE_SHA")
    base_data = _load_json(_input_path(env, base_dir, "BASE_SIZES_JSON"), "base sizes")
    baseline: dict[str, int] = {}
    if isinstance(base_data, dict) and base_sha and base_data.get("head_sha") == base_sha:
        for artifact in _artifact_sizes(base_data):
            if name := str(artifact.get("name", "")).strip():
                baseline[name] = _int(artifact.get("size_bytes"))
    elif base_data is not None:
        logger.warning(
            "Ignoring base sizes for %s; PR base is %s",
            base_data.get("head_sha", "unknown revision") if isinstance(base_data, dict) else "?",
            base_sha or "unknown",
        )

    items: list[ArtifactSize] = []
    for artifact in _artifact_sizes(pr_data):
        name = str(artifact.get("name", "")).strip()
        try:
            size = int(artifact.get("size_bytes", 0))
        except (TypeError, ValueError):
            continue
        if name:
            items.append(
                ArtifactSize(
                    clip(name),
                    clip(artifact.get("workflow", "")),
                    size,
                    baseline.get(name),
                    run_id=_int(artifact.get("run_id")),
                    artifact_id=_int(artifact.get("artifact_id")),
                    expires=str(artifact.get("expires_at") or "")[:10],
                )
            )
    return items


def load_precommit(env: Mapping[str, str], base_dir: Path) -> Precommit | None:
    data = _load_json(_input_path(env, base_dir, "PRECOMMIT_RESULTS_PATH"), "pre-commit results")
    if not isinstance(data, dict):
        return None
    hooks = data.get("failed_hooks")
    return Precommit(
        passed=_int(data.get("passed")),
        failed=_int(data.get("failed")),
        skipped=_int(data.get("skipped")),
        failed_hooks=tuple(clip(hook) for hook in hooks[:20]) if isinstance(hooks, list) else (),
        run_url=str(data.get("run_url", "")),
    )


def load_analysis(analysis_dir: Path) -> list[AnalysisTool]:
    """Merge every shard's analyze.py *-timings.json by tool, with its warnings."""
    merged: dict[str, dict[str, list[str]]] = {}
    for path in sorted(analysis_dir.glob("**/*-timings.json")):
        data = _load_json(path, "analysis timings")
        if not isinstance(data, dict) or not isinstance(data.get("files"), list):
            continue
        tool = merged.setdefault(
            clip(data.get("tool") or path.name.removesuffix("-timings.json")),
            {"files": [], "findings": [], "errors": []},
        )
        for row in data["files"]:
            if not isinstance(row, dict):
                continue
            name = clip(row.get("file", ""))
            tool["files"].append(name)
            if row.get("status") == "findings":
                tool["findings"].append(name)
            elif row.get("status") == "error":
                tool["errors"].append(name)
    warnings, changed = load_unique_warnings(analysis_dir)
    return [
        AnalysisTool(
            name,
            tuple(sorted(set(rows["files"]))),
            tuple(sorted(set(rows["findings"]))),
            tuple(sorted(set(rows["errors"]))),
            warnings=len(warnings[name]) if name in warnings else None,
            changed=tuple(sorted(changed[name], key=lambda w: (w.file, w.line, w.column)))
            if name in changed
            else None,
        )
        for name, rows in sorted(merged.items())
    ]


def load_analysis_commit(analysis_dir: Path) -> str:
    """Return the commit that every ``*-diagnostics.json`` naming one analyzed, else ''.

    On a pull request Code Analysis checks out GitHub's merge commit, whose line numbers can
    differ from the PR head's where the base branch changed the same file.
    """
    commits = set()
    for path in sorted(analysis_dir.glob(f"**/*{DIAGNOSTICS_SUFFIX}")):
        data = _load_json(path, "analysis diagnostics")
        commit = data.get("commit") if isinstance(data, dict) else None
        if isinstance(commit, str) and re.fullmatch(r"[0-9a-f]{40}|[0-9a-f]{64}", commit):
            commits.add(commit)
    return commits.pop() if len(commits) == 1 else ""


def _manifest_entries(env: Mapping[str, str], base_dir: Path, *, expired: bool) -> list[str]:
    """Name the artifacts that weren't downloaded: failed downloads, or ``expired`` ones."""
    listed = []
    for key, suffix in (
        ("ARTIFACTS_DIR", ""),
        ("ANALYSIS_ARTIFACTS_DIR", ""),
        ("BASE_ARTIFACTS_DIR", ", base"),
    ):
        for entry in _manifest(_input_path(env, base_dir, key)):
            if entry.get("downloaded") is False and bool(entry.get("expired")) == expired:
                workflow = escape_text(clip(entry.get("workflow", "")))
                listed.append(f"{code_span(clip(entry.get('name', '')))} ({workflow}{suffix})")
    return listed


def collect_report(env: Mapping[str, str], base_dir: Path, now: datetime) -> Report:
    status = _load_json(_input_path(env, base_dir, "BUILD_STATUS_JSON"), "status")
    if not isinstance(status, dict):
        status = {}
    platforms = [entry for entry in status.get("platforms") or [] if isinstance(entry, dict)]
    others = [entry for entry in status.get("others") or [] if isinstance(entry, dict)]
    suites = load_suites(_input_path(env, base_dir, "ARTIFACTS_DIR"))
    base_suites = load_suites(_input_path(env, base_dir, "BASE_ARTIFACTS_DIR"))
    analysis_dir = _input_path(env, base_dir, "ANALYSIS_ARTIFACTS_DIR")
    return Report(
        platforms=tuple(platforms),
        precommit_run=status.get("precommit")
        if isinstance(status.get("precommit"), dict)
        else None,
        analysis_run=status.get("analysis") if isinstance(status.get("analysis"), dict) else None,
        others=tuple(others),
        suites=tuple(suites),
        test_changes=compare_tests(suites, base_suites),
        coverage=load_coverage(env, base_dir),
        sizes=tuple(load_sizes(env, base_dir)),
        base=load_base(env, base_dir),
        precommit=load_precommit(env, base_dir),
        analysis=tuple(load_analysis(analysis_dir)),
        missing_downloads=tuple(_manifest_entries(env, base_dir, expired=False)),
        expired_downloads=tuple(_manifest_entries(env, base_dir, expired=True)),
        repo=_env(env, "REPO", _env(env, "GITHUB_REPOSITORY", str(status.get("repo", "")))),
        server_url=_env(env, "SERVER_URL", _env(env, "GITHUB_SERVER_URL", "https://github.com")),
        commit=_env(env, "COMMIT_SHA", str(status.get("head_sha", ""))),
        analysis_commit=load_analysis_commit(analysis_dir),
        pr_number=_env(env, "PR_NUMBER"),
        triggered_by=_env(env, "TRIGGERED_BY", str(triggering_run(env).get("name") or "unknown")),
        now=now,
    )


# ---------------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------------


def _state(entry: dict[str, Any] | None) -> str:
    return str(entry.get("state", "pending")) if entry else "not_run"


def _commit_link(report: Report, sha: str) -> str:
    short = sha[:7]
    if not short:
        return ""
    if not report.repo:
        return code_span(short)
    return _link(code_span(short), f"{report.server_url}/{report.repo}/commit/{sha}")


def render_verdict(report: Report) -> str:
    states = [_state(entry) for entry in report.platforms]
    gating = [*states, _state(report.precommit_run)]
    if report.analysis_run:
        gating.append(_state(report.analysis_run))
    failing = [
        name
        for name, state in [
            *((entry.get("name", ""), _state(entry)) for entry in report.platforms),
            ("pre-commit", _state(report.precommit_run)),
            ("Code Analysis", _state(report.analysis_run)),
            *((entry.get("name", ""), _state(entry)) for entry in report.others),
        ]
        if state == "failure"
    ]
    pending = sum(state in _PENDING for state in states)
    skipped = sum(state == "skipped" for state in states)
    others_pending = sum(_state(entry) in _PENDING for entry in report.others)

    if failing:
        names = ", ".join(escape_text(clip(name)) for name in failing)
        verdict = f"❌ **{_plural(len(failing), 'workflow')} failed** ({names})"
    elif any(state in _PENDING for state in gating):
        verdict = "⏳ **Builds in progress**"
    elif "cancelled" in gating:
        verdict = "🚫 **Builds cancelled**"
    elif states and skipped == len(states):
        verdict = "⏭️ **No platform builds needed**"
    else:
        verdict = "✅ **All builds passed**"

    parts = [verdict, _commit_link(report, report.commit)]
    if states:
        progress = f"{len(states) - pending} of {len(states)} platform workflows done"
        extras = []
        if skipped:
            extras.append(f"{skipped} skipped by change detection")
        if pending:
            extras.append(f"{pending} pending")
        if extras:
            progress += f" ({', '.join(extras)})"
        parts.append(progress)
    if others_pending:
        parts.append(f"{_plural(others_pending, 'other check')} still running")
    return " · ".join(part for part in parts if part)


def _suites_for(report: Report, workflow: str) -> list[Suite]:
    return [suite for suite in report.suites if suite.workflow == workflow]


def _counts(passed: int, failed: int, skipped: int, noun: str = "") -> str:
    parts = [f"{format_count(passed)} passed"]
    if failed:
        parts.append(f"**{format_count(failed)} failed**")
    if skipped:
        parts.append(f"{format_count(skipped)} skipped")
    return f"{noun}{', '.join(parts)}"


def _duration_cell(entry: dict[str, Any] | None) -> str:
    if not entry:
        return "—"
    seconds = entry.get("duration_seconds")
    if not isinstance(seconds, (int, float)):
        return "running" if entry.get("state") == "running" else "—"
    duration = format_duration(seconds)
    attempt = _int(entry.get("attempt"))
    if attempt > 1:
        elapsed = entry.get("elapsed_seconds")
        total = f", {format_duration(elapsed)} total" if isinstance(elapsed, (int, float)) else ""
        duration += f" (attempt {attempt}{total})"
    return duration


def _run_cell(entry: dict[str, Any] | None) -> str:
    if not entry or not entry.get("url"):
        return "—"
    number = entry.get("run_number")
    return _link(f"#{number}" if number else "run", str(entry["url"]))


def _size_cell(report: Report, workflow: str) -> str:
    deltas = [item for item in report.sizes if item.workflow == workflow and item.delta is not None]
    if not deltas:
        return "—"
    flag = "⚠️ " if any(item.flagged for item in deltas) else ""
    return f"{flag}{_delta_size(sum(item.delta or 0 for item in deltas))}"


def _analysis_results(report: Report) -> str:
    changed = [tool for tool in report.analysis if tool.changed is not None]
    if changed:
        count = sum(len(tool.changed or ()) for tool in changed)
        if not count:
            return "no warnings on changed lines"
        return f"**{_plural(count, 'warning')} on changed lines**"
    if report.analysis:
        files = len({name for tool in report.analysis for name in tool.files})
        return f"{format_count(files)} files analyzed"
    return "—"


def render_table(report: Report) -> str:
    rows: list[list[str]] = []
    for entry in report.platforms:
        name = str(entry.get("name", ""))
        suites = _suites_for(report, name)
        tests = (
            _counts(
                sum(s.count("passed") for s in suites),
                sum(s.count("failed") for s in suites),
                sum(s.count("skipped") for s in suites),
            )
            if suites
            else "—"
        )
        state = _state(entry)
        label = STATE_LABELS.get(state, state)
        if state == "skipped":
            label += " (no changes)"
        rows.append(
            [
                escape_text(clip(name)),
                label,
                _duration_cell(entry),
                tests,
                _size_cell(report, name),
                _run_cell(entry),
            ]
        )
    hooks = report.precommit
    rows.append(
        [
            "pre-commit",
            STATE_LABELS.get(_state(report.precommit_run), "—"),
            _duration_cell(report.precommit_run),
            _counts(hooks.passed, hooks.failed, hooks.skipped, "hooks: ") if hooks else "—",
            "—",
            _run_cell(report.precommit_run),
        ]
    )
    if report.analysis_run:
        rows.append(
            [
                "Code Analysis",
                STATE_LABELS.get(_state(report.analysis_run), "—"),
                _duration_cell(report.analysis_run),
                _analysis_results(report),
                "—",
                _run_cell(report.analysis_run),
            ]
        )
    return md_table(
        ["Workflow", "Status", "Duration", "Results", "Size Δ", "Run"],
        rows,
        markdown=True,
    )


_OTHER_ORDER = (
    "failure",
    "cancelled",
    "action_required",
    "running",
    "pending",
    "success",
    "skipped",
)


def render_others(report: Report) -> str:
    """One line for every other workflow of the commit: problems linked first, passes by name."""
    if not report.others:
        return ""
    by_state: dict[str, list[dict[str, Any]]] = {}
    for entry in report.others:
        by_state.setdefault(_state(entry), []).append(entry)
    groups = []
    for state in sorted(
        by_state, key=lambda s: _OTHER_ORDER.index(s) if s in _OTHER_ORDER else len(_OTHER_ORDER)
    ):
        icon = _STATE_ICONS.get(state, "❔")
        names = [escape_text(clip(entry.get("name", ""))) for entry in by_state[state]]
        if state in {"success", "skipped"}:
            groups.append(f"{icon} {', '.join(names)}")
        else:
            links = [
                _link(name, str(entry.get("url", "")))
                for name, entry in zip(names, by_state[state], strict=True)
            ]
            groups.append(" · ".join(f"{icon} {link}" for link in links))
    return "**Other checks:** " + " · ".join(groups)


def _base_run_text(run: BaseRun) -> str:
    text = escape_text(run.workflow)
    if run.url:
        text += " " + _link(f"#{run.run_number}" if run.run_number else "run", run.url)
    if run.status != "completed":
        return f"{text} (still running)"
    if run.conclusion not in {"success", ""}:
        return f"{text} ({escape_text(run.conclusion.replace('_', ' '))})"
    return text


def render_base(report: Report) -> str:
    """Name the base commit and the runs whose artifacts the changes are measured against."""
    base = report.base
    if base is None:
        return ""
    commit = _commit_link(report, base.sha)
    if base.runs is None:
        return f"**Base:** {commit}: its builds could not be read, so changes are not shown."
    if not base.runs:
        return f"**Base:** {commit} has no builds of its own yet, so changes are not shown."
    listed = {run.workflow for run in base.runs}
    parts = [_base_run_text(run) for run in base.runs]
    parts += [
        f"{escape_text(clip(name))}: no build"
        for entry in report.platforms
        if (name := str(entry.get("name", ""))) and name not in listed
    ]
    return f"**Base:** {commit} · " + " · ".join(parts)


def render_missing_downloads(report: Report) -> str:
    notes = []
    if report.missing_downloads:
        notes.append(
            "⚠️ Could not download "
            + ", ".join(report.missing_downloads)
            + "; the results below leave them out."
        )
    if report.expired_downloads:
        notes.append(
            "⌛ Past artifact retention: "
            + ", ".join(report.expired_downloads)
            + "; comparisons with them are left out."
        )
    return "\n\n".join(notes)


def _failed_jobs(entry: dict[str, Any]) -> list[dict[str, Any]]:
    jobs = [
        job
        for job in entry.get("jobs") or []
        if isinstance(job, dict) and job.get("conclusion") in _FAILED_JOB_CONCLUSIONS
    ]
    # The <workflow>-ok gate fails with every leg; list it only when nothing else failed.
    legs = [job for job in jobs if job.get("name") != entry.get("name")]
    return legs or jobs


def _job_line(workflow: str, job: dict[str, Any]) -> str:
    url = str(job.get("url", ""))
    steps = job.get("failed_steps") or []
    lost = job.get("runner_lost")
    if isinstance(lost, dict):
        times = [
            f"{label} {clock}"
            for label, key in (("started", "started_at"), ("ended", "completed_at"))
            if (clock := _clock(str(lost.get(key, ""))))
        ]
        step = code_span(clip(lost.get("step", ""))) if lost.get("step") else "a step"
        span = f" ({', '.join(times)})" if times else ""
        what = f"runner lost during {step}{span}, no logs; re-run"
    elif steps:
        failed = ", ".join(
            _link(code_span(clip(step.get("name", ""))), str(step.get("url", "")))
            for step in steps[:3]
        )
        what = f"failed at {failed}"
    else:
        what = str(job.get("conclusion", "failed")).replace("_", " ")
    log = _link("log", url) if url else ""
    line = f"- **{escape_text(clip(workflow))}** · {escape_text(clip(job.get('name', '')))}: {what}"
    return f"{line} · {log}" if log else line


def _excerpt_block(case: JUnitCase, limits: Limits) -> str:
    lines = [line[: limits.line_chars] for line in case.excerpt[: limits.excerpt_lines]]
    return code_block("\n".join(lines) or "(no output captured)", "text")


def render_failures(report: Report, limits: Limits) -> str:
    job_lines: list[str] = []
    for entry in [*report.platforms, report.precommit_run, report.analysis_run, *report.others]:
        if entry and _state(entry) == "failure":
            name = str(entry.get("name", ""))
            jobs = _failed_jobs(entry)
            if name == "pre-commit" and report.precommit and report.precommit.failed_hooks:
                hooks = ", ".join(code_span(hook) for hook in report.precommit.failed_hooks[:10])
                url = str(jobs[0].get("url", "")) if jobs else str(entry.get("url", ""))
                job_lines.append(f"- **pre-commit** · failed hooks: {hooks} · {_link('log', url)}")
                continue
            if not jobs:
                job_lines.append(
                    f"- **{escape_text(clip(name))}**: failed · "
                    f"{_link('run', str(entry.get('url', '')))}"
                )
            job_lines += [_job_line(name, job) for job in jobs]
    hidden_jobs = max(0, len(job_lines) - limits.failed_jobs)
    job_lines = job_lines[: limits.failed_jobs]
    if hidden_jobs:
        job_lines.append(f"- {more_items(hidden_jobs)}")

    failed_tests = [
        case for suite in report.suites for case in suite.cases if case.outcome == "failed"
    ]
    if not job_lines and not failed_tests:
        return ""
    parts = ["### ❌ Failures"]
    if job_lines:
        parts.append("\n".join(job_lines))
    if failed_tests:
        total = sum(len(suite.cases) for suite in report.suites)
        parts.append(
            f"**Failed tests** ({format_count(len(failed_tests))} of {format_count(total)})"
        )
        shown = failed_tests[: limits.failed_tests]
        if limits.excerpt_lines:
            parts += [
                details(
                    f"❌ {escape_text(case.name)} · {escape_text(case.suite)}",
                    _excerpt_block(case, limits),
                )
                for case in shown
            ]
        else:
            parts.append(
                "\n".join(f"- {code_span(case.name)} · {escape_text(case.suite)}" for case in shown)
            )
        if len(failed_tests) > len(shown):
            parts.append(more_items(len(failed_tests) - len(shown)))
    return "\n\n".join(parts)


def _suite_change(changes: TestChanges | None, suite: Suite) -> str:
    if changes is None or suite.name not in changes.compared:
        return "—"
    added = sum(suite.name in found for found in changes.added.values())
    removed = sum(suite.name in found for found in changes.removed.values())
    return f"+{added} -{removed}" if added or removed else "same"


def _test_name_lines(label: str, names: dict[str, tuple[str, ...]], limit: int) -> str:
    if not names:
        return ""
    shown = [
        f"{code_span(name)} ({', '.join(escape_text(suite) for suite in suites)})"
        for name, suites in list(names.items())[:limit]
    ]
    if len(names) > limit:
        shown.append(more_items(len(names) - limit))
    return f"{label}: " + "; ".join(shown)


def render_tests(report: Report, limits: Limits) -> str:
    if not report.suites:
        return ""
    changes = report.test_changes
    rows = [
        [
            code_span(suite.name),
            escape_text(suite.workflow) or "—",
            format_count(suite.count("passed")),
            f"**{format_count(suite.count('failed'))}**" if suite.count("failed") else "0",
            format_count(suite.count("skipped")),
            format_duration(
                suite.wall_seconds
                if suite.wall_seconds is not None
                else sum(case.seconds for case in suite.cases)
            ),
            _suite_change(changes, suite),
        ]
        for suite in sorted(
            report.suites, key=lambda suite: (not suite.count("failed"), suite.name)
        )[: limits.suites]
    ]
    body = md_table(
        ["Suite", "Workflow", "Passed", "Failed", "Skipped", "Time", "Tests vs base"],
        rows,
        align=["left", "left", "right", "right", "right", "right", "right"],
        markdown=True,
    )
    if len(report.suites) > limits.suites:
        body += f"\n\n{more_items(len(report.suites) - limits.suites)}"
    passed = sum(suite.count("passed") for suite in report.suites)
    failed = sum(suite.count("failed") for suite in report.suites)
    skipped = sum(suite.count("skipped") for suite in report.suites)
    summary = (
        f"Tests: {format_count(passed)} passed, {format_count(failed)} failed, "
        f"{format_count(skipped)} skipped in {_plural(len(report.suites), 'suite')}"
    )
    if changes is not None and changes.compared:
        summary += (
            f"; {format_count(len(changes.added))} added, "
            f"{format_count(len(changes.removed))} removed vs base"
        )
        for line in (
            _test_name_lines("Added", changes.added, limits.test_names),
            _test_name_lines("Removed", changes.removed, limits.test_names),
        ):
            if line:
                body += f"\n\n{line}"
    elif report.base is not None:
        body += "\n\nThe base build has no test results to compare with."
    return details(summary, body)


def _coverage_metrics(summary: CoverageSummary) -> list[tuple[str, int, int]]:
    metrics = [("Lines", summary.line_covered, summary.line_total)]
    if summary.function_total:
        metrics.append(("Functions", summary.function_covered, summary.function_total))
    if summary.branch_total:
        metrics.append(("Branches", summary.branch_covered, summary.branch_total))
    return metrics


def _pct(covered: int, total: int) -> float:
    return 100.0 * covered / total if total else 0.0


def render_coverage(report: Report) -> str:
    coverage = report.coverage
    if not coverage:
        return ""
    baseline = (
        {name: (c, t) for name, c, t in _coverage_metrics(coverage.baseline)}
        if coverage.baseline
        else {}
    )
    rows = []
    line_delta = ""
    for name, covered, total in _coverage_metrics(coverage.current):
        current = _pct(covered, total)
        base = baseline.get(name)
        change = "—"
        if base and base[1]:
            change = f"{current - _pct(*base):+.2f} pp"
            if name == "Lines":
                line_delta = f" ({change} vs base)"
        rows.append(
            [
                name,
                f"{format_count(covered)} of {format_count(total)}",
                format_percent(current),
                format_percent(_pct(*base)) if base and base[1] else "—",
                change,
            ]
        )
    body = md_table(
        ["Metric", "Covered", "Coverage", "Base", "Change"],
        rows,
        align=["left", "right", "right", "right", "right"],
    )
    notes = [f"Counts {code_span(COVERAGE_PREFIX)} only; test code is left out."]
    base_sha = code_span(report.base.sha[:7]) if report.base else "the PR base"
    if coverage.baseline:
        workflow = (
            f" ({escape_text(coverage.base_workflow)} build)" if coverage.base_workflow else ""
        )
        notes.append(f"Base: {base_sha}{workflow}.")
    else:
        notes.append(f"No coverage report for base {base_sha}, so changes are not shown.")
    if report.repo and report.pr_number.isdigit():
        codecov = f"https://app.codecov.io/gh/{report.repo}/pull/{report.pr_number}"
        notes.append(f"Coverage of the changed lines: {_link('Codecov', codecov)}.")
    summary = (
        f"Code coverage ({COVERAGE_PREFIX}): "
        f"{format_percent(_pct(coverage.current.line_covered, coverage.current.line_total))}"
        f" of lines{line_delta}"
    )
    return details(summary, f"{body}\n\n{' '.join(notes)}")


def _artifact_cell(report: Report, item: ArtifactSize) -> str:
    flag = "⚠️ " if item.flagged else ""
    name = code_span(item.name)
    if report.repo and item.run_id > 0 and item.artifact_id > 0:
        name = _link(
            name,
            f"{report.server_url}/{report.repo}/actions/runs/{item.run_id}"
            f"/artifacts/{item.artifact_id}",
        )
    return f"{flag}{name}"


def render_sizes(report: Report, limits: Limits) -> str:
    if not report.sizes:
        return ""
    ordered = sorted(report.sizes, key=lambda item: (not item.flagged, item.name))
    rows = [
        [
            _artifact_cell(report, item),
            escape_text(item.workflow) or "—",
            format_bytes(item.size),
            _delta_size(item.delta) if item.delta is not None else "—",
            f"{item.percent:+.2f}%" if item.percent is not None else "—",
            escape_text(item.expires) or "—",
        ]
        for item in ordered[: limits.artifacts]
    ]
    body = md_table(
        ["Artifact", "Workflow", "Size", "Change", "%", "Expires"],
        rows,
        align=["left", "left", "right", "right", "right", "right"],
        markdown=True,
    )
    if len(ordered) > limits.artifacts:
        body += f"\n\n{more_items(len(ordered) - limits.artifacts)}"
    base = code_span(report.base.sha[:7]) if report.base else "the PR base"
    if any(item.delta is not None for item in report.sizes):
        note = (
            f"Changes are against base {base}; only artifacts with the same name are compared "
            f"(PR Android builds are single-ABI). ⚠️ marks growth above {SIZE_WARN_PERCENT:g}% "
            f"and {format_bytes(SIZE_WARN_MIN_BYTES)}."
        )
    else:
        note = f"No artifacts of base {base} to compare with."
    note += " Downloading needs a GitHub sign-in."
    flagged = sum(item.flagged for item in report.sizes)
    expiry = min((item.expires for item in report.sizes if item.expires), default="")
    summary = f"Artifacts: {_plural(len(report.sizes), 'download')}"
    if expiry:
        summary += f", first expires {escape_text(expiry)}"
    if flagged:
        summary = f"⚠️ {summary}; {format_count(flagged)} grew"
    return details(summary, f"{body}\n\n{note}")


def render_precommit(report: Report) -> str:
    hooks = report.precommit
    if not hooks or not (hooks.failed or hooks.failed_hooks):
        return ""
    rows = [["Passed", hooks.passed], ["Failed", hooks.failed], ["Skipped", hooks.skipped]]
    body = md_table(["Hooks", "Count"], rows, align=["left", "right"])
    if hooks.failed_hooks:
        body += "\n\nFailed: " + ", ".join(code_span(hook) for hook in hooks.failed_hooks)
    url = hooks.run_url or str((report.precommit_run or {}).get("url", ""))
    if _sanitize_external_url(url):
        body += f"\n\n{_link('Hook output', url)} · reproduce locally with `just lint`."
    return details(
        f"Pre-commit: {hooks.passed} passed, {hooks.failed} failed, {hooks.skipped} skipped",
        body,
    )


def _warning_link(report: Report, warning: AnalysisWarning) -> str:
    location = code_span(f"{warning.file}:{warning.line}")
    commit = report.analysis_commit or report.commit
    if not report.repo or not commit or warning.file.startswith("build/"):
        return location
    return _link(
        location, f"{report.server_url}/{report.repo}/blob/{commit}/{warning.file}#L{warning.line}"
    )


def render_analysis(report: Report, limits: Limits) -> str:
    run = report.analysis_run
    if not run:
        return ""
    state = _state(run)
    url = str(run.get("url", ""))
    tools = report.analysis
    changed_tools = [tool for tool in tools if tool.changed is not None]
    changed = sorted(
        (warning for tool in changed_tools for warning in tool.changed or ()),
        key=lambda w: (w.file, w.line, w.column, w.tool),
    )
    parts: list[str] = []
    if changed:
        listed = [
            f"- {_warning_link(report, warning)} {code_span(warning.check)} ({escape_text(warning.tool)})"
            for warning in changed[: limits.analysis_warnings]
        ]
        if len(changed) > limits.analysis_warnings:
            listed.append(f"- {more_items(len(changed) - limits.analysis_warnings)}")
        parts.append("Warnings on lines this pull request changed:\n\n" + "\n".join(listed))
    elif changed_tools:
        parts.append("No warnings on lines this pull request changed.")
    if tools:
        has_warnings = any(tool.warnings is not None for tool in tools)
        headers = ["Tool", "Files analyzed"]
        if changed_tools:
            headers.append("On changed lines")
        headers += ["All warnings" if has_warnings else "With findings", "Errors"]
        rows = []
        for tool in tools:
            row = [code_span(tool.tool), format_count(len(tool.files))]
            if changed_tools:
                row.append("—" if tool.changed is None else format_count(len(tool.changed)))
            if has_warnings:
                row.append("—" if tool.warnings is None else format_count(tool.warnings))
            else:
                row.append(format_count(len(tool.findings)))
            row.append(format_count(len(tool.errors)))
            rows.append(row)
        table = md_table(
            headers, rows, align=["left", *["right"] * (len(headers) - 1)], markdown=True
        )
        note = (
            "All warnings counts each unique warning in every analyzed unit and the headers it "
            "includes, mostly code this pull request did not touch."
            if has_warnings
            else "Warnings are advisory."
        )
        parts.append(f"{table}\n\n{note}")
    elif state in _PENDING:
        parts.append(
            "Still running when this comment was updated; findings appear once it completes."
        )
    else:
        parts.append("No analysis results were downloaded.")
    parts.append(
        f"{_link('Code Analysis run', url)} (diagnostics are in each job's summary and log)."
    )
    title = f"Code Analysis: {STATE_LABELS.get(state, state)}"
    if changed_tools:
        title += (
            f", {_plural(len(changed), 'warning')} on changed lines"
            if changed
            else ", no warnings on changed lines"
        )
    return details(title, "\n\n".join(parts))


def render_footer(report: Report) -> str:
    parts = [f"Updated {report.now.strftime('%Y-%m-%d %H:%M UTC')}"]
    if commit := _commit_link(report, report.commit):
        parts.append(f"for {commit}")
    parts.append(f"triggered by {escape_text(report.triggered_by)}")
    if report.repo and report.pr_number.isdigit():
        parts.append(
            _link("all checks", f"{report.server_url}/{report.repo}/pull/{report.pr_number}/checks")
        )
    return f"<sub>{' · '.join(parts)}</sub>"


def render(report: Report, limits: Limits) -> str:
    sections = [
        "## Build Results",
        render_verdict(report),
        render_table(report),
        render_others(report),
        render_base(report),
        render_missing_downloads(report),
        render_failures(report, limits),
        render_tests(report, limits),
        render_coverage(report),
        render_sizes(report, limits),
        render_precommit(report),
        render_analysis(report, limits),
        "---",
        render_footer(report),
    ]
    return "\n\n".join(section for section in sections if section) + "\n"


def generate_comment(
    env: Mapping[str, str], base_dir: Path, now_utc: datetime | None = None
) -> str:
    report = collect_report(env, base_dir, now_utc or datetime.now(timezone.utc))
    comment = ""
    for limits in LIMIT_STEPS:
        comment = render(report, limits)
        if len(comment) <= MAX_COMMENT_CHARS:
            return comment
    notice = "\n\n…comment truncated to fit GitHub's size limit; see the workflow runs.\n"
    return comment[: MAX_COMMENT_CHARS - len(notice)] + notice


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate Build Results PR comment markdown.",
    )
    parser.add_argument(
        "--base-dir",
        type=Path,
        default=Path.cwd(),
        help="Base directory for resolving relative paths (default: cwd)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Output path for comment markdown (default: $COMMENT_OUTPUT or comment.md)",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    base_dir = args.base_dir
    comment = generate_comment(os.environ, base_dir)
    if args.output:
        output_path = args.output
    else:
        output_path = base_dir / _env(os.environ, "COMMENT_OUTPUT", "comment.md")
    output_path.write_text(comment, encoding="utf-8")
    write_step_summary(comment)
    print(comment, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
