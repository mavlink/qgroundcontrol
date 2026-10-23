#!/usr/bin/env python3
"""Generate the Build Results PR comment.

Reads the workflow status that collect_build_status.py wrote (--status-out), the downloaded
test results, coverage, pre-commit, and Code Analysis artifacts, and the artifact sizes, from
the INPUT_PATHS that build-results.yml's steps write. The comment leads with a verdict and a
per-workflow table, lists failures, and collapses the rest.
"""

from __future__ import annotations

import argparse
import logging
import os
import re
import sys
from collections import Counter
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import TYPE_CHECKING, Any
from urllib.parse import quote, urlsplit, urlunsplit

if TYPE_CHECKING:
    from collections.abc import Mapping

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.coverage_summary import CoverageSummary, CoverageSummaryError, read_gcovr_summary
from common.format import (
    format_bytes,
    format_count,
    format_delta_bytes,
    format_duration,
    format_percent,
)
from common.gh_actions import write_step_summary
from common.io import read_json
from common.markdown import code_block, code_span, details, escape_text, md_table, more_items
from common.xml import XMLParseError

from ci.reporting.junit_report import JUnitCase, clip, parse_junit
from ci.reporting.workflow_runs import triggering_run

logger = logging.getLogger(__name__)

# GitHub rejects comments over 65,536 characters; the margin covers the upsert action's tag.
MAX_COMMENT_CHARS = 60_000
# An artifact that grows by more than SIZE_WARN_PERCENT of its base size, and by at least
# SIZE_WARN_MIN_BYTES, is flagged.
SIZE_WARN_PERCENT = 1.0
SIZE_WARN_MIN_BYTES = 1024 * 1024

STATE_LABELS = {
    "success": "✅ Passed",
    "failure": "❌ Failed",
    "interrupted": "🔁 Retrying",
    "cancelled": "🚫 Cancelled",
    "running": "🔄 Running",
    "pending": "⏳ Pending",
    "skipped": "⏭️ Skipped",
    "action_required": "⏸️ Awaiting approval",
    "not_run": "⚪ Not run",
}
_PENDING = {"pending", "running", "action_required"}
_FAILED_JOB_CONCLUSIONS = {"failure", "timed_out", "startup_failure"}
_BASELINE_KEY_SHA = re.compile(r"^build-baseline-v\d+-([0-9a-f]{7,40})(?:-|$)")
# Inputs relative to --base-dir, where build-results.yml's steps write them; an environment
# variable of the same name overrides each.
INPUT_PATHS = {
    "BUILD_STATUS_JSON": "build-status.json",
    "ARTIFACTS_DIR": "artifacts",
    "ANALYSIS_ARTIFACTS_DIR": "analysis-artifacts",
    "PRECOMMIT_RESULTS_PATH": "artifacts/pre-commit-results/pre-commit-results.json",
    "COVERAGE_SUMMARY": "artifacts/coverage-report/coverage-summary.json",
    "BASELINE_COVERAGE_SUMMARY": "baseline-coverage-summary.json",
    "PR_SIZES_JSON": "pr-sizes.json",
    "BASELINE_SIZES_JSON": "baseline-sizes.json",
}


@dataclass(frozen=True)
class Limits:
    """How much of each list the comment shows; smaller sets keep a large report under the cap."""

    failed_jobs: int = 10
    failed_tests: int = 10
    excerpt_lines: int = 8
    line_chars: int = 240
    suites: int = 30
    slowest: int = 5
    artifacts: int = 30
    analysis_files: int = 10


LIMIT_STEPS = (
    Limits(),
    Limits(failed_tests=5, excerpt_lines=4, suites=12, artifacts=12, analysis_files=5),
    Limits(
        failed_jobs=5,
        failed_tests=3,
        excerpt_lines=0,
        suites=6,
        slowest=3,
        artifacts=6,
        analysis_files=0,
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


@dataclass(frozen=True)
class ArtifactSize:
    name: str
    workflow: str
    size: int
    base: int | None

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
class Sizes:
    artifacts: tuple[ArtifactSize, ...]
    base_sha: str
    compared: bool


@dataclass(frozen=True)
class Coverage:
    current: CoverageSummary
    baseline: CoverageSummary | None
    baseline_sha: str
    baseline_is_base: bool


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
    # Unique warnings across shards, by file and by check; None without diagnostics files.
    warnings: int | None = None
    warning_files: tuple[tuple[str, int], ...] = ()
    warning_checks: tuple[tuple[str, int], ...] = ()


@dataclass(frozen=True)
class Report:
    platforms: tuple[dict[str, Any], ...]
    precommit_run: dict[str, Any] | None
    analysis_run: dict[str, Any] | None
    suites: tuple[Suite, ...]
    coverage: Coverage | None
    sizes: Sizes | None
    precommit: Precommit | None
    analysis: tuple[AnalysisTool, ...]
    repo: str
    server_url: str
    commit: str
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


def _manifest(artifacts_dir: Path) -> dict[str, str]:
    data = _load_json(artifacts_dir / "download-manifest.json", "the download manifest")
    if not isinstance(data, list):
        return {}
    return {
        str(entry.get("name", "")): clip(entry.get("workflow", ""))
        for entry in data
        if isinstance(entry, dict)
    }


def load_suites(artifacts_dir: Path) -> list[Suite]:
    """Return one suite per downloaded test-results-* artifact with readable JUnit XML."""
    workflows = _manifest(artifacts_dir)
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


def _baseline_sha(key: str) -> str:
    match = _BASELINE_KEY_SHA.match(key.strip())
    return match.group(1) if match else ""


def load_coverage(env: Mapping[str, str], base_dir: Path) -> Coverage | None:
    current_path = _input_path(env, base_dir, "COVERAGE_SUMMARY")
    if not current_path.is_file():
        return None
    try:
        current = read_gcovr_summary(current_path)
    except CoverageSummaryError:
        logger.warning("Failed to parse coverage from %s", current_path, exc_info=True)
        return None
    if current.line_total == 0:
        return None
    baseline_path = _input_path(env, base_dir, "BASELINE_COVERAGE_SUMMARY")
    baseline = None
    if baseline_path.is_file():
        try:
            baseline = read_gcovr_summary(baseline_path)
        except CoverageSummaryError:
            logger.warning("Failed to parse coverage from %s", baseline_path, exc_info=True)
    sha = _baseline_sha(_env(env, "BASELINE_KEY")) if baseline else ""
    base_sha = _env(env, "BASELINE_SHA")
    return Coverage(
        current=current,
        baseline=baseline,
        baseline_sha=sha,
        baseline_is_base=bool(sha and base_sha.startswith(sha)),
    )


def load_sizes(env: Mapping[str, str], base_dir: Path) -> Sizes | None:
    pr_data = _load_json(_input_path(env, base_dir, "PR_SIZES_JSON"), "PR sizes")
    if not isinstance(pr_data, dict) or not isinstance(pr_data.get("artifacts"), list):
        return None

    base_sha = _env(env, "BASELINE_SHA")
    baseline: dict[str, int] = {}
    baseline_data = _load_json(_input_path(env, base_dir, "BASELINE_SIZES_JSON"), "baseline sizes")
    if isinstance(baseline_data, dict):
        if base_sha and baseline_data.get("head_sha") != base_sha:
            logger.warning(
                "Ignoring size baseline for %s; PR base is %s",
                baseline_data.get("head_sha", "unknown revision"),
                base_sha,
            )
        else:
            for artifact in baseline_data.get("artifacts") or []:
                if isinstance(artifact, dict) and (name := str(artifact.get("name", "")).strip()):
                    baseline[name] = _int(artifact.get("size_bytes"))

    items: list[ArtifactSize] = []
    for artifact in pr_data["artifacts"]:
        if not isinstance(artifact, dict):
            continue
        name = str(artifact.get("name", "")).strip()
        try:
            size = int(artifact.get("size_bytes", 0))
        except (TypeError, ValueError):
            continue
        if name:
            items.append(
                ArtifactSize(
                    clip(name), clip(artifact.get("workflow", "")), size, baseline.get(name)
                )
            )
    if not items:
        return None
    return Sizes(tuple(items), base_sha[:7], compared=any(item.base is not None for item in items))


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


def _ranked(counts: Counter[str]) -> tuple[tuple[str, int], ...]:
    return tuple(sorted(counts.items(), key=lambda item: (-item[1], item[0])))


def load_analysis_warnings(analysis_dir: Path) -> dict[str, set[tuple[str, int, int, str]]]:
    """Union every shard's analyze.py *-diagnostics.json warnings by tool.

    A header's warnings appear in each shard that compiles a file including it, so a warning is
    identified by its location and check.
    """
    merged: dict[str, set[tuple[str, int, int, str]]] = {}
    for path in sorted(analysis_dir.glob("**/*-diagnostics.json")):
        data = _load_json(path, "analysis diagnostics")
        if not isinstance(data, dict) or not isinstance(data.get("diagnostics"), dict):
            continue
        warnings = merged.setdefault(
            clip(data.get("tool") or path.name.removesuffix("-diagnostics.json")), set()
        )
        for file, rows in data["diagnostics"].items():
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
                    warnings.add((clip(str(file)), row[0], row[1], clip(str(row[3]))))
    return merged


def load_analysis(analysis_dir: Path) -> list[AnalysisTool]:
    """Merge every shard's analyze.py *-timings.json by tool, with its unique warnings."""
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
    warnings = load_analysis_warnings(analysis_dir)
    tools = []
    for name, rows in sorted(merged.items()):
        tool_warnings = warnings.get(name)
        tools.append(
            AnalysisTool(
                name,
                tuple(sorted(set(rows["files"]))),
                tuple(sorted(set(rows["findings"]))),
                tuple(sorted(set(rows["errors"]))),
                warnings=None if tool_warnings is None else len(tool_warnings),
                warning_files=_ranked(Counter(file for file, *_ in tool_warnings or ())),
                warning_checks=_ranked(Counter(check for *_, check in tool_warnings or ())),
            )
        )
    return tools


def collect_report(env: Mapping[str, str], base_dir: Path, now: datetime) -> Report:
    status = _load_json(_input_path(env, base_dir, "BUILD_STATUS_JSON"), "status")
    if not isinstance(status, dict):
        status = {}
    artifacts_dir = _input_path(env, base_dir, "ARTIFACTS_DIR")
    analysis_dir = _input_path(env, base_dir, "ANALYSIS_ARTIFACTS_DIR")
    platforms = [entry for entry in status.get("platforms") or [] if isinstance(entry, dict)]
    return Report(
        platforms=tuple(platforms),
        precommit_run=status.get("precommit")
        if isinstance(status.get("precommit"), dict)
        else None,
        analysis_run=status.get("analysis") if isinstance(status.get("analysis"), dict) else None,
        suites=tuple(load_suites(artifacts_dir)),
        coverage=load_coverage(env, base_dir),
        sizes=load_sizes(env, base_dir),
        precommit=load_precommit(env, base_dir),
        analysis=tuple(load_analysis(analysis_dir)),
        repo=_env(env, "REPO", _env(env, "GITHUB_REPOSITORY", str(status.get("repo", "")))),
        server_url=_env(env, "SERVER_URL", _env(env, "GITHUB_SERVER_URL", "https://github.com")),
        commit=_env(env, "COMMIT_SHA", str(status.get("head_sha", ""))),
        pr_number=_env(env, "PR_NUMBER"),
        triggered_by=_env(env, "TRIGGERED_BY", str(triggering_run(env).get("name") or "unknown")),
        now=now,
    )


# ---------------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------------


def _state(entry: dict[str, Any] | None) -> str:
    return str(entry.get("state", "pending")) if entry else "not_run"


def _commit_link(report: Report) -> str:
    short = report.commit[:7]
    if not short:
        return ""
    if not report.repo:
        return code_span(short)
    return _link(code_span(short), f"{report.server_url}/{report.repo}/commit/{report.commit}")


def render_verdict(report: Report) -> str:
    states = [_state(entry) for entry in report.platforms]
    gating = [*states, _state(report.precommit_run)]
    failing = [
        name
        for name, state in [
            *((entry.get("name", ""), _state(entry)) for entry in report.platforms),
            ("pre-commit", _state(report.precommit_run)),
            ("Code Analysis", _state(report.analysis_run)),
        ]
        if state == "failure"
    ]
    pending = sum(state in _PENDING for state in states)
    retrying = sum(state == "interrupted" for state in states)
    skipped = sum(state == "skipped" for state in states)
    done = len(states) - pending - retrying

    if failing:
        verdict = f"❌ **{_plural(len(failing), 'workflow')} failed**"
    elif "interrupted" in [*gating, _state(report.analysis_run)]:
        verdict = "🔁 **Retrying after a Spot interruption**"
    elif any(state in _PENDING for state in gating):
        verdict = "⏳ **Builds in progress**"
    elif "cancelled" in gating:
        verdict = "🚫 **Builds cancelled**"
    elif states and skipped == len(states):
        verdict = "⏭️ **No platform builds needed**"
    else:
        verdict = "✅ **All builds passed**"

    progress = f"{done} of {len(states)} platform workflows done"
    extras = []
    if skipped:
        extras.append(f"{skipped} skipped by change detection")
    if pending:
        extras.append(f"{pending} pending")
    if retrying:
        extras.append(f"{retrying} retrying")
    if extras:
        progress += f" ({', '.join(extras)})"
    parts = [verdict, _commit_link(report), progress] if states else [verdict, _commit_link(report)]
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
    if isinstance(seconds, (int, float)):
        return format_duration(seconds)
    return "running" if entry.get("state") == "running" else "—"


def _run_cell(entry: dict[str, Any] | None) -> str:
    if not entry or not entry.get("url"):
        return "—"
    number = entry.get("run_number")
    return _link(f"#{number}" if number else "run", str(entry["url"]))


def _size_cell(report: Report, workflow: str) -> str:
    if not report.sizes:
        return "—"
    items = [item for item in report.sizes.artifacts if item.workflow == workflow]
    if not items:
        return "—"
    total = format_bytes(sum(item.size for item in items))
    deltas = [item.delta for item in items if item.delta is not None]
    if not deltas:
        return total
    flag = "⚠️ " if any(item.flagged for item in items) else ""
    return f"{flag}{total} ({format_delta_bytes(sum(deltas))})"


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
        files = len({name for tool in report.analysis for name in tool.files})
        findings = len({name for tool in report.analysis for name in tool.findings})
        warnings = _warning_totals(report.analysis)
        if warnings:
            results = (
                f"{format_count(warnings[0])} unique warnings in {format_count(warnings[1])} files"
            )
        elif report.analysis:
            results = f"{format_count(files)} files, {format_count(findings)} with findings"
        else:
            results = "—"
        rows.append(
            [
                "Code Analysis",
                STATE_LABELS.get(_state(report.analysis_run), "—"),
                _duration_cell(report.analysis_run),
                results,
                "—",
                _run_cell(report.analysis_run),
            ]
        )
    return md_table(
        ["Workflow", "Status", "Duration", "Results", "Artifacts (Δ vs base)", "Run"],
        rows,
        markdown=True,
    )


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
    if steps:
        failed = ", ".join(
            _link(code_span(clip(step.get("name", ""))), str(step.get("url", "")))
            for step in steps[:3]
        )
        what = f"failed at {failed}"
    else:
        what = str(job.get("conclusion", "failed")).replace("_", " ")
    if job.get("interrupted"):
        what = "AWS reclaimed its Spot instance"
    log = _link("log", url) if url else ""
    line = f"- **{escape_text(clip(workflow))}** · {escape_text(clip(job.get('name', '')))}: {what}"
    return f"{line} · {log}" if log else line


def _excerpt_block(case: JUnitCase, limits: Limits) -> str:
    lines = [line[: limits.line_chars] for line in case.excerpt[: limits.excerpt_lines]]
    return code_block("\n".join(lines) or "(no output captured)", "text")


def render_interruptions(report: Report) -> str:
    """Workflows whose only failures were Spot interruptions, which RunsOn re-runs."""
    lines = [
        _job_line(str(entry.get("name", "")), job)
        for entry in [*report.platforms, report.precommit_run, report.analysis_run]
        if entry and _state(entry) == "interrupted"
        for job in _failed_jobs(entry)
        if job.get("interrupted")
    ]
    if not lines:
        return ""
    return "\n\n".join(
        [
            "### 🔁 Spot interruptions",
            "\n".join(lines),
            "RunsOn re-runs these jobs once their workflow run finishes; this comment updates then.",
        ]
    )


def render_failures(report: Report, limits: Limits) -> str:
    job_lines: list[str] = []
    for entry in [*report.platforms, report.precommit_run, report.analysis_run]:
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
                    f"- **{escape_text(name)}**: failed · {_link('run', str(entry.get('url', '')))}"
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


def render_tests(report: Report, limits: Limits) -> str:
    if not report.suites:
        return ""
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
        ]
        for suite in sorted(
            report.suites, key=lambda suite: (not suite.count("failed"), suite.name)
        )[: limits.suites]
    ]
    body = md_table(
        ["Suite", "Workflow", "Passed", "Failed", "Skipped", "Time"],
        rows,
        align=["left", "left", "right", "right", "right", "right"],
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
    return details(summary, body)


def render_slowest(report: Report, limits: Limits) -> str:
    # A test that runs in several suites (coverage, sanitizers, macOS) is listed once, at its slowest.
    slowest: dict[str, JUnitCase] = {}
    for suite in report.suites:
        for case in suite.cases:
            if case.outcome == "skipped" or case.seconds <= 0:
                continue
            if case.name not in slowest or case.seconds > slowest[case.name].seconds:
                slowest[case.name] = case
    cases = sorted(slowest.values(), key=lambda case: case.seconds, reverse=True)[: limits.slowest]
    if not cases:
        return ""
    body = md_table(
        ["Test", "Suite", "Time"],
        [
            [code_span(case.name), escape_text(case.suite), format_duration(case.seconds)]
            for case in cases
        ],
        align=["left", "left", "right"],
        markdown=True,
    )
    return details(
        f"Slowest tests: {escape_text(cases[0].name)} took {format_duration(cases[0].seconds)}",
        body,
    )


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
        if base and base[1]:
            delta = current - _pct(*base)
            change = f"{delta:+.2f} pp"
            if name == "Lines":
                line_delta = f" ({change})"
            rows.append(
                [
                    name,
                    f"{format_count(covered)} of {format_count(total)}",
                    format_percent(current),
                    format_percent(_pct(*base)),
                    change,
                ]
            )
        else:
            rows.append(
                [
                    name,
                    f"{format_count(covered)} of {format_count(total)}",
                    format_percent(current),
                    "—",
                    "—",
                ]
            )
    body = md_table(
        ["Metric", "Covered", "Coverage", "Baseline", "Change"],
        rows,
        align=["left", "right", "right", "right", "right"],
    )
    if not coverage.baseline:
        note = (
            "No coverage baseline yet: `master` saves one after every platform workflow "
            "succeeds, so changes are shown once a baseline exists."
        )
    elif coverage.baseline_is_base:
        note = f"Baseline: PR base {code_span(coverage.baseline_sha[:7])}."
    elif coverage.baseline_sha:
        note = (
            f"Baseline: `master` at {code_span(coverage.baseline_sha[:7])}, the newest "
            "snapshot; the PR base has none."
        )
    else:
        note = "Baseline: the newest `master` snapshot."
    summary = f"Code coverage: {format_percent(_pct(coverage.current.line_covered, coverage.current.line_total))} of lines{line_delta}"
    return details(summary, f"{body}\n\n{note}")


def render_sizes(report: Report, limits: Limits) -> str:
    sizes = report.sizes
    if not sizes:
        return ""
    ordered = sorted(sizes.artifacts, key=lambda item: (not item.flagged, item.name))
    rows = []
    for item in ordered[: limits.artifacts]:
        delta = item.delta
        rows.append(
            [
                f"{'⚠️ ' if item.flagged else ''}{code_span(item.name)}",
                escape_text(item.workflow) or "—",
                format_bytes(item.size),
                format_delta_bytes(delta) if delta is not None else "—",
                f"{item.percent:+.2f}%" if item.percent is not None else "—",
            ]
        )
    body = md_table(
        ["Artifact", "Workflow", "Size", "Change", "%"],
        rows,
        align=["left", "left", "right", "right", "right"],
        markdown=True,
    )
    if len(ordered) > limits.artifacts:
        body += f"\n\n{more_items(len(ordered) - limits.artifacts)}"
    base = code_span(sizes.base_sha) if sizes.base_sha else "the PR base"
    if sizes.compared:
        note = (
            f"Compared with PR base {base}; only artifacts with the same name are compared "
            f"(PR Android builds are single-ABI). ⚠️ marks growth above {SIZE_WARN_PERCENT:g}% "
            f"and {format_bytes(SIZE_WARN_MIN_BYTES)}."
        )
    else:
        note = f"No size baseline for {base} yet, so changes are not shown."
    total = format_bytes(sum(item.size for item in sizes.artifacts))
    deltas = [item.delta for item in sizes.artifacts if item.delta is not None]
    change = f", {format_delta_bytes(sum(deltas))} vs base" if deltas else ""
    flag = "⚠️ " if any(item.flagged for item in sizes.artifacts) else ""
    summary = f"{flag}Artifact sizes: {_plural(len(sizes.artifacts), 'artifact')}, {total}{change}"
    return details(summary, f"{body}\n\n{note}")


def render_precommit(report: Report) -> str:
    hooks = report.precommit
    if not hooks:
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


def _warning_totals(tools: tuple[AnalysisTool, ...]) -> tuple[int, int] | None:
    """Unique warnings and the files they are in, across tools that wrote diagnostics."""
    reported = [tool for tool in tools if tool.warnings is not None]
    if not reported:
        return None
    files = {file for tool in reported for file, _ in tool.warning_files}
    return sum(tool.warnings or 0 for tool in reported), len(files)


def _analysis_warning_tables(tools: tuple[AnalysisTool, ...], limit: int) -> list[str]:
    reported = [tool for tool in tools if tool.warnings is not None]
    parts = [
        md_table(
            ["Tool", "Files analyzed", "Unique warnings", "In files", "Errors"],
            [
                [
                    code_span(tool.tool),
                    format_count(len(tool.files)),
                    "—" if tool.warnings is None else format_count(tool.warnings),
                    "—" if tool.warnings is None else format_count(len(tool.warning_files)),
                    format_count(len(tool.errors)),
                ]
                for tool in tools
            ],
            align=["left", "right", "right", "right", "right"],
            markdown=True,
        )
    ]
    if not limit:
        return parts
    checks = sorted(
        ((check, tool.tool, count) for tool in reported for check, count in tool.warning_checks),
        key=lambda row: (-row[2], row[0]),
    )
    if checks:
        rows = [
            [code_span(check), tool, format_count(count)] for check, tool, count in checks[:limit]
        ]
        parts.append(
            "Most frequent checks:\n\n"
            + md_table(
                ["Check", "Tool", "Unique warnings"],
                rows,
                align=["left", "left", "right"],
                markdown=True,
            )
        )
    per_file: Counter[str] = Counter()
    for tool in reported:
        per_file.update(dict(tool.warning_files))
    if per_file:
        rows = [[code_span(file), format_count(count)] for file, count in _ranked(per_file)[:limit]]
        parts.append(
            "Files with the most warnings (a header's warnings count once):\n\n"
            + md_table(["File", "Unique warnings"], rows, align=["left", "right"], markdown=True)
        )
    return parts


def render_analysis(report: Report, limits: Limits) -> str:
    run = report.analysis_run
    if not run:
        return ""
    state = _state(run)
    url = str(run.get("url", ""))
    parts: list[str] = []
    warnings = _warning_totals(report.analysis)
    if warnings:
        parts += _analysis_warning_tables(report.analysis, limits.analysis_files)
    elif report.analysis:
        parts.append(
            md_table(
                ["Tool", "Files analyzed", "With findings", "Errors"],
                [
                    [
                        code_span(tool.tool),
                        format_count(len(tool.files)),
                        format_count(len(tool.findings)),
                        format_count(len(tool.errors)),
                    ]
                    for tool in report.analysis
                ],
                align=["left", "right", "right", "right"],
                markdown=True,
            )
        )
        files = sorted(
            {name for tool in report.analysis for name in (*tool.errors, *tool.findings)}
        )
        if files and limits.analysis_files:
            listed = [f"- {code_span(name)}" for name in files[: limits.analysis_files]]
            if len(files) > limits.analysis_files:
                listed.append(f"- {more_items(len(files) - limits.analysis_files)}")
            parts.append("Files with findings (warnings are advisory):\n\n" + "\n".join(listed))
    elif state in _PENDING:
        parts.append(
            "Still running when this comment was updated; findings appear once it completes."
        )
    elif state == "interrupted":
        parts.append("Re-running after a Spot interruption; findings appear once it completes.")
    else:
        parts.append("No analysis results were downloaded.")
    parts.append(
        f"{_link('Code Analysis run', url)} (diagnostics are in each job's summary and log)."
    )
    title = f"Code Analysis: {STATE_LABELS.get(state, state)}"
    if warnings:
        title += f", {format_count(warnings[0])} unique warnings"
    return details(title, "\n\n".join(parts))


def render_footer(report: Report) -> str:
    parts = [f"Updated {report.now.strftime('%Y-%m-%d %H:%M UTC')}"]
    if commit := _commit_link(report):
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
        render_failures(report, limits),
        render_interruptions(report),
        render_tests(report, limits),
        render_slowest(report, limits),
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
