"""CI health history for the Pages dashboard: metrics of master push builds over time.

Each record holds one completed workflow run of a ``master`` push, keyed by (commit, workflow):
the run's result and wall time, its jobs' Build-step durations (jobs API), its distributable
artifacts' sizes, its JUnit suites' counts and durations with the failed tests, ``src/`` line
coverage, and Code Analysis's unique warnings per tool. The history is versioned JSON that the
site publishes at ``data/history.json`` and reads back on the next build, so it needs no data
branch: records accumulate and are dropped only by the commit-count and age caps.
"""

from __future__ import annotations

import json
import shutil
import subprocess
import time
import zipfile
from datetime import datetime, timedelta
from http.client import HTTPException
from pathlib import Path
from typing import TYPE_CHECKING, Any, NamedTuple
from urllib.error import HTTPError
from urllib.parse import urlencode

from common.analysis_diagnostics import DIAGNOSTICS_SUFFIX, load_unique_warnings
from common.coverage_summary import SUMMARY_FILENAME, CoverageSummaryError, read_gcovr_summary
from common.gh_actions import (
    download_artifact_zip,
    gh_api_json,
    gh_warning,
    list_run_artifacts,
    list_run_jobs,
    parse_github_timestamp,
    seconds_between,
)
from common.junit import parse_junit
from common.net import fetch_bytes
from common.xml import XMLParseError
from qgc_tools.build_artifacts import is_distributable_artifact

if TYPE_CHECKING:
    from collections.abc import Iterable

HISTORY_VERSION = 1
HISTORY_PATH = "data/history.json"
# Each build lists every workflow's latest runs and collects those not yet recorded, so a missed
# trigger is caught up; the first deploy backfills further, as far as artifacts are retained.
RECENT_RUNS = 10
BACKFILL_RUNS = 30
MAX_COMMITS = 1000
MAX_AGE_DAYS = 365
MAX_FAILED_TESTS = 50
# A build stops collecting after COLLECT_SECONDS, and gives up on an artifact after
# ARTIFACT_ATTEMPTS stalled transfers, so it ends well inside pages.yml's 30-minute build job.
COLLECT_SECONDS = 15 * 60
ARTIFACT_TIMEOUT_SECONDS = 120.0
ARTIFACT_ATTEMPTS = 2

PAGES_WORKFLOW = "pages.yml"
ANALYSIS_WORKFLOW = "Code Analysis"
ANALYSIS_ARTIFACT_PREFIX = "clang-tidy-clazy-output"
COVERAGE_ARTIFACT = "coverage-report"
COVERAGE_PREFIX = "src/"
TEST_RESULTS_PREFIX = "test-results-"
BUILD_STEP = "Build"


class HistoryError(RuntimeError):
    """The deployed history could not be read; deploying would truncate it."""


class CollectionDeadline(RuntimeError):
    """The collection deadline passed before a run was collected."""


class Collection(NamedTuple):
    records: list[dict[str, Any]]
    # Runs left for the next build: their collection failed, or the deadline passed first.
    pending: int


def run_url(repo: str, run_id: int) -> str:
    return f"https://github.com/{repo}/actions/runs/{run_id}"


def empty_history(repo: str) -> dict[str, Any]:
    return {"version": HISTORY_VERSION, "repository": repo, "records": []}


def record_key(record: dict[str, Any]) -> tuple[str, str]:
    return str(record["commit"]), str(record["workflow"])


def validate_history(data: Any) -> dict[str, Any]:
    """Return *data* when it is a history this builder can extend; raise HistoryError otherwise."""
    if not isinstance(data, dict):
        raise HistoryError("history is not a JSON object")
    if data.get("version") != HISTORY_VERSION:
        raise HistoryError(f"history version {data.get('version')!r} is not {HISTORY_VERSION}")
    records = data.get("records")
    if not isinstance(records, list):
        raise HistoryError("history records must be a list")
    for record in records:
        if not (
            isinstance(record, dict)
            and isinstance(record.get("commit"), str)
            and isinstance(record.get("workflow"), str)
            and isinstance(record.get("run_id"), int)
            and parse_github_timestamp(record.get("created_at")) is not None
        ):
            raise HistoryError(f"malformed history record: {str(record)[:200]}")
    return data


def fetch_history(url: str) -> dict[str, Any] | None:
    """Return the deployed history at *url*, or None when the site has none (HTTP 404).

    Any other failure raises HistoryError: a deploy without the existing records would replace
    the published history with a truncated one. A 404 alone does not prove that the site was
    never deployed; see require_first_deploy.
    """
    # The query string bypasses the Pages CDN's cached copy of an earlier deploy.
    try:
        body = fetch_bytes(f"{url}?t={int(time.time())}", timeout=30)
    except HTTPError as exc:
        if exc.code == 404:
            return None
        raise HistoryError(f"cannot fetch {url}: HTTP {exc.code}") from exc
    except (HTTPException, OSError, RuntimeError) as exc:
        raise HistoryError(f"cannot fetch {url}: {exc}") from exc
    try:
        data = json.loads(body)
    except ValueError as exc:
        raise HistoryError(f"{url} is not JSON: {exc}") from exc
    return validate_history(data)


def require_first_deploy(repo: str) -> None:
    """Raise HistoryError unless the site was never deployed, so a 404 may start a new history.

    The site also answers 404 during a Pages outage or after an unpublish, when starting over
    would replace up to a year of records. A pages.yml run concludes success only when its deploy
    job succeeded (a run whose jobs all skip concludes "skipped"), so any successful run means a
    deploy, and unlike a check of the deploy job by name this survives renaming the job.
    """
    query = urlencode({"status": "success", "per_page": 1})
    path = f"repos/{repo}/actions/workflows/{PAGES_WORKFLOW}/runs?{query}"
    try:
        data = gh_api_json(path)
    except (subprocess.SubprocessError, RuntimeError, ValueError, OSError) as exc:
        raise HistoryError(f"cannot tell whether the site was deployed before: {exc}") from exc
    count = data.get("total_count") if isinstance(data, dict) else None
    if not isinstance(count, int):
        raise HistoryError(f"cannot tell whether the site was deployed before: {path} has no count")
    if count:
        raise HistoryError(
            f"the site has no history (HTTP 404) but {count} {PAGES_WORKFLOW} runs deployed it; "
            "dispatch Pages with new_history to start a new one"
        )


def cap_records(
    records: Iterable[dict[str, Any]],
    *,
    now: datetime,
    max_commits: int = MAX_COMMITS,
    max_age_days: int = MAX_AGE_DAYS,
) -> list[dict[str, Any]]:
    """Keep the records of the newest *max_commits* commits that are at most *max_age_days* old.

    A commit's time is its first run's creation, so all of a commit's records go together.
    """
    records = list(records)
    first_seen: dict[str, datetime] = {}
    for record in records:
        created = parse_github_timestamp(record["created_at"])
        if created is None:
            continue
        commit = str(record["commit"])
        if commit not in first_seen or created < first_seen[commit]:
            first_seen[commit] = created
    cutoff = now - timedelta(days=max_age_days)
    newest = sorted(first_seen, key=lambda commit: first_seen[commit], reverse=True)
    kept = {commit for commit in newest[:max_commits] if first_seen[commit] >= cutoff}
    return [record for record in records if record["commit"] in kept]


def merge_history(
    history: dict[str, Any],
    new_records: Iterable[dict[str, Any]],
    *,
    now: datetime,
    max_commits: int = MAX_COMMITS,
    max_age_days: int = MAX_AGE_DAYS,
) -> dict[str, Any]:
    """Return *history* with *new_records* merged by (commit, workflow), then capped.

    A new record replaces the stored one for its key (a re-run's newer attempt); every other
    stored record is kept unless the caps drop it.
    """
    by_key = {record_key(record): record for record in history["records"]}
    for record in new_records:
        by_key[record_key(record)] = record
    merged = sorted(by_key.values(), key=lambda r: (str(r["created_at"]), str(r["workflow"])))
    return {
        **history,
        "version": HISTORY_VERSION,
        "generated_at": now.strftime("%Y-%m-%dT%H:%M:%SZ"),
        "records": cap_records(merged, now=now, max_commits=max_commits, max_age_days=max_age_days),
    }


def list_master_push_runs(repo: str, workflows: Iterable[str], per_workflow: int) -> list[dict]:
    """Return each workflow's latest *per_workflow* completed runs of pushes to master."""
    ids = {
        str(workflow.get("name")): int(workflow["id"])
        for workflow in gh_api_json(f"repos/{repo}/actions/workflows?per_page=100").get(
            "workflows", []
        )
        if isinstance(workflow, dict) and workflow.get("id")
    }
    query = urlencode(
        {"branch": "master", "event": "push", "status": "completed", "per_page": per_workflow}
    )
    runs: list[dict] = []
    for name in workflows:
        if name not in ids:
            gh_warning(f"No workflow named {name!r} in {repo}")
            continue
        data = gh_api_json(f"repos/{repo}/actions/workflows/{ids[name]}/runs?{query}")
        runs += [
            run
            for run in data.get("workflow_runs", [])
            if isinstance(run, dict)
            and run.get("event") == "push"
            and run.get("head_branch") == "master"
            and (run.get("head_repository") or {}).get("full_name") == repo
        ]
    return runs


def build_step_seconds(jobs: Iterable[dict[str, Any]]) -> dict[str, int]:
    """Return each job's Build step duration in seconds, for jobs that ran one."""
    durations: dict[str, int] = {}
    for job in jobs:
        for step in job.get("steps") or []:
            if not isinstance(step, dict) or step.get("name") != BUILD_STEP:
                continue
            seconds = seconds_between(step.get("started_at"), step.get("completed_at"))
            if seconds is not None and step.get("conclusion") != "skipped":
                durations[str(job.get("name", ""))] = round(seconds)
    return dict(sorted(durations.items()))


def _extract_members(archive: Path, suffix: str, destination: Path) -> list[Path]:
    """Extract the archive members whose names end with *suffix*, flattened into *destination*."""
    destination.mkdir(parents=True, exist_ok=True)
    extracted = []
    with zipfile.ZipFile(archive) as zf:
        for member in zf.infolist():
            name = Path(member.filename).name
            if member.is_dir() or not name.endswith(suffix):
                continue
            target = destination / name
            with zf.open(member) as source, target.open("wb") as sink:
                shutil.copyfileobj(source, sink)
            extracted.append(target)
    return sorted(extracted)


def suite_metrics(
    suite: str, reports: Iterable[Path]
) -> tuple[dict[str, float | int], list[list[str]]]:
    """Return a suite's counts and duration and its failed tests from its JUnit reports."""
    total = failed = skipped = 0
    seconds = 0.0
    failures: list[list[str]] = []
    for report in reports:
        try:
            cases, wall = parse_junit(report, suite)
        except (OSError, XMLParseError) as exc:
            gh_warning(f"Cannot read JUnit report {report.name}: {exc}")
            continue
        total += len(cases)
        failed += sum(case.outcome == "failed" for case in cases)
        skipped += sum(case.outcome == "skipped" for case in cases)
        seconds += wall if wall is not None else sum(case.seconds for case in cases)
        failures += [[suite, case.name] for case in cases if case.outcome == "failed"]
    metrics: dict[str, float | int] = {
        "total": total,
        "failed": failed,
        "skipped": skipped,
        "seconds": round(seconds, 1),
    }
    return metrics, failures


def _coverage(path: Path) -> dict[str, float | int] | None:
    try:
        summary = read_gcovr_summary(path, path_prefix=COVERAGE_PREFIX)
    except CoverageSummaryError as exc:
        gh_warning(f"Cannot read coverage summary: {exc}")
        return None
    if not summary.line_total:
        return None
    return {
        "line_percent": summary.line_percent,
        "line_covered": summary.line_covered,
        "line_total": summary.line_total,
    }


def _seconds_left(deadline: float | None) -> float | None:
    """Return the seconds left before *deadline* (a time.monotonic() value), None without one.

    Raises CollectionDeadline once it has passed.
    """
    if deadline is None:
        return None
    remaining = deadline - time.monotonic()
    if remaining <= 0:
        raise CollectionDeadline("the collection deadline passed")
    return remaining


def _artifact_metrics(
    repo: str,
    run_id: int,
    artifacts: list[dict[str, Any]],
    run_dir: Path,
    *,
    deadline: float | None = None,
) -> dict[str, Any]:
    """Download a run's test, coverage, and analysis artifacts and return their metrics."""
    metrics: dict[str, Any] = {}
    tests: dict[str, dict[str, float | int]] = {}
    failures: list[list[str]] = []
    analysis = run_dir / "analysis"
    for artifact in sorted(artifacts, key=lambda a: str(a.get("name", ""))):
        name = str(artifact.get("name", ""))
        if not (
            name.startswith(TEST_RESULTS_PREFIX)
            or name == COVERAGE_ARTIFACT
            or name.startswith(ANALYSIS_ARTIFACT_PREFIX)
        ):
            continue
        timeout = ARTIFACT_TIMEOUT_SECONDS
        if (remaining := _seconds_left(deadline)) is not None:
            # Split the time left so that every attempt ends by the deadline.
            timeout = min(timeout, remaining / ARTIFACT_ATTEMPTS)
        archive = run_dir / f"{artifact['id']}.zip"
        download_artifact_zip(
            repo, artifact["id"], archive, attempts=ARTIFACT_ATTEMPTS, timeout=timeout
        )
        try:
            if name.startswith(TEST_RESULTS_PREFIX):
                suite = name.removeprefix(TEST_RESULTS_PREFIX)
                if reports := _extract_members(archive, ".xml", run_dir / name):
                    tests[suite], found = suite_metrics(suite, reports)
                    failures += found
            elif name == COVERAGE_ARTIFACT:
                summaries = _extract_members(archive, SUMMARY_FILENAME, run_dir / name)
                if summaries and (coverage := _coverage(summaries[0])):
                    metrics["coverage"] = coverage
            else:
                _extract_members(archive, DIAGNOSTICS_SUFFIX, analysis / name)
        except zipfile.BadZipFile as exc:
            gh_warning(f"Cannot read artifact {name} of run {run_id}: {exc}")
        finally:
            archive.unlink(missing_ok=True)
    if tests:
        metrics["tests"] = tests
    if failures:
        metrics["failed_tests"] = sorted(failures)[:MAX_FAILED_TESTS]
    if analysis.is_dir():
        warnings, _changed = load_unique_warnings(analysis)
        if warnings:
            metrics["warnings"] = {tool: len(found) for tool, found in sorted(warnings.items())}
    return metrics


def collect_record(
    repo: str, run: dict[str, Any], work_dir: Path, *, deadline: float | None = None
) -> dict[str, Any] | None:
    """Return a completed run's record, or None when its artifacts are past retention.

    Raises when the API or an artifact download fails (``subprocess.SubprocessError``,
    ``RuntimeError``), or CollectionDeadline when *deadline* passes before every download, so a
    later build collects the run again instead of storing an incomplete record.
    """
    run_id = int(run["id"])
    artifacts = list_run_artifacts(repo, run_id)
    live = [artifact for artifact in artifacts if not artifact.get("expired")]
    if artifacts and not live:
        return None

    record: dict[str, Any] = {
        "commit": str(run["head_sha"]),
        "workflow": str(run["name"]),
        "run_id": run_id,
        "attempt": int(run.get("run_attempt") or 1),
        "created_at": str(run["created_at"]),
        "conclusion": str(run.get("conclusion") or ""),
    }
    wall = seconds_between(
        run.get("run_started_at") or run.get("created_at"), run.get("updated_at")
    )
    if wall is not None:
        record["wall_seconds"] = round(wall)
    if build := build_step_seconds(list_run_jobs(repo, run_id)):
        record["build_seconds"] = build
    sizes = {
        str(a["name"]): int(a.get("size_in_bytes") or 0)
        for a in live
        if is_distributable_artifact(str(a.get("name", "")))
    }
    if sizes:
        record["artifact_bytes"] = dict(sorted(sizes.items()))

    run_dir = work_dir / str(run_id)
    try:
        record.update(_artifact_metrics(repo, run_id, live, run_dir, deadline=deadline))
    finally:
        shutil.rmtree(run_dir, ignore_errors=True)
    return record


def collect_new_records(
    repo: str,
    history: dict[str, Any],
    workflows: Iterable[str],
    work_dir: Path,
    *,
    per_workflow: int,
    deadline: float | None = None,
) -> Collection:
    """Collect the latest master push runs that the history lacks, or holds an older attempt of.

    Runs are collected newest first. A run that cannot be collected now is skipped with a
    warning, as is every run left when *deadline* (a time.monotonic() value) passes; the next
    build retries them while they remain among each workflow's latest runs.
    """
    stored = {
        record_key(record): (record.get("run_id"), record.get("attempt"))
        for record in history["records"]
    }

    def is_new(run: dict[str, Any]) -> bool:
        key = (str(run.get("head_sha", "")), str(run.get("name", "")))
        return stored.get(key) != (run.get("id"), int(run.get("run_attempt") or 1))

    try:
        runs = list_master_push_runs(repo, workflows, per_workflow)
    except (subprocess.SubprocessError, RuntimeError, ValueError, OSError) as exc:
        gh_warning(f"Cannot list master push runs, keeping the stored history: {exc}")
        return Collection([], 0)
    todo = sorted(
        filter(is_new, runs), key=lambda run: str(run.get("created_at", "")), reverse=True
    )
    records = []
    pending = 0
    for index, run in enumerate(todo):
        try:
            _seconds_left(deadline)
            record = collect_record(repo, run, work_dir, deadline=deadline)
        except CollectionDeadline:
            left = len(todo) - index
            pending += left
            gh_warning(f"The collection deadline passed; {left} runs left for the next build")
            break
        except (subprocess.SubprocessError, RuntimeError, ValueError, OSError) as exc:
            name = run.get("name")
            gh_warning(f"Cannot collect {name} run {run.get('id')}, retrying next build: {exc}")
            pending += 1
            continue
        if record is not None:
            records.append(record)
            print(f"Collected {record['workflow']} run {record['run_id']} ({record['commit'][:7]})")
    return Collection(records, pending)


def failing_tests(
    records: Iterable[dict[str, Any]], *, now: datetime, days: int
) -> list[dict[str, Any]]:
    """Return the tests that failed in master runs of the last *days* days, most recent first.

    Each row counts the runs the test failed in and the runs of its suite, and names the last
    failing run.
    """
    cutoff = now - timedelta(days=days)
    recent = sorted(
        (
            record
            for record in records
            if (created := parse_github_timestamp(record.get("created_at"))) and created >= cutoff
        ),
        key=lambda record: str(record["created_at"]),
    )
    suite_runs: dict[str, int] = {}
    rows: dict[tuple[str, str], dict[str, Any]] = {}
    for record in recent:
        for suite in record.get("tests") or {}:
            suite_runs[suite] = suite_runs.get(suite, 0) + 1
        for suite, name in {tuple(entry) for entry in record.get("failed_tests") or []}:
            row = rows.setdefault((suite, name), {"suite": suite, "name": name, "failures": 0})
            row["failures"] += 1
            row.update(
                workflow=record["workflow"],
                commit=record["commit"],
                run_id=record["run_id"],
                last_seen=record["created_at"],
            )
    for row in rows.values():
        row["runs"] = suite_runs.get(row["suite"], row["failures"])
    by_name = sorted(rows.values(), key=lambda row: (row["suite"], row["name"]))
    return sorted(by_name, key=lambda row: (row["last_seen"], row["failures"]), reverse=True)
