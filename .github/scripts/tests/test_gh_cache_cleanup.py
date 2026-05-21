"""Tests for gh_cache_cleanup.py."""

from __future__ import annotations

import subprocess
from pathlib import Path
from typing import Any

import pytest

import gh_cache_cleanup as mod


def _completed(stdout: str = "", returncode: int = 0) -> subprocess.CompletedProcess:
    return subprocess.CompletedProcess(args=[], returncode=returncode, stdout=stdout, stderr="")


def test_list_caches_parses_tab_separated(monkeypatch) -> None:
    calls: list[list[str]] = []

    def fake_run(cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess:
        calls.append(cmd)
        return _completed("key-1\t100MB\trefs/heads/main\t2026-02-24\nkey-2\t50MB\trefs/pull/1/merge\t2026-02-25\n")

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
    monkeypatch.setattr(subprocess, "run", lambda *a, **kw: _completed("key1  10MB  refs/heads/main"))
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
    monkeypatch.setattr(mod, "list_caches", lambda *a, **kw: [
        mod.CacheRow(key="k1", size="1MB", ref="refs/heads/main"),
        mod.CacheRow(key="k2", size="2MB", ref="refs/heads/main"),
    ])

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
    monkeypatch.setattr(mod, "list_caches", lambda *a, **kw: [mod.CacheRow(key="k1", size="1MB", ref="main")])
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
