"""Tests for gh_cache_cleanup.py."""

from __future__ import annotations

import json
import subprocess
from datetime import UTC, datetime, timedelta
from typing import TYPE_CHECKING, Any

import pytest
from ci.cache import gh_cache_cleanup as mod
from ci.cache.build_cache_artifact import artifact_name

from .._helpers import completed

if TYPE_CHECKING:
    from pathlib import Path

REPO_ID = 22
SNAPSHOT_A = artifact_name("ccache-mac-ios-Release-shared", "moccache-mac-ios-shared-Release")
SNAPSHOT_B = artifact_name("ccache-mac-ios-Debug-shared", "moccache-mac-ios-shared-Debug")


def _ago(hours: float) -> str:
    return (datetime.now(UTC) - timedelta(hours=hours)).strftime("%Y-%m-%dT%H:%M:%SZ")


class FakeArtifacts:
    """The repository's artifact and run APIs, served from memory through ``gh``."""

    def __init__(self) -> None:
        self.artifacts: list[dict[str, Any]] = []
        self.runs: dict[int, dict[str, Any]] = {}
        self.requests: list[tuple[int, str]] = []
        self.failures: dict[tuple[int, str], int] = {}
        # Names whose listing returns every artifact, as if the API ignored its name filter.
        self.unfiltered: set[str] = set()
        self.undeletable: set[int] = set()
        self.deleted: list[int] = []
        self.total_offset = 0
        self.real_gh: Any = None

    def add(
        self,
        name: str,
        *,
        hours_ago: float = 1,
        size_mb: int = 100,
        expired: bool = False,
        workflow_id: int = 11,
        event: str = "push",
        branch: str = "master",
        repository_id: int = REPO_ID,
    ) -> int:
        """Add an artifact, newer than those added before it, and its producing run."""
        artifact_id = 1000 + len(self.artifacts)
        run_id = 5000 + len(self.artifacts)
        self.runs[run_id] = {
            "id": run_id,
            "workflow_id": workflow_id,
            "event": event,
            "head_branch": branch,
            "head_repository": {"id": repository_id},
        }
        self.artifacts.insert(
            0,
            {
                "id": artifact_id,
                "name": name,
                "size_in_bytes": size_mb * 1024 * 1024,
                "expired": expired,
                "created_at": _ago(hours_ago),
                "workflow_run": {
                    "id": run_id,
                    "head_branch": branch,
                    "head_repository_id": repository_id,
                },
            },
        )
        return artifact_id

    def gh(self, *args: str, **kwargs: Any) -> subprocess.CompletedProcess:
        path = next((arg for arg in args if "/actions/artifacts" in arg), "")
        if args[0] != "api" or not path:
            return self.real_gh(*args, **kwargs)
        if args[1:3] == ("--method", "DELETE"):
            artifact_id = int(path.rsplit("/", 1)[1])
            (artifact,) = (item for item in self.artifacts if item["id"] == artifact_id)
            assert mod.is_snapshot_name(artifact["name"]), artifact["name"]
            if artifact_id in self.undeletable:
                return completed(returncode=1, stderr="HTTP 403")
            self.deleted.append(artifact_id)
            return completed()
        assert args[1:3] == ("--method", "GET")
        query = dict(arg.split("=", 1) for arg in args if "=" in arg)
        page, name = int(query["page"]), query.get("name", "")
        self.requests.append((page, name))
        if self.failures.get((page, name), 0) > 0:
            self.failures[(page, name)] -= 1
            return completed(returncode=1, stderr="unexpected end of JSON input")
        listed = [
            item
            for item in self.artifacts
            if not name or name in self.unfiltered or item["name"] == name
        ]
        size = int(query["per_page"])
        payload = {
            "total_count": len(listed) + self.total_offset,
            "artifacts": listed[(page - 1) * size : page * size],
        }
        return completed(json.dumps(payload))

    def run(self, path: str) -> dict[str, Any]:
        return self.runs[int(path.rsplit("/", 1)[1])]


@pytest.fixture(autouse=True)
def artifacts(monkeypatch: pytest.MonkeyPatch) -> FakeArtifacts:
    """Serve the artifact API from memory, empty unless a test adds artifacts."""
    fake = FakeArtifacts()
    fake.real_gh = mod.gh
    monkeypatch.setattr(mod, "gh", fake.gh)
    monkeypatch.setattr(mod, "gh_api_json", fake.run)
    monkeypatch.setattr(mod.time, "sleep", lambda _seconds: None)
    return fake


def test_list_caches_parses_json(monkeypatch) -> None:
    calls: list[list[str]] = []

    def fake_run(cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess:
        calls.append(cmd)
        return completed(
            '[{"key":"key-1","ref":"refs/heads/main","sizeInBytes":104857600},'
            '{"key":"key-2","ref":"refs/pull/1/merge","sizeInBytes":52428800}]'
        )

    monkeypatch.setattr(subprocess, "run", fake_run)
    rows = mod.list_caches("owner/repo", "", 500)
    assert [r.key for r in rows] == ["key-1", "key-2"]
    assert rows[0].size_bytes == 104857600
    assert rows[1].ref == "refs/pull/1/merge"
    assert calls[0][:3] == ["gh", "cache", "list"]
    assert "--ref" not in calls[0]


@pytest.mark.parametrize(
    ("branch", "ref"),
    [("feature-x", "refs/heads/feature-x"), ("refs/pull/7/merge", "refs/pull/7/merge")],
)
def test_list_caches_passes_ref_filter(monkeypatch, branch: str, ref: str) -> None:
    calls: list[list[str]] = []

    def fake_run(cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess:
        calls.append(cmd)
        return completed("[]")

    monkeypatch.setattr(subprocess, "run", fake_run)
    assert mod.list_caches("owner/repo", branch, 500) == []
    assert calls[0][calls[0].index("--ref") + 1] == ref


def test_list_caches_limit_is_not_capped_at_100(monkeypatch) -> None:
    calls: list[list[str]] = []

    def fake_run(cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess:
        calls.append(cmd)
        return completed("[]")

    monkeypatch.setattr(subprocess, "run", fake_run)
    mod.list_caches("owner/repo", "refs/pull/7/merge", 500)
    assert calls[0][calls[0].index("--limit") + 1] == "500"


def test_delete_caches_counts_outcomes(monkeypatch) -> None:
    outcomes = iter([0, 1, 0])

    def fake_run(cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess:
        return completed(returncode=next(outcomes))

    monkeypatch.setattr(subprocess, "run", fake_run)
    deleted, failed = mod.delete_caches("owner/repo", "", ["a", "b", "c"])
    assert (deleted, failed) == (2, 1)


def test_delete_caches_skips_empty_keys(monkeypatch) -> None:
    calls: list[list[str]] = []

    def fake_run(cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess:
        calls.append(cmd)
        return completed(returncode=0)

    monkeypatch.setattr(subprocess, "run", fake_run)
    mod.delete_caches("owner/repo", "", ["", "x", ""])
    assert len(calls) == 1
    assert calls[0][:4] == ["gh", "cache", "delete", "x"]


def test_delete_caches_scopes_to_ref(monkeypatch) -> None:
    calls: list[list[str]] = []

    def fake_run(cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess:
        calls.append(cmd)
        return completed(returncode=0)

    monkeypatch.setattr(subprocess, "run", fake_run)
    assert mod.delete_caches("owner/repo", "refs/pull/7/merge", ["k"]) == (1, 0)
    assert calls[0][calls[0].index("--ref") + 1] == "refs/pull/7/merge"


def test_main_dry_run_lists_without_deleting(monkeypatch, capsys) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setattr(
        mod,
        "list_caches",
        lambda *a, **kw: [_usage("k1", 1), _usage("k2", 2)],
    )

    called = False

    def fail_delete(*_a, **_kw):
        nonlocal called
        called = True
        return (0, 0)

    monkeypatch.setattr(mod, "delete_caches", fail_delete)
    assert mod.main([]) == 0
    assert not called
    assert "Found 2 cache(s)" in capsys.readouterr().out


def test_main_default_limit_covers_rolling_generations(monkeypatch) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    seen: list[int] = []
    monkeypatch.setattr(
        mod,
        "list_caches",
        lambda _repo, _branch, limit: seen.append(limit) or [],
    )

    assert mod.main([]) == 0
    assert seen == [500]


def test_main_delete_invokes_delete(monkeypatch, capsys) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setattr(mod, "list_caches", lambda *a, **kw: [_usage("k1", 1)])
    monkeypatch.setattr(mod, "delete_caches", lambda *a, **kw: (1, 0))
    assert mod.main(["--delete"]) == 0
    assert "Deleted 1 cache(s); 0 failed" in capsys.readouterr().out


def test_main_summary_includes_dry_run_notice(monkeypatch, gh_summary: Path) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setattr(mod, "list_caches", lambda *a, **kw: [_usage("k1", 1)])
    assert mod.main(["--summary"]) == 0
    summary = gh_summary.read_text()
    assert summary.startswith("### Cache cleanup\n\n| Item | Value |")
    assert "| Mode | list (dry run) |" in summary
    assert "| Caches | 1 |" in summary
    assert "| `k1` | 1 MiB | refs/heads/master |" in summary
    assert "Dry run" in summary


def test_main_summary_after_delete_shows_results(monkeypatch, gh_summary: Path) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setattr(mod, "list_caches", lambda *a, **kw: [_usage("k1", 1)])
    monkeypatch.setattr(mod, "delete_caches", lambda *a, **kw: (1, 0))
    assert mod.main(["--delete", "--summary"]) == 0
    summary = gh_summary.read_text()
    assert "| Mode | delete |" in summary
    assert "| Deleted | 1 |" in summary
    assert "Dry run" not in summary


def test_main_reads_branch_and_flags_from_environment(monkeypatch, gh_summary: Path) -> None:
    for name, value in {
        "GH_REPO": "owner/repo",
        "CLEANUP_BRANCH": "refs/pull/7/merge",
        "CLEANUP_DELETE": "true",
        "CLEANUP_SUMMARY": "false",
        "CLEANUP_PRUNE": "false",
    }.items():
        monkeypatch.setenv(name, value)
    listed: list[str] = []
    deleted: list[tuple[str, list[str]]] = []
    monkeypatch.setattr(
        mod,
        "list_caches",
        lambda _repo, branch, _limit: listed.append(branch) or [_usage("k1", 1)],
    )
    monkeypatch.setattr(
        mod, "delete_caches", lambda _repo, branch, keys: deleted.append((branch, keys)) or (1, 0)
    )
    assert mod.main([]) == 0
    assert listed == ["refs/pull/7/merge"]
    assert deleted == [("refs/pull/7/merge", ["k1"])]
    assert gh_summary.read_text() == ""


def test_main_prunes_with_summary_from_environment(monkeypatch, gh_summary: Path) -> None:
    for name, value in {
        "GH_REPO": "owner/repo",
        "CLEANUP_DELETE": "false",
        "CLEANUP_SUMMARY": "true",
        "CLEANUP_PRUNE": "true",
    }.items():
        monkeypatch.setenv(name, value)
    monkeypatch.setattr(mod, "list_caches", lambda *a, **kw: [_usage("avd", 10000)])
    monkeypatch.setattr(mod, "delete_caches", lambda *a, **kw: pytest.fail("dry run deleted"))
    assert mod.main([]) == 0
    assert "| Mode | prune (dry run) |" in gh_summary.read_text()
    assert "Dry run" in gh_summary.read_text()


def _trigger(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, event_name: str, payload: dict[str, Any]
) -> None:
    event = tmp_path / "event.json"
    event.write_text(json.dumps({"repository": {"default_branch": "main"}, **payload}))
    monkeypatch.setenv("GITHUB_EVENT_NAME", event_name)
    monkeypatch.setenv("GITHUB_EVENT_PATH", str(event))
    monkeypatch.setenv("GH_REPO", "owner/repo")


def _record_gh(
    monkeypatch: pytest.MonkeyPatch, caches: list[Any]
) -> tuple[list[str], list[tuple[str, list[str]]]]:
    listed: list[str] = []
    deleted: list[tuple[str, list[str]]] = []
    monkeypatch.setattr(
        mod, "list_caches", lambda _repo, branch, _limit: listed.append(branch) or caches
    )
    monkeypatch.setattr(
        mod, "delete_caches", lambda _repo, branch, keys: deleted.append((branch, keys)) or (1, 0)
    )
    return listed, deleted


def _stale_caches() -> list[Any]:
    """10000 MiB on a feature branch: above the high-water mark, with nothing protected."""
    return [
        _usage("stale-a", 6000, ref="refs/heads/feature", accessed="2026-01-02"),
        _usage("stale-b", 4000, ref="refs/heads/feature", accessed="2026-01-01"),
    ]


def test_schedule_prunes_every_branch_and_deletes_with_a_summary(
    monkeypatch, tmp_path: Path, gh_summary: Path
) -> None:
    _trigger(monkeypatch, tmp_path, "schedule", {"schedule": "17 */6 * * *"})
    listed, deleted = _record_gh(monkeypatch, _stale_caches())

    assert mod.main([]) == 0

    assert listed == [""]
    # Least recently used first, down to the 6500 MiB target.
    assert deleted == [("refs/heads/feature", ["stale-b"])]
    assert "| Mode | prune |" in gh_summary.read_text()


def test_closed_pull_request_deletes_only_its_merge_ref_without_a_summary(
    monkeypatch, tmp_path: Path, gh_summary: Path
) -> None:
    _trigger(
        monkeypatch,
        tmp_path,
        "pull_request_target",
        {"action": "closed", "pull_request": {"number": 42}, "inputs": {"mode": "prune"}},
    )
    listed, deleted = _record_gh(monkeypatch, [_usage("ccache-pr", 1, ref="refs/pull/42/merge")])

    assert mod.main([]) == 0

    assert listed == ["refs/pull/42/merge"]
    assert deleted == [("refs/pull/42/merge", ["ccache-pr"])]
    assert gh_summary.read_text() == ""


@pytest.mark.parametrize(
    ("inputs", "mode", "deletes"),
    [
        # Dispatch defaults: a dry-run prune.
        ({}, "prune (dry run)", False),
        ({"branch": "", "dry-run": "true", "mode": "prune"}, "prune (dry run)", False),
        ({"branch": "", "dry-run": "false", "mode": "prune"}, "prune", True),
        ({"branch": "", "dry-run": False, "mode": "prune"}, "prune", True),
    ],
)
def test_dispatch_prune_honors_dry_run(
    monkeypatch,
    tmp_path: Path,
    gh_summary: Path,
    inputs: dict[str, Any],
    mode: str,
    deletes: bool,
) -> None:
    _trigger(monkeypatch, tmp_path, "workflow_dispatch", {"inputs": inputs})
    listed, deleted = _record_gh(monkeypatch, _stale_caches())

    assert mod.main([]) == 0

    assert listed == [""]
    assert bool(deleted) is deletes
    assert f"| Mode | {mode} |" in gh_summary.read_text()


@pytest.mark.parametrize(
    ("inputs", "mode", "deletes"),
    [
        ({"branch": "feature-x", "dry-run": "true", "mode": "wipe"}, "list (dry run)", False),
        ({"branch": "feature-x", "dry-run": "false", "mode": "wipe"}, "delete", True),
    ],
)
def test_dispatch_wipe_lists_or_deletes_the_selected_branch(
    monkeypatch,
    tmp_path: Path,
    gh_summary: Path,
    inputs: dict[str, Any],
    mode: str,
    deletes: bool,
) -> None:
    _trigger(monkeypatch, tmp_path, "workflow_dispatch", {"inputs": inputs})
    listed, deleted = _record_gh(monkeypatch, [_usage("k1", 1, ref="refs/heads/feature-x")])

    assert mod.main([]) == 0

    assert listed == ["feature-x"]
    assert deleted == ([("feature-x", ["k1"])] if deletes else [])
    summary = gh_summary.read_text()
    assert f"| Mode | {mode} |" in summary
    assert "| Branch | `feature-x` |" in summary


def test_dispatch_wipe_of_every_branch(monkeypatch, tmp_path: Path, gh_summary: Path) -> None:
    _trigger(
        monkeypatch,
        tmp_path,
        "workflow_dispatch",
        {"inputs": {"branch": "", "dry-run": "false", "mode": "wipe"}},
    )
    listed, deleted = _record_gh(monkeypatch, [_usage("k1", 1)])

    assert mod.main([]) == 0

    assert listed == [""]
    assert deleted == [("", ["k1"])]
    assert "| Branch | all |" in gh_summary.read_text()


def test_prune_protects_the_repository_default_branch() -> None:
    schedule = mod.event_defaults("schedule", {"repository": {"default_branch": "main"}})
    assert schedule.default_branch == "main"
    assert mod.event_defaults("schedule", {}).default_branch == "master"


def test_environment_overrides_the_event(monkeypatch, tmp_path: Path, gh_summary: Path) -> None:
    _trigger(monkeypatch, tmp_path, "schedule", {})
    for name, value in {
        "CLEANUP_BRANCH": "refs/pull/7/merge",
        "CLEANUP_DELETE": "false",
        "CLEANUP_SUMMARY": "false",
        "CLEANUP_PRUNE": "false",
    }.items():
        monkeypatch.setenv(name, value)
    listed, deleted = _record_gh(monkeypatch, [_usage("k1", 1)])

    assert mod.main([]) == 0

    assert listed == ["refs/pull/7/merge"]
    assert deleted == []
    assert gh_summary.read_text() == ""


def test_without_an_event_caches_are_only_listed() -> None:
    assert mod.event_defaults("", {}) == mod.CleanupDefaults()
    assert mod.event_defaults("push", {}) == mod.CleanupDefaults()


def test_main_requires_repo(monkeypatch) -> None:
    monkeypatch.delenv("GH_REPO", raising=False)
    with pytest.raises(SystemExit):
        mod.main([])


def _usage(
    key: str, mb: int, *, ref: str = "refs/heads/master", accessed: str = "2026-01-01"
) -> Any:
    return mod.CacheUsage(key=key, ref=ref, size_bytes=mb * 1024 * 1024, last_accessed=accessed)


def test_select_prune_victims_noop_under_high_water() -> None:
    caches = [_usage("ccache-linux", 3000), _usage("avd-Linux", 1000)]
    victims, total, projected = mod.select_prune_victims(
        caches, keep_mb=6500, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert victims == []
    assert total == projected == 4000 * 1024 * 1024


@pytest.mark.parametrize("prefix", ["ccache", "moccache"])
def test_pr_generations_are_bounded_below_high_water(prefix: str) -> None:
    old = _usage(
        f"{prefix}-linux-pr-42-{'a' * 64}-99-2",
        500,
        ref="refs/pull/42/merge",
        accessed="2026-09-02",
    )
    newest = _usage(
        f"{prefix}-linux-pr-42-{'b' * 64}-100-2",
        500,
        ref="refs/pull/42/merge",
        accessed="2026-09-01",
    )
    retry = _usage(f"{prefix}-linux-pr-42-{'b' * 64}-100-1", 500, ref="refs/pull/42/merge")
    # Different matrix legs, PRs, and default-branch entries must survive.
    others = [
        _usage(f"{prefix}-linux-pr-42-{'b' * 64}-simulator-99-1", 500, ref="refs/pull/42/merge"),
        _usage(old.key, 500, ref="refs/pull/43/merge"),
        _usage(old.key, 500),
        _usage("qt-linux-99-1", 500, ref="refs/pull/42/merge"),
    ]
    victims, total, projected = mod.select_prune_victims(
        [old, newest, retry, *others],
        keep_mb=6500,
        high_water_mb=9000,
        protect=mod.DEFAULT_PROTECT,
    )
    assert victims == [old, retry]
    assert total - projected == 1000 * 1024 * 1024


def test_pr_generation_cleanup_still_applies_storage_pressure() -> None:
    caches = [
        _usage("ccache-linux-pr-42-hash-99-1", 1000, ref="refs/pull/42/merge"),
        _usage("ccache-linux-pr-42-hash-100-1", 1000, ref="refs/pull/42/merge"),
        _usage("ccache-linux-shared-hash-100-1", 5000),
        _usage("unprotected", 5000),
    ]
    victims, total, projected = mod.select_prune_victims(
        caches, keep_mb=6500, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert victims == [caches[0], caches[3]]
    assert total == 12000 * 1024 * 1024
    assert projected == 6000 * 1024 * 1024


def test_pr_and_feature_caches_do_not_prevent_reaching_target() -> None:
    caches = [
        _usage("qt-linux", 3000, ref="refs/heads/main"),
        _usage("qt-linux", 3000, ref="refs/pull/42/merge"),
        _usage("ccache-linux-branch-test", 3000, ref="refs/heads/test"),
        _usage("qt-linux", 3000, ref="refs/pull/43/merge"),
    ]
    victims, _, projected = mod.select_prune_victims(
        caches,
        keep_mb=6500,
        high_water_mb=9000,
        protect=mod.DEFAULT_PROTECT,
        default_branch="main",
    )
    assert caches[0] not in victims
    assert len(victims) == 2
    assert projected == 6000 * 1024 * 1024


def test_select_prune_victims_never_evicts_protected() -> None:
    caches = [
        _usage("ccache-linux", 6000),
        _usage("cpm-sources-v3-shared", 1000),
        _usage("avd-Linux", 3000),
    ]
    victims, _total, _projected = mod.select_prune_victims(
        caches, keep_mb=6500, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert [v.key for v in victims] == ["avd-Linux"]


@pytest.mark.parametrize(
    "retired", ["cpm-modules-linux-shared-hash", "cpm-sources-v2-linux-shared-hash"]
)
def test_retired_cpm_caches_are_evictable(retired: str) -> None:
    caches = [_usage("ccache-linux", 6000), _usage(retired, 3000)]
    victims, _total, projected = mod.select_prune_victims(
        caches, keep_mb=6500, high_water_mb=8000, protect=mod.DEFAULT_PROTECT
    )
    assert [v.key for v in victims] == [retired]
    assert projected == 6000 * 1024 * 1024


def test_select_prune_victims_breaks_equal_age_ties_by_size() -> None:
    caches = [
        _usage("ccache-linux", 5000),
        _usage("avd-Linux", 1750),
        _usage("codeql-trap", 1370),
        _usage("gradle-wrapper", 400),
    ]
    victims, _total, projected = mod.select_prune_victims(
        caches, keep_mb=6500, high_water_mb=8000, protect=mod.DEFAULT_PROTECT
    )
    assert [v.key for v in victims] == ["avd-Linux", "codeql-trap"]
    assert projected <= 6500 * 1024 * 1024


def test_pruning_preserves_recent_large_compiler_caches() -> None:
    caches = [
        _usage("old-small", 2000, accessed="2026-01-01"),
        _usage("new-large", 7000, accessed="2026-01-02"),
    ]
    victims, _, projected = mod.select_prune_victims(
        caches, keep_mb=7500, high_water_mb=8000, protect=mod.DEFAULT_PROTECT
    )
    assert [cache.key for cache in victims] == ["old-small"]
    assert projected == 7000 * 1024 * 1024


def test_select_prune_victims_floor_above_keep_when_protected_dominates() -> None:
    caches = [_usage("ccache-linux", 9500), _usage("avd-Linux", 1000)]
    victims, _total, projected = mod.select_prune_victims(
        caches, keep_mb=6500, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert [v.key for v in victims] == ["avd-Linux"]
    assert projected == 9500 * 1024 * 1024


def test_select_prune_victims_keeps_newest_rolling_generation() -> None:
    caches = [
        _usage("ccache-linux-shared-hash-100-1", 3000, accessed="2026-01-01"),
        _usage("ccache-linux-shared-hash-101-1", 3000, accessed="2026-01-02"),
        _usage("avd-Linux", 4000),
    ]
    victims, _total, _projected = mod.select_prune_victims(
        caches, keep_mb=6500, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert [v.key for v in victims] == ["avd-Linux"]


def test_select_prune_victims_can_evict_older_rolling_generation() -> None:
    old_digest = "a" * 64
    new_digest = "b" * 64
    caches = [
        _usage(f"ccache-linux-shared-{old_digest}-100-1", 3000, accessed="2026-01-01"),
        _usage(f"ccache-linux-shared-{new_digest}-101-1", 3000, accessed="2026-01-02"),
        _usage("avd-Linux", 4000),
    ]
    victims, _total, projected = mod.select_prune_victims(
        caches, keep_mb=3000, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert [v.key for v in victims] == [
        "avd-Linux",
        f"ccache-linux-shared-{old_digest}-100-1",
    ]
    assert projected == 3000 * 1024 * 1024


def test_select_prune_victims_can_evict_previous_apt_month() -> None:
    digest = "a" * 64
    caches = [
        _usage(f"apt-debs-ubuntu-24.04-X64-2026-07-{digest}", 3000, accessed="2026-07-01"),
        _usage(f"apt-debs-ubuntu-24.04-X64-2026-08-{digest}", 3000, accessed="2026-08-01"),
        _usage("avd-Linux", 4000),
    ]
    victims, _total, projected = mod.select_prune_victims(
        caches, keep_mb=3000, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert [v.key for v in victims] == [
        "avd-Linux",
        f"apt-debs-ubuntu-24.04-X64-2026-07-{digest}",
    ]
    assert projected == 3000 * 1024 * 1024


@pytest.mark.parametrize("prefix", ["moccache", "qt", "prek-v1", "prek-renovate-v1", "lychee-v1"])
def test_select_prune_victims_protects_dependency_cache(prefix: str) -> None:
    caches = [_usage(f"{prefix}-linux", 7000), _usage("avd-Linux", 3000)]
    victims, _total, projected = mod.select_prune_victims(
        caches, keep_mb=6500, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert [v.key for v in victims] == ["avd-Linux"]
    assert projected == 7000 * 1024 * 1024


@pytest.mark.parametrize("prefix", ["ccache", "cpm-sources-v3", "gst-sdk-v1"])
def test_select_prune_victims_replaces_legacy_static_entry(prefix: str) -> None:
    caches = [
        _usage(f"{prefix}-linux-shared-hash", 4000, accessed="2026-01-01"),
        _usage(f"{prefix}-linux-shared-hash-101-1", 4000, accessed="2026-01-02"),
        _usage("avd-Linux", 2000),
    ]
    victims, _total, _projected = mod.select_prune_victims(
        caches, keep_mb=4000, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert [v.key for v in victims] == [f"{prefix}-linux-shared-hash", "avd-Linux"]


def test_select_prune_victims_keeps_lone_legacy_static_entry() -> None:
    caches = [_usage("ccache-linux-shared-hash", 9000), _usage("avd-Linux", 2000)]
    victims, _total, projected = mod.select_prune_victims(
        caches, keep_mb=6500, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert [v.key for v in victims] == ["avd-Linux"]
    assert projected == 9000 * 1024 * 1024


def test_list_caches_reads_last_access_for_pruning(monkeypatch) -> None:
    payload = '[{"key":"ccache-x","ref":"refs/heads/master","sizeInBytes":1048576,"lastAccessedAt":"2026-01-01"}]'
    monkeypatch.setattr(subprocess, "run", lambda *a, **kw: completed(payload))
    assert mod.list_caches("owner/repo", "", 500) == [_usage("ccache-x", 1, accessed="2026-01-01")]


def test_main_prune_dry_run_does_not_delete(monkeypatch, capsys) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setattr(
        mod, "list_caches", lambda *a, **kw: [_usage("ccache-x", 9000), _usage("avd", 2000)]
    )
    called = False

    def fail_delete(*_a, **_kw):
        nonlocal called
        called = True
        return (0, 0)

    monkeypatch.setattr(mod, "delete_caches", fail_delete)
    assert mod.main(["--prune"]) == 0
    assert not called
    assert "1 eviction candidate(s); deleted 0; failed 0" in capsys.readouterr().out


def test_main_prune_delete_evicts_unprotected(monkeypatch, capsys) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setattr(
        mod, "list_caches", lambda *a, **kw: [_usage("ccache-x", 9000), _usage("avd", 2000)]
    )
    seen: list[list[str]] = []
    monkeypatch.setattr(
        mod, "delete_caches", lambda repo, branch, keys: seen.append(keys) or (len(keys), 0)
    )
    assert mod.main(["--prune", "--delete"]) == 0
    assert seen == [["avd"]]
    assert "deleted 1; failed 0" in capsys.readouterr().out


def test_prune_deletes_only_selected_ref_and_reports_failures(
    monkeypatch, capsys, gh_summary: Path
) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    shared = _usage("qt-linux", 4000)
    first_pr = _usage("qt-linux", 4000, ref="refs/pull/42/merge")
    second_pr = _usage("qt-linux", 4000, ref="refs/pull/43/merge")
    monkeypatch.setattr(mod, "list_caches", lambda *a, **kw: [shared, first_pr, second_pr])
    calls = []

    def delete(repo, branch, keys):
        calls.append((repo, branch, keys))
        return (1, 0) if branch == first_pr.ref else (0, 1)

    monkeypatch.setattr(mod, "delete_caches", delete)
    assert mod.main(["--prune", "--delete", "--summary"]) == 0
    assert calls == [
        ("owner/repo", first_pr.ref, ["qt-linux"]),
        ("owner/repo", second_pr.ref, ["qt-linux"]),
    ]
    out = capsys.readouterr().out
    assert "deleted 1; failed 1" in out
    assert "| Pool | 12,000 MiB → 8,000 MiB |" in gh_summary.read_text()
    assert "| Failed deletions | 1 |" in gh_summary.read_text()
    assert "above target" in out


def test_prune_keeps_the_newest_two_snapshots_of_each_name(
    monkeypatch, artifacts: FakeArtifacts, capsys, gh_summary: Path
) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setattr(mod, "list_caches", lambda *a, **kw: [])
    oldest, older = artifacts.add(SNAPSHOT_A, size_mb=300), artifacts.add(SNAPSHOT_A, size_mb=200)
    artifacts.add(SNAPSHOT_A)
    artifacts.add(SNAPSHOT_A)
    artifacts.add(SNAPSHOT_B)

    assert mod.main(["--prune", "--delete", "--summary"]) == 0

    assert sorted(artifacts.deleted) == [oldest, older]
    assert "superseded 2 (500 MiB); deleted 2 (500 MiB freed); failed 0" in capsys.readouterr().out
    summary = gh_summary.read_text()
    assert "### Compiler-cache snapshot cleanup" in summary
    assert "| Snapshots | 5 under 2 names, 800 MiB |" in summary
    assert "| Kept | 3, 300 MiB: the newest 2 of each name and restore group |" in summary
    assert "| Deleted | 2 |" in summary
    assert "| Freed | 500 MiB |" in summary
    assert f"| {oldest} | `{SNAPSHOT_A}` | 300 MiB |" in summary


@pytest.mark.parametrize(
    "other",
    [
        {"workflow_id": 12},
        {"branch": "Stable_V5.1"},
        {"repository_id": 33},
        {"event": "pull_request"},
    ],
)
def test_a_restore_group_keeps_its_own_newest_snapshots(
    artifacts: FakeArtifacts, other: dict[str, Any]
) -> None:
    # A job restores only from producers of its workflow, branch, repository and event class,
    # so the newest snapshot of the name is no use to the other group's jobs.
    kept = [artifacts.add(SNAPSHOT_A, **other), artifacts.add(SNAPSHOT_A, **other)]
    superseded = artifacts.add(SNAPSHOT_A)
    newest = [artifacts.add(SNAPSHOT_A), artifacts.add(SNAPSHOT_A)]

    snapshots = mod.list_snapshots("owner/repo", SNAPSHOT_A)
    groups = mod.snapshot_restore_groups("owner/repo", snapshots)
    selected = mod.select_superseded_snapshots(snapshots, groups, keep=2)

    assert [snapshot.artifact_id for snapshot in selected] == [superseded]
    assert not {snapshot.artifact_id for snapshot in selected} & {*kept, *newest}


def test_trusted_events_share_one_restore_group(artifacts: FakeArtifacts) -> None:
    superseded = artifacts.add(SNAPSHOT_A, event="schedule")
    artifacts.add(SNAPSHOT_A, event="workflow_dispatch")
    artifacts.add(SNAPSHOT_A, event="push")

    snapshots = mod.list_snapshots("owner/repo", SNAPSHOT_A)
    groups = mod.snapshot_restore_groups("owner/repo", snapshots)

    assert [s.artifact_id for s in mod.select_superseded_snapshots(snapshots, groups, 2)] == [
        superseded
    ]


@pytest.mark.parametrize(
    "name",
    [
        "QGroundControl-x86_64",
        "publish-set",
        "test-results-linux-coverage-unit",
        "coverage-report",
        "compiler-cache",
        "compiler-cache-v2-",
        f"{SNAPSHOT_A}.sbom",
        f"{SNAPSHOT_A[:-1]}",
        f"{SNAPSHOT_A[:-1]}G",
        SNAPSHOT_A.replace("-v2-", "-v1-"),
        SNAPSHOT_A.upper(),
        f"docker-{SNAPSHOT_A}",
    ],
)
def test_prune_never_touches_an_artifact_that_is_not_a_snapshot(
    monkeypatch, artifacts: FakeArtifacts, name: str
) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setattr(mod, "list_caches", lambda *a, **kw: [])
    for _ in range(5):
        artifacts.add(name)

    assert mod.main(["--prune", "--delete"]) == 0

    assert artifacts.deleted == []
    # Only the discovery pages were read: no other name is ever listed or looked up.
    assert {requested for _, requested in artifacts.requests} == {""}


def test_prune_deletes_only_snapshots_of_the_name_when_the_api_ignores_the_name_filter(
    monkeypatch, artifacts: FakeArtifacts
) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setattr(mod, "list_caches", lambda *a, **kw: [])
    for _ in range(3):
        artifacts.add("test-results-linux-coverage-unit")
    artifacts.add(SNAPSHOT_B)
    artifacts.add(SNAPSHOT_B)
    superseded = artifacts.add(SNAPSHOT_A)
    artifacts.add(SNAPSHOT_A)
    artifacts.add(SNAPSHOT_A)
    artifacts.unfiltered = {SNAPSHOT_A}

    assert mod.main(["--prune", "--delete"]) == 0

    assert (1, SNAPSHOT_A) in artifacts.requests
    assert artifacts.deleted == [superseded]


def test_expired_snapshots_are_neither_counted_nor_deleted(artifacts: FakeArtifacts) -> None:
    artifacts.add(SNAPSHOT_A, expired=True)
    live = [artifacts.add(SNAPSHOT_A), artifacts.add(SNAPSHOT_A)]

    snapshots = mod.list_snapshots("owner/repo", SNAPSHOT_A)

    assert sorted(snapshot.artifact_id for snapshot in snapshots) == live


def test_discovery_pages_back_to_the_lookback_and_lists_each_name_completely(
    artifacts: FakeArtifacts,
) -> None:
    # Older than the lookback: on a page the discovery never reads.
    for _ in range(100):
        artifacts.add("QGroundControl-x86_64", hours_ago=60)
    superseded = [artifacts.add(SNAPSHOT_A, hours_ago=50) for _ in range(148)]
    for _ in range(98):
        artifacts.add("test-results-linux", hours_ago=30)
    kept = [artifacts.add(SNAPSHOT_A, hours_ago=20), artifacts.add(SNAPSHOT_A, hours_ago=20)]
    for _ in range(100):
        artifacts.add("coverage-report", hours_ago=2)

    names = mod.recent_snapshot_names("owner/repo", datetime.now(UTC) - timedelta(hours=48))
    snapshots = mod.list_snapshots("owner/repo", SNAPSHOT_A)

    assert names == [SNAPSHOT_A]
    # The third page is wholly older than the lookback and ends the discovery; the name's own
    # list still reaches its snapshots on the pages after it.
    assert artifacts.requests == [(1, ""), (2, ""), (3, ""), (1, SNAPSHOT_A), (2, SNAPSHOT_A)]
    assert sorted(snapshot.artifact_id for snapshot in snapshots) == [*superseded, *kept]


def test_a_failed_listing_page_is_retried(artifacts: FakeArtifacts) -> None:
    artifacts.add(SNAPSHOT_A)
    artifacts.failures = {(1, ""): 2, (1, SNAPSHOT_A): 1}

    names = mod.recent_snapshot_names("owner/repo", datetime.now(UTC) - timedelta(hours=48))

    assert names == [SNAPSHOT_A]
    assert len(mod.list_snapshots("owner/repo", SNAPSHOT_A)) == 1
    assert artifacts.requests == [(1, "")] * 3 + [(1, SNAPSHOT_A)] * 2


def _superseded(artifacts: FakeArtifacts) -> None:
    for _ in range(4):
        artifacts.add(SNAPSHOT_A)


@pytest.mark.parametrize(
    "broken",
    ["discovery", "name listing", "partial name listing", "malformed artifact", "run lookup"],
)
def test_prune_deletes_nothing_when_a_listing_fails(
    monkeypatch, artifacts: FakeArtifacts, gh_summary: Path, broken: str
) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setattr(mod, "list_caches", lambda *a, **kw: [])
    _superseded(artifacts)
    error: type[Exception] = mod.ArtifactListingError
    if broken == "discovery":
        artifacts.failures = {(1, ""): 99}
    elif broken == "name listing":
        artifacts.failures = {(1, SNAPSHOT_A): 99}
    elif broken == "partial name listing":
        artifacts.total_offset = 1
    elif broken == "malformed artifact":
        del artifacts.artifacts[0]["workflow_run"]
    else:
        error = KeyError
        artifacts.runs.clear()

    with pytest.raises(error):
        mod.main(["--prune", "--delete", "--summary"])

    assert artifacts.deleted == []
    assert "Compiler-cache snapshot cleanup" not in gh_summary.read_text()


def test_failed_snapshot_deletions_are_reported(
    monkeypatch, artifacts: FakeArtifacts, capsys, gh_summary: Path
) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setattr(mod, "list_caches", lambda *a, **kw: [])
    _superseded(artifacts)
    artifacts.undeletable = {1000}

    assert mod.main(["--prune", "--delete", "--summary"]) == 0

    assert artifacts.deleted == [1001]
    assert "::warning::Failed to delete 1 compiler-cache snapshot" in capsys.readouterr().out
    summary = gh_summary.read_text()
    assert "| Deleted | 1 |" in summary
    assert "| Failed deletions | 1 |" in summary


@pytest.mark.parametrize(
    ("event_name", "payload", "deletes"),
    [
        ("schedule", {}, True),
        ("workflow_dispatch", {"inputs": {}}, False),
        ("workflow_dispatch", {"inputs": {"dry-run": "true", "mode": "prune"}}, False),
        ("workflow_dispatch", {"inputs": {"dry-run": "false", "mode": "prune"}}, True),
    ],
)
def test_snapshot_prune_follows_the_trigger(
    monkeypatch,
    tmp_path: Path,
    artifacts: FakeArtifacts,
    capsys,
    gh_summary: Path,
    event_name: str,
    payload: dict[str, Any],
    deletes: bool,
) -> None:
    _trigger(monkeypatch, tmp_path, event_name, payload)
    _record_gh(monkeypatch, [])
    _superseded(artifacts)

    assert mod.main([]) == 0

    assert sorted(artifacts.deleted) == ([1000, 1001] if deletes else [])
    # A dry run names what it would delete, in the log and in the summary.
    created = artifacts.artifacts[-1]["created_at"]
    assert f"Superseded snapshot 1000 (100 MiB, {created}, master)" in capsys.readouterr().out
    summary = gh_summary.read_text()
    assert "| Superseded | 2, 200 MiB |" in summary
    assert ("no snapshots were deleted" in summary) is not deletes
    assert ("| Freed | 200 MiB |" in summary) is deletes


@pytest.mark.parametrize(
    ("event_name", "payload"),
    [
        ("pull_request_target", {"action": "closed", "pull_request": {"number": 42}}),
        ("workflow_dispatch", {"inputs": {"dry-run": "false", "mode": "wipe"}}),
    ],
)
def test_only_a_prune_reads_or_deletes_snapshots(
    monkeypatch, tmp_path: Path, artifacts: FakeArtifacts, event_name: str, payload: dict[str, Any]
) -> None:
    _trigger(monkeypatch, tmp_path, event_name, payload)
    _record_gh(monkeypatch, [])
    _superseded(artifacts)

    assert mod.main([]) == 0

    assert artifacts.requests == []
    assert artifacts.deleted == []


def test_fewer_than_two_snapshots_cannot_be_kept(monkeypatch, artifacts: FakeArtifacts) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    with pytest.raises(SystemExit):
        mod.main(["--prune", "--keep-snapshots", "1"])
    with pytest.raises(ValueError, match="at least 2"):
        mod.select_superseded_snapshots([], {}, keep=1)
    assert artifacts.requests == []
