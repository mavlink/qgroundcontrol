"""Tests for gh_cache_cleanup.py."""

from __future__ import annotations

import json
import subprocess
from typing import TYPE_CHECKING, Any

import pytest
from ci.cache import gh_cache_cleanup as mod

from .._helpers import completed

if TYPE_CHECKING:
    from pathlib import Path


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
        _usage("cpm-sources-v2-shared", 1000),
        _usage("avd-Linux", 3000),
    ]
    victims, _total, _projected = mod.select_prune_victims(
        caches, keep_mb=6500, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert [v.key for v in victims] == ["avd-Linux"]


def test_retired_cpm_modules_caches_are_evictable() -> None:
    caches = [_usage("ccache-linux", 6000), _usage("cpm-modules-linux-shared-hash", 3000)]
    victims, _total, projected = mod.select_prune_victims(
        caches, keep_mb=6500, high_water_mb=8000, protect=mod.DEFAULT_PROTECT
    )
    assert [v.key for v in victims] == ["cpm-modules-linux-shared-hash"]
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


@pytest.mark.parametrize("prefix", ["ccache", "cpm-sources-v2", "gst-sdk-v1"])
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
