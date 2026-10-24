"""Tests for pages_history.py: history fetch, merge and caps, and run metric collection."""

from __future__ import annotations

import io
import json
import subprocess
import time
import zipfile
from datetime import datetime, timedelta, timezone
from types import SimpleNamespace
from typing import TYPE_CHECKING, Any
from urllib.error import HTTPError, URLError

import pytest
from ci.publish import pages_history as mod
from hypothesis import given
from hypothesis import strategies as st

from ..test_junit import QT_FAILURE, junit_xml

if TYPE_CHECKING:
    from pathlib import Path

REPO = "mavlink/qgroundcontrol"
NOW = datetime(2026, 10, 23, 23, 0, tzinfo=timezone.utc)
HISTORY_URL = "https://mavlink.github.io/qgroundcontrol/data/history.json"
COMMIT = "6f5b75cf8721ae45c2ea28d1f09b880cbbe72a5d"


def _record(commit: str, workflow: str = "Linux", *, days_ago: float = 0, **fields: Any) -> dict:
    created = NOW - timedelta(days=days_ago)
    return {
        "commit": commit,
        "workflow": workflow,
        "run_id": fields.pop("run_id", 100),
        "attempt": fields.pop("attempt", 1),
        "created_at": created.strftime("%Y-%m-%dT%H:%M:%SZ"),
        "conclusion": "success",
        **fields,
    }


def _history(*records: dict) -> dict:
    return {"version": mod.HISTORY_VERSION, "repository": REPO, "records": list(records)}


def _http_error(code: int) -> HTTPError:
    return HTTPError(HISTORY_URL, code, "error", {}, None)  # type: ignore[arg-type]


# ---------------------------------------------------------------------------
# Fetching the deployed history
# ---------------------------------------------------------------------------


def test_fetch_returns_none_when_the_site_has_no_history(monkeypatch: pytest.MonkeyPatch) -> None:
    def fetch(url: str, *, timeout: float) -> bytes:
        raise _http_error(404)

    monkeypatch.setattr(mod, "fetch_bytes", fetch)
    assert mod.fetch_history(HISTORY_URL) is None


@pytest.mark.parametrize(
    "error", [_http_error(403), _http_error(500), RuntimeError("3 attempts"), URLError("dns")]
)
def test_fetch_failures_abort(monkeypatch: pytest.MonkeyPatch, error: Exception) -> None:
    def fetch(url: str, *, timeout: float) -> bytes:
        raise error

    monkeypatch.setattr(mod, "fetch_bytes", fetch)
    with pytest.raises(mod.HistoryError, match="cannot fetch"):
        mod.fetch_history(HISTORY_URL)


@pytest.mark.parametrize(
    "body",
    [
        b"<html>",
        b"[]",
        json.dumps({"version": 2, "records": []}).encode(),
        json.dumps({"version": 1, "records": {}}).encode(),
        json.dumps({"version": 1, "records": [{"commit": "a", "workflow": "Linux"}]}).encode(),
    ],
)
def test_unusable_history_aborts(monkeypatch: pytest.MonkeyPatch, body: bytes) -> None:
    monkeypatch.setattr(mod, "fetch_bytes", lambda url, *, timeout: body)
    with pytest.raises(mod.HistoryError):
        mod.fetch_history(HISTORY_URL)


def test_fetch_bypasses_the_cdn_cache(monkeypatch: pytest.MonkeyPatch) -> None:
    urls = []
    history = _history(_record(COMMIT))

    def fetch(url: str, *, timeout: float) -> bytes:
        urls.append(url)
        return json.dumps(history).encode()

    monkeypatch.setattr(mod, "fetch_bytes", fetch)
    assert mod.fetch_history(HISTORY_URL) == history
    assert urls[0].startswith(f"{HISTORY_URL}?t=")


DEPLOYS_PATH = f"repos/{REPO}/actions/workflows/pages.yml/runs?status=success&per_page=1"


def _deploys(monkeypatch: pytest.MonkeyPatch, answer: object) -> list[str]:
    """Answer the successful pages.yml runs query with *answer* (raised if an exception)."""
    paths: list[str] = []

    def api(path: str) -> object:
        paths.append(path)
        if isinstance(answer, Exception):
            raise answer
        return answer

    monkeypatch.setattr(mod, "gh_api_json", api)
    return paths


def test_a_site_never_deployed_may_start_a_new_history(monkeypatch: pytest.MonkeyPatch) -> None:
    paths = _deploys(monkeypatch, {"total_count": 0, "workflow_runs": []})
    mod.require_first_deploy(REPO)
    assert paths == [DEPLOYS_PATH]


def test_a_404_after_a_deploy_aborts(monkeypatch: pytest.MonkeyPatch) -> None:
    # A Pages outage or an unpublish also answers 404; starting over would drop the records.
    _deploys(monkeypatch, {"total_count": 1, "workflow_runs": [{"id": 1}]})
    with pytest.raises(mod.HistoryError, match=r"1 pages\.yml runs deployed it.*new_history"):
        mod.require_first_deploy(REPO)


@pytest.mark.parametrize(
    "answer",
    [
        subprocess.CalledProcessError(1, ["gh", "api"], stderr="HTTP 502"),
        subprocess.TimeoutExpired(["gh", "api"], 60),
        RuntimeError("gh failed"),
        ValueError("Expecting value"),
        {"message": "Not Found"},
        {"total_count": "1"},
        [],
    ],
)
def test_a_failed_deploy_check_aborts(monkeypatch: pytest.MonkeyPatch, answer: object) -> None:
    _deploys(monkeypatch, answer)
    with pytest.raises(mod.HistoryError, match="cannot tell whether the site was deployed"):
        mod.require_first_deploy(REPO)


# ---------------------------------------------------------------------------
# Merging and capping
# ---------------------------------------------------------------------------


def test_merge_adds_and_replaces_by_commit_and_workflow() -> None:
    stored = _history(
        _record("a", days_ago=2, run_id=1),
        _record("a", "Windows", days_ago=2, run_id=2),
        _record("b", days_ago=1, run_id=3),
    )
    rerun = _record("b", days_ago=1, run_id=3, attempt=2, conclusion="failure")
    merged = mod.merge_history(stored, [rerun, _record("c", run_id=4)], now=NOW)

    assert [(r["commit"], r["workflow"], r["attempt"]) for r in merged["records"]] == [
        ("a", "Linux", 1),
        ("a", "Windows", 1),
        ("b", "Linux", 2),
        ("c", "Linux", 1),
    ]
    assert merged["version"] == mod.HISTORY_VERSION
    assert merged["generated_at"] == "2026-10-23T23:00:00Z"
    assert merged["repository"] == REPO


def test_caps_drop_whole_commits_by_count_and_age() -> None:
    records = [
        _record("old", days_ago=400),
        _record("c1", days_ago=3),
        _record("c1", "Windows", days_ago=2.9),
        _record("c2", days_ago=2),
        _record("c3", days_ago=1),
    ]
    capped = mod.cap_records(records, now=NOW, max_commits=2, max_age_days=365)
    assert [r["commit"] for r in capped] == ["c2", "c3"]
    by_age = mod.cap_records(records, now=NOW, max_commits=1000, max_age_days=365)
    assert [r["commit"] for r in by_age] == ["c1", "c1", "c2", "c3"]


_commits = st.sampled_from([f"{index:040x}" for index in range(12)])
_workflows = st.sampled_from(["Linux", "Windows", "Code Analysis"])
_records = st.builds(
    lambda commit, workflow, days, run_id: _record(commit, workflow, days_ago=days, run_id=run_id),
    _commits,
    _workflows,
    st.floats(min_value=0, max_value=300),
    st.integers(min_value=1, max_value=10**6),
)


@given(stored=st.lists(_records, max_size=30), new=st.lists(_records, max_size=10))
def test_merge_never_drops_a_record_within_the_caps(stored: list, new: list) -> None:
    history = mod.validate_history(_history(*stored))
    merged = mod.merge_history(history, new, now=NOW)
    keys = {mod.record_key(record) for record in merged["records"]}
    assert {mod.record_key(record) for record in [*stored, *new]} <= keys
    assert len(keys) == len(merged["records"])
    again = mod.merge_history(merged, [], now=NOW)
    assert again["records"] == merged["records"]


# ---------------------------------------------------------------------------
# Collecting a run
# ---------------------------------------------------------------------------


def _zip(members: dict[str, str]) -> bytes:
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, "w") as archive:
        for name, text in members.items():
            archive.writestr(name, text)
    return buffer.getvalue()


COVERAGE_SUMMARY = {
    "gcovr/summary_format_version": "0.6",
    "line_total": 300,
    "line_covered": 250,
    "line_percent": 83.3,
    "branch_total": 10,
    "branch_covered": 5,
    "branch_percent": 50.0,
    "files": [
        {"filename": "src/Vehicle/Vehicle.cc", "line_total": 150, "line_covered": 100,
         "line_percent": 66.7, "branch_total": 8, "branch_covered": 4, "branch_percent": 50.0},
        {"filename": "src/ADSB/ADSB.h", "line_total": 50, "line_covered": 50,
         "line_percent": 100.0, "branch_total": 2, "branch_covered": 1, "branch_percent": 50.0},
        {"filename": "test/VehicleTest.cc", "line_total": 100, "line_covered": 100,
         "line_percent": 100.0, "branch_total": 0, "branch_covered": 0, "branch_percent": 0.0},
    ],
}  # fmt: skip
DIAGNOSTICS = {
    "tool": "clang-tidy",
    "shard": 1,
    "diagnostics": {
        "/usr/include/c++/14/bits/predefined_ops.h": [
            [395, 25, "warning", "clang-analyzer-security.ArrayBound"]
        ],
        "src/Vehicle/Vehicle.h": [
            [10, 5, "warning", "modernize-use-nodiscard"],
            [12, 1, "error", "clang-diagnostic-error"],
        ],
    },
}
LINUX_RUN = {
    "id": 40021772122,
    "name": "Linux",
    "head_sha": COMMIT,
    "head_branch": "master",
    "event": "push",
    "status": "completed",
    "conclusion": "success",
    "run_attempt": 1,
    "created_at": "2026-10-23T20:46:14Z",
    "run_started_at": "2026-10-23T20:46:14Z",
    "updated_at": "2026-10-23T20:59:07Z",
    "head_repository": {"full_name": REPO},
}
JOBS = [
    {
        "name": "changes / Detect Changes",
        "conclusion": "success",
        "steps": [{"name": "Plan", "conclusion": "success",
                   "started_at": "2026-10-23T20:46:20Z", "completed_at": "2026-10-23T20:46:25Z"}],
    },
    {
        "name": "Release linux_gcc_64",
        "conclusion": "success",
        "steps": [
            {"name": "Build Setup", "conclusion": "success",
             "started_at": "2026-10-23T20:47:08Z", "completed_at": "2026-10-23T20:47:34Z"},
            {"name": "Build", "conclusion": "success",
             "started_at": "2026-10-23T20:48:08Z", "completed_at": "2026-10-23T20:49:02Z"},
            {"name": "Post Build", "conclusion": "success",
             "started_at": "2026-10-23T20:53:12Z", "completed_at": "2026-10-23T20:53:12Z"},
        ],
    },
    {
        "name": "Custom Plugin",
        "conclusion": "skipped",
        "steps": [{"name": "Build", "conclusion": "skipped",
                   "started_at": "2026-10-23T20:48:04Z", "completed_at": "2026-10-23T20:48:04Z"}],
    },
]  # fmt: skip


def _artifact(artifact_id: int, name: str, size: int = 1000, *, expired: bool = False) -> dict:
    return {"id": artifact_id, "name": name, "size_in_bytes": size, "expired": expired}


ARTIFACTS = [
    _artifact(1, "QGroundControl-x86_64", 202_952_797),
    _artifact(2, "sbom-QGroundControl-x86_64", 14_026),
    _artifact(3, "coverage-report", 17_870_558),
    _artifact(4, "test-results-linux-coverage-unit", 45_310),
    _artifact(5, "build-profile-rn3v5577", 116_409),
    _artifact(6, "clang-tidy-clazy-output", 7_340_165),
]
ZIPS = {
    3: _zip({"coverage-summary.json": json.dumps(COVERAGE_SUMMARY), "coverage.html": "<html>"}),
    4: _zip(
        {
            "junit-results-linux-coverage-unit.xml": junit_xml(
                [
                    ("MissionControllerTest", "failed", 95.2, "Failed", QT_FAILURE),
                    ("ParameterManagerTest", "passed", 48.7, "", ""),
                    ("SerialLinkTest", "skipped", 0, "", ""),
                ],
                70,
            ),
            "test-output-linux-coverage-unit.txt": "log",
        }
    ),
    6: _zip(
        {
            "clang-tidy-diagnostics.json": json.dumps(DIAGNOSTICS),
            "clang-tidy-raw.txt": "raw",
        }
    ),
}


@pytest.fixture
def github(monkeypatch: pytest.MonkeyPatch) -> dict[str, Any]:
    """Fake GitHub API: artifact listings, jobs, and artifact archives by id."""
    state: dict[str, Any] = {
        "artifacts": ARTIFACTS,
        "jobs": JOBS,
        "zips": ZIPS,
        "downloads": [],
        "limits": [],
    }

    def download(
        repo: str, artifact_id: int, destination: Path, *, attempts: int, timeout: float
    ) -> None:
        state["downloads"].append(artifact_id)
        state["limits"].append((attempts, timeout))
        if artifact_id not in state["zips"]:
            raise RuntimeError(f"Cannot download artifact {artifact_id}: HTTP 500")
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(state["zips"][artifact_id])

    monkeypatch.setattr(mod, "list_run_artifacts", lambda repo, run_id: state["artifacts"])
    monkeypatch.setattr(mod, "list_run_jobs", lambda repo, run_id: state["jobs"])
    monkeypatch.setattr(mod, "download_artifact_zip", download)
    return state


def test_collect_record_reads_every_metric(github: dict, tmp_path: Path) -> None:
    record = mod.collect_record(REPO, LINUX_RUN, tmp_path / "work")

    assert record == {
        "commit": COMMIT,
        "workflow": "Linux",
        "run_id": 40021772122,
        "attempt": 1,
        "created_at": "2026-10-23T20:46:14Z",
        "conclusion": "success",
        "wall_seconds": 773,
        "build_seconds": {"Release linux_gcc_64": 54},
        "artifact_bytes": {"QGroundControl-x86_64": 202_952_797},
        # src/ only: 150 of 200 lines; the test/ file is left out.
        "coverage": {"line_percent": 75.0, "line_covered": 150, "line_total": 200},
        "tests": {"linux-coverage-unit": {"total": 3, "failed": 1, "skipped": 1, "seconds": 70.0}},
        "failed_tests": [["linux-coverage-unit", "MissionControllerTest"]],
        "warnings": {"clang-tidy": 2},
    }
    # Packages, SBOMs and build profiles are listed, not downloaded.
    assert sorted(github["downloads"]) == [3, 4, 6]
    # A stalled transfer gives up long before the build job's timeout.
    assert github["limits"] == [(mod.ARTIFACT_ATTEMPTS, mod.ARTIFACT_TIMEOUT_SECONDS)] * 3
    assert mod.ARTIFACT_ATTEMPTS * mod.ARTIFACT_TIMEOUT_SECONDS <= 300
    assert not (tmp_path / "work").exists() or not any((tmp_path / "work").iterdir())


@pytest.fixture
def clock(monkeypatch: pytest.MonkeyPatch) -> SimpleNamespace:
    """Replace pages_history's monotonic clock with one the test sets (``clock.now``)."""
    fake = SimpleNamespace(now=1000.0)
    monkeypatch.setattr(mod, "time", SimpleNamespace(monotonic=lambda: fake.now, time=time.time))
    return fake


def test_downloads_end_by_the_collection_deadline(
    github: dict, clock: SimpleNamespace, tmp_path: Path
) -> None:
    far = mod.collect_record(REPO, LINUX_RUN, tmp_path, deadline=clock.now + 3600)
    near = mod.collect_record(REPO, LINUX_RUN, tmp_path, deadline=clock.now + 100)
    assert far == near
    assert (
        github["limits"]
        == [(mod.ARTIFACT_ATTEMPTS, mod.ARTIFACT_TIMEOUT_SECONDS)] * 3
        + [(mod.ARTIFACT_ATTEMPTS, 100 / mod.ARTIFACT_ATTEMPTS)] * 3
    )


def test_a_deadline_passing_mid_run_leaves_no_partial_record(
    github: dict, clock: SimpleNamespace, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    real = github["zips"]

    def slow_download(repo: str, artifact_id: int, destination: Path, **limits: float) -> None:
        github["downloads"].append(artifact_id)
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(real[artifact_id])
        clock.now += 60

    monkeypatch.setattr(mod, "download_artifact_zip", slow_download)
    with pytest.raises(mod.CollectionDeadline):
        mod.collect_record(REPO, LINUX_RUN, tmp_path / "work", deadline=clock.now + 90)
    # Artifacts download by name: clang-tidy-clazy-output, coverage-report, then test-results.
    assert github["downloads"] == [6, 3]
    assert not any((tmp_path / "work").iterdir())


def test_collect_record_skips_runs_whose_artifacts_expired(github: dict, tmp_path: Path) -> None:
    github["artifacts"] = [_artifact(4, "test-results-linux-coverage-unit", expired=True)]
    assert mod.collect_record(REPO, LINUX_RUN, tmp_path) is None
    github["artifacts"] = []
    record = mod.collect_record(REPO, LINUX_RUN, tmp_path)
    assert record is not None
    assert "tests" not in record
    assert record["wall_seconds"] == 773


def test_unreadable_archives_leave_their_metric_out(github: dict, tmp_path: Path) -> None:
    github["zips"] = {**ZIPS, 3: b"not a zip"}
    record = mod.collect_record(REPO, LINUX_RUN, tmp_path)
    assert record is not None
    assert "coverage" not in record
    assert record["tests"]["linux-coverage-unit"]["total"] == 3


def test_build_steps_exclude_skipped_and_other_steps() -> None:
    assert mod.build_step_seconds(JOBS) == {"Release linux_gcc_64": 54}


def _runs_page(*runs: dict) -> dict:
    return {"total_count": len(runs), "workflow_runs": list(runs)}


def test_master_push_runs_exclude_pull_requests_and_forks(monkeypatch: pytest.MonkeyPatch) -> None:
    fork_pr = {
        **LINUX_RUN,
        "id": 2,
        "event": "pull_request",
        "head_repository": {"full_name": "x/y"},
    }
    branch = {**LINUX_RUN, "id": 3, "head_branch": "feature"}
    dispatch = {**LINUX_RUN, "id": 4, "event": "workflow_dispatch"}
    calls = []

    def api(path: str) -> dict:
        calls.append(path)
        if path.endswith("/actions/workflows?per_page=100"):
            return {"workflows": [{"id": 11, "name": "Linux"}, {"id": 12, "name": "Windows"}]}
        return _runs_page(LINUX_RUN, fork_pr, branch, dispatch)

    monkeypatch.setattr(mod, "gh_api_json", api)
    runs = mod.list_master_push_runs(REPO, ["Linux", "Missing"], 30)
    assert [run["id"] for run in runs] == [LINUX_RUN["id"]]
    assert calls[1] == (
        f"repos/{REPO}/actions/workflows/11/runs"
        "?branch=master&event=push&status=completed&per_page=30"
    )


def test_collect_new_records_skips_known_attempts_and_retries_failures(
    github: dict, monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    known = {**LINUX_RUN, "id": 7, "head_sha": "a" * 40}
    rerun = {**LINUX_RUN, "id": 8, "head_sha": "b" * 40, "run_attempt": 2}
    broken = {**LINUX_RUN, "id": 9, "head_sha": "c" * 40}
    history = _history(_record("a" * 40, run_id=7), _record("b" * 40, run_id=8))
    monkeypatch.setattr(mod, "list_master_push_runs", lambda *a, **k: [known, rerun, broken])
    real = mod.collect_record

    def collect(repo: str, run: dict, work_dir: Path, *, deadline: float | None) -> dict | None:
        if run["id"] == 9:
            raise subprocess.CalledProcessError(1, ["gh", "api"], stderr="HTTP 502")
        return real(repo, run, work_dir, deadline=deadline)

    monkeypatch.setattr(mod, "collect_record", collect)
    records, pending = mod.collect_new_records(REPO, history, ["Linux"], tmp_path, per_workflow=10)
    assert [(r["run_id"], r["attempt"]) for r in records] == [(8, 2)]
    assert pending == 1


def _runs(count: int) -> list[dict]:
    """Return *count* Linux runs of distinct commits, one hour apart, oldest first."""
    start = datetime(2026, 10, 1, tzinfo=timezone.utc)
    return [
        {
            **LINUX_RUN,
            "id": index,
            "head_sha": f"{index:040x}",
            "created_at": (start + timedelta(hours=index)).strftime("%Y-%m-%dT%H:%M:%SZ"),
        }
        for index in range(1, count + 1)
    ]


def test_collection_stops_at_its_deadline_newest_runs_first(
    clock: SimpleNamespace,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    tmp_path: Path,
) -> None:
    runs = _runs(6)
    monkeypatch.setattr(mod, "list_master_push_runs", lambda *a, **k: runs)
    deadlines = []

    def collect(repo: str, run: dict, work_dir: Path, *, deadline: float | None) -> dict:
        deadlines.append(deadline)
        clock.now += 100
        return _record(run["head_sha"], run_id=run["id"])

    monkeypatch.setattr(mod, "collect_record", collect)
    deadline = clock.now + 250
    records, pending = mod.collect_new_records(
        REPO, _history(), ["Linux"], tmp_path, per_workflow=10, deadline=deadline
    )
    # The deadline leaves the oldest runs, which the next build still lists.
    assert [record["run_id"] for record in records] == [6, 5, 4]
    assert pending == 3
    assert deadlines == [deadline] * 3
    assert "::warning::The collection deadline passed; 3 runs left for the next build" in (
        capsys.readouterr().out
    )


def test_a_deadline_passing_mid_run_skips_it_and_the_rest(
    clock: SimpleNamespace, monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    monkeypatch.setattr(mod, "list_master_push_runs", lambda *a, **k: _runs(3))

    def collect(repo: str, run: dict, work_dir: Path, *, deadline: float | None) -> dict:
        if run["id"] == 2:
            raise mod.CollectionDeadline("the collection deadline passed")
        return _record(run["head_sha"], run_id=run["id"])

    monkeypatch.setattr(mod, "collect_record", collect)
    collected = mod.collect_new_records(
        REPO, _history(), ["Linux"], tmp_path, per_workflow=10, deadline=clock.now + 3600
    )
    assert collected == mod.Collection([_record(f"{3:040x}", run_id=3)], 2)


def test_listing_failures_keep_the_stored_history(
    monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str], tmp_path: Path
) -> None:
    def fail(*args: object, **kwargs: object) -> list:
        raise subprocess.CalledProcessError(1, ["gh"], stderr="HTTP 401")

    monkeypatch.setattr(mod, "list_master_push_runs", fail)
    assert mod.collect_new_records(REPO, _history(), ["Linux"], tmp_path, per_workflow=10) == (
        [],
        0,
    )
    assert "::warning::Cannot list master push runs" in capsys.readouterr().out


# ---------------------------------------------------------------------------
# Failing tests
# ---------------------------------------------------------------------------


def test_failing_tests_count_failures_against_suite_runs() -> None:
    suite = {"linux-coverage-unit": {"total": 3, "failed": 0}}
    records = [
        _record("old", days_ago=40, tests=suite, failed_tests=[["linux-coverage-unit", "OldTest"]]),
        _record("a", days_ago=3, run_id=1, tests=suite,
                failed_tests=[["linux-coverage-unit", "FTPManagerTest"]]),
        _record("b", days_ago=2, run_id=2, tests=suite),
        _record("c", days_ago=1, run_id=3, tests=suite,
                failed_tests=[["linux-coverage-unit", "FTPManagerTest"],
                              ["linux-coverage-unit", "AFlakyTest"]]),
    ]  # fmt: skip
    rows = mod.failing_tests(records, now=NOW, days=30)
    assert [(row["name"], row["failures"], row["runs"], row["run_id"]) for row in rows] == [
        ("FTPManagerTest", 2, 3, 3),
        ("AFlakyTest", 1, 3, 3),
    ]
    assert rows[0]["commit"] == "c"
