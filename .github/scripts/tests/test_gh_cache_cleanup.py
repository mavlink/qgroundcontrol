"""Tests for gh_cache_cleanup.py."""

from __future__ import annotations

import subprocess
from typing import TYPE_CHECKING, Any

import gh_cache_cleanup as mod
import pytest

if TYPE_CHECKING:
    from pathlib import Path


def _completed(stdout: str = "", returncode: int = 0) -> subprocess.CompletedProcess:
    return subprocess.CompletedProcess(args=[], returncode=returncode, stdout=stdout, stderr="")


def test_list_caches_parses_tab_separated(monkeypatch) -> None:
    calls: list[list[str]] = []

    def fake_run(cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess:
        calls.append(cmd)
        return _completed(
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
        return _completed("")

    monkeypatch.setattr(subprocess, "run", fake_run)
    mod.list_caches("owner/repo", "feature-x")
    assert calls[0][calls[0].index("-B") + 1] == "feature-x"


def test_list_caches_falls_back_to_whitespace_split(monkeypatch) -> None:
    monkeypatch.setattr(
        subprocess, "run", lambda *a, **kw: _completed("key1  10MB  refs/heads/main")
    )
    rows = mod.list_caches("owner/repo", "")
    assert len(rows) == 1
    assert rows[0].key == "key1"


def test_delete_caches_counts_outcomes(monkeypatch) -> None:
    outcomes = iter([0, 1, 0])

    def fake_run(cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess:
        return _completed(returncode=next(outcomes))

    monkeypatch.setattr(subprocess, "run", fake_run)
    deleted, failed = mod.delete_caches("owner/repo", "", ["a", "b", "c"])
    assert (deleted, failed) == (2, 1)


def test_delete_caches_skips_empty_keys(monkeypatch) -> None:
    calls: list[list[str]] = []

    def fake_run(cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess:
        calls.append(cmd)
        return _completed(returncode=0)

    monkeypatch.setattr(subprocess, "run", fake_run)
    mod.delete_caches("owner/repo", "", ["", "x", ""])
    assert len(calls) == 1


def test_main_dry_run_writes_count_and_zero_deleted(monkeypatch, tmp_path: Path) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    output_file = tmp_path / "gh_output"
    output_file.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
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
    contents = output_file.read_text()
    assert "count=2" in contents
    assert "deleted=0" in contents


def test_main_delete_invokes_delete(monkeypatch, tmp_path: Path) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    output_file = tmp_path / "gh_output"
    output_file.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    monkeypatch.setattr(
        mod, "list_caches", lambda *a, **kw: [mod.CacheRow(key="k1", size="1MB", ref="main")]
    )
    monkeypatch.setattr(mod, "delete_caches", lambda *a, **kw: (1, 0))
    assert mod.main(["--delete"]) == 0
    contents = output_file.read_text()
    assert "deleted=1" in contents


def test_main_summary_includes_dry_run_notice(monkeypatch, tmp_path: Path) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    output_file = tmp_path / "gh_output"
    output_file.write_text("")
    summary_file = tmp_path / "step_summary"
    summary_file.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    monkeypatch.setenv("GITHUB_STEP_SUMMARY", str(summary_file))
    monkeypatch.setattr(mod, "list_caches", lambda *a, **kw: [mod.CacheRow("k1", "1MB", "main")])
    assert mod.main(["--summary"]) == 0
    summary = summary_file.read_text()
    assert "## Cache Summary" in summary
    assert "Dry run" in summary


def test_main_summary_after_delete_shows_results(monkeypatch, tmp_path: Path) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    output_file = tmp_path / "gh_output"
    output_file.write_text("")
    summary_file = tmp_path / "step_summary"
    summary_file.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
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


def test_select_prune_victims_largest_first_until_target() -> None:
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


def test_select_prune_victims_floor_above_keep_when_protected_dominates() -> None:
    caches = [_usage("ccache-linux", 9500), _usage("avd-Linux", 1000)]
    victims, _total, projected = mod.select_prune_victims(
        caches, keep_mb=6500, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert [v.key for v in victims] == ["avd-Linux"]
    assert projected == 9500 * 1024 * 1024


def test_list_caches_usage_parses_json(monkeypatch) -> None:
    payload = '[{"key":"ccache-x","ref":"refs/heads/master","sizeInBytes":1048576,"lastAccessedAt":"2026-01-01"}]'
    monkeypatch.setattr(subprocess, "run", lambda *a, **kw: _completed(payload))
    rows = mod.list_caches_usage("owner/repo")
    assert rows[0].key == "ccache-x"
    assert rows[0].size_bytes == 1048576


def test_main_prune_dry_run_does_not_delete(monkeypatch, tmp_path: Path) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    output_file = tmp_path / "gh_output"
    output_file.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
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
    assert "deleted=0" in output_file.read_text()


def test_main_prune_delete_evicts_unprotected(monkeypatch, tmp_path: Path) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    output_file = tmp_path / "gh_output"
    output_file.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    monkeypatch.setattr(
        mod, "list_caches_usage", lambda *a, **kw: [_usage("ccache-x", 9000), _usage("avd", 2000)]
    )
    seen: list[list[str]] = []
    monkeypatch.setattr(
        mod, "delete_caches", lambda repo, branch, keys: seen.append(keys) or (len(keys), 0)
    )
    assert mod.main(["--prune", "--delete"]) == 0
    assert seen == [["avd"]]
    assert "deleted=1" in output_file.read_text()
