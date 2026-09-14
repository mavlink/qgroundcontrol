"""Tests for gh_cache_cleanup.py."""

from __future__ import annotations

import subprocess
from typing import TYPE_CHECKING, Any

import gh_cache_cleanup as mod
import pytest
from _helpers import completed

if TYPE_CHECKING:
    from pathlib import Path


def test_list_caches_parses_tab_separated(monkeypatch) -> None:
    calls: list[list[str]] = []

    def fake_run(cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess:
        calls.append(cmd)
        return completed(
            "key-1\t100MB\trefs/heads/main\t2026-02-24\nkey-2\t50MB\trefs/pull/1/merge\t2026-02-25\n"
        )

    monkeypatch.setattr(subprocess, "run", fake_run)
    rows = mod.list_caches("owner/repo", "")
    assert [r.key for r in rows] == ["key-1", "key-2"]
    assert rows[0].size == "100MB"
    assert rows[1].ref == "refs/pull/1/merge"
    assert "-B" not in calls[0]


def test_list_caches_passes_branch_filter(monkeypatch) -> None:
    calls: list[list[str]] = []

    def fake_run(cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess:
        calls.append(cmd)
        return completed("")

    monkeypatch.setattr(subprocess, "run", fake_run)
    mod.list_caches("owner/repo", "feature-x")
    assert calls[0][calls[0].index("-B") + 1] == "feature-x"


def test_list_caches_falls_back_to_whitespace_split(monkeypatch) -> None:
    monkeypatch.setattr(
        subprocess, "run", lambda *a, **kw: completed("key1  10MB  refs/heads/main")
    )
    rows = mod.list_caches("owner/repo", "")
    assert len(rows) == 1
    assert rows[0].key == "key1"


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


def test_main_dry_run_writes_count_and_zero_deleted(monkeypatch, gh_output: Path) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setattr(
        mod,
        "list_caches",
        lambda *a, **kw: [
            mod.CacheRow(key="k1", size="1MB", ref="refs/heads/main"),
            mod.CacheRow(key="k2", size="2MB", ref="refs/heads/main"),
        ],
    )

    called = False

    def fail_delete(*_a, **_kw):
        nonlocal called
        called = True
        return (0, 0)

    monkeypatch.setattr(mod, "delete_caches", fail_delete)
    assert mod.main([]) == 0
    assert not called
    contents = gh_output.read_text()
    assert "count=2" in contents
    assert "deleted=0" in contents


def test_main_default_limit_covers_rolling_generations(monkeypatch, gh_output: Path) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    seen: list[int] = []
    monkeypatch.setattr(
        mod,
        "list_caches",
        lambda _repo, _branch, limit: seen.append(limit) or [],
    )

    assert mod.main([]) == 0
    assert seen == [500]


def test_main_delete_invokes_delete(monkeypatch, gh_output: Path) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setattr(
        mod, "list_caches", lambda *a, **kw: [mod.CacheRow(key="k1", size="1MB", ref="main")]
    )
    monkeypatch.setattr(mod, "delete_caches", lambda *a, **kw: (1, 0))
    assert mod.main(["--delete"]) == 0
    assert "deleted=1" in gh_output.read_text()


@pytest.mark.usefixtures("gh_output")
def test_main_summary_includes_dry_run_notice(monkeypatch, tmp_path: Path) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    summary_file = tmp_path / "step_summary"
    summary_file.write_text("")
    monkeypatch.setenv("GITHUB_STEP_SUMMARY", str(summary_file))
    monkeypatch.setattr(mod, "list_caches", lambda *a, **kw: [mod.CacheRow("k1", "1MB", "main")])
    assert mod.main(["--summary"]) == 0
    summary = summary_file.read_text()
    assert "## Cache Summary" in summary
    assert "Dry run" in summary


@pytest.mark.usefixtures("gh_output")
def test_main_summary_after_delete_shows_results(monkeypatch, tmp_path: Path) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    summary_file = tmp_path / "step_summary"
    summary_file.write_text("")
    monkeypatch.setenv("GITHUB_STEP_SUMMARY", str(summary_file))
    monkeypatch.setattr(mod, "list_caches", lambda *a, **kw: [mod.CacheRow("k1", "1MB", "main")])
    monkeypatch.setattr(mod, "delete_caches", lambda *a, **kw: (1, 0))
    assert mod.main(["--delete", "--summary"]) == 0
    summary = summary_file.read_text()
    assert "Deletion Results" in summary
    assert "Deleted: 1" in summary
    assert "Dry run" not in summary


def test_main_requires_repo(monkeypatch) -> None:
    monkeypatch.delenv("GH_REPO", raising=False)
    monkeypatch.delenv("GITHUB_REPOSITORY", raising=False)
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


@pytest.mark.parametrize("prefix", ["ccache", "moccache", "cpm-modules"])
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


def test_baseline_retention_uses_publication_order_not_recent_reads() -> None:
    old = _usage(f"build-baseline-v2-{'a' * 40}-99-2", 1000, accessed="2026-09-02")
    latest = _usage(f"build-baseline-v2-{'b' * 40}-100-1", 1000, accessed="2026-09-01")
    caches = [old, latest, _usage("ccache-linux", 5000), _usage("other", 4000)]
    victims, _, projected = mod.select_prune_victims(
        caches, keep_mb=6500, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert old in victims
    assert latest not in victims
    assert projected == 6000 * 1024 * 1024


def test_select_prune_victims_never_evicts_protected() -> None:
    caches = [
        _usage("ccache-linux", 6000),
        _usage("cpm-modules-shared", 1000),
        _usage("avd-Linux", 3000),
    ]
    victims, _total, _projected = mod.select_prune_victims(
        caches, keep_mb=6500, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert [v.key for v in victims] == ["avd-Linux"]


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


@pytest.mark.parametrize("prefix", ["moccache", "qt"])
def test_select_prune_victims_protects_dependency_cache(prefix: str) -> None:
    caches = [_usage(f"{prefix}-linux", 7000), _usage("avd-Linux", 3000)]
    victims, _total, projected = mod.select_prune_victims(
        caches, keep_mb=6500, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert [v.key for v in victims] == ["avd-Linux"]
    assert projected == 7000 * 1024 * 1024


@pytest.mark.parametrize("prefix", ["ccache", "cpm-modules", "cpm-sources-v2", "gst-sdk-v1"])
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


def test_list_caches_usage_parses_json(monkeypatch) -> None:
    payload = '[{"key":"ccache-x","ref":"refs/heads/master","sizeInBytes":1048576,"lastAccessedAt":"2026-01-01"}]'
    monkeypatch.setattr(subprocess, "run", lambda *a, **kw: completed(payload))
    rows = mod.list_caches_usage("owner/repo")
    assert rows[0].key == "ccache-x"
    assert rows[0].size_bytes == 1048576


def test_main_prune_dry_run_does_not_delete(monkeypatch, gh_output: Path) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setattr(
        mod, "list_caches_usage", lambda *a, **kw: [_usage("ccache-x", 9000), _usage("avd", 2000)]
    )
    called = False

    def fail_delete(*_a, **_kw):
        nonlocal called
        called = True
        return (0, 0)

    monkeypatch.setattr(mod, "delete_caches", fail_delete)
    assert mod.main(["--prune"]) == 0
    assert not called
    assert "deleted=0" in gh_output.read_text()


def test_main_prune_delete_evicts_unprotected(monkeypatch, gh_output: Path) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setattr(
        mod, "list_caches_usage", lambda *a, **kw: [_usage("ccache-x", 9000), _usage("avd", 2000)]
    )
    seen: list[list[str]] = []
    monkeypatch.setattr(
        mod, "delete_caches", lambda repo, branch, keys: seen.append(keys) or (len(keys), 0)
    )
    assert mod.main(["--prune", "--delete"]) == 0
    assert seen == [["avd"]]
    assert "deleted=1" in gh_output.read_text()


def test_prune_deletes_only_selected_ref_and_reports_failures(
    monkeypatch, gh_output: Path, tmp_path: Path, capsys
) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    summary = tmp_path / "summary.md"
    monkeypatch.setenv("GITHUB_STEP_SUMMARY", str(summary))
    shared = _usage("qt-linux", 4000)
    first_pr = _usage("qt-linux", 4000, ref="refs/pull/42/merge")
    second_pr = _usage("qt-linux", 4000, ref="refs/pull/43/merge")
    monkeypatch.setattr(mod, "list_caches_usage", lambda *a, **kw: [shared, first_pr, second_pr])
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
    assert "failed=1" in gh_output.read_text()
    assert "deleted=1" in gh_output.read_text()
    assert "8000 MiB" in summary.read_text()
    assert "Failed: 1" in summary.read_text()
    assert "above target" in capsys.readouterr().out
