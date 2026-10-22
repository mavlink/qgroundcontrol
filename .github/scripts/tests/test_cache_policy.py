"""Tests for cache_policy.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

import cache_policy as mod
import pytest

if TYPE_CHECKING:
    from pathlib import Path


def test_explicit_true_passes_through(capsys, gh_output: Path) -> None:
    assert mod.main(["--requested", "true"]) == 0
    assert capsys.readouterr().out.strip() == "true"
    assert "save=true" in gh_output.read_text()


def test_explicit_false_passes_through(capsys, gh_output: Path) -> None:
    assert mod.main(["--requested", "false"]) == 0
    assert capsys.readouterr().out.strip() == "false"
    assert "save=false" in gh_output.read_text()


@pytest.mark.usefixtures("gh_output")
def test_auto_on_push_saves(monkeypatch, capsys) -> None:
    monkeypatch.setenv("EVENT_NAME", "push")
    monkeypatch.setenv("THIS_REPO", "owner/repo")
    monkeypatch.delenv("PR_REPO", raising=False)
    assert mod.main(["--requested", "auto"]) == 0
    assert capsys.readouterr().out.strip() == "true"


@pytest.mark.usefixtures("gh_output")
def test_auto_on_pull_request_saves(monkeypatch, capsys) -> None:
    monkeypatch.setenv("EVENT_NAME", "pull_request")
    monkeypatch.setenv("PR_REPO", "owner/repo")
    monkeypatch.setenv("THIS_REPO", "owner/repo")
    assert mod.main(["--requested", "auto"]) == 0
    assert capsys.readouterr().out.strip() == "true"


@pytest.mark.usefixtures("gh_output")
def test_auto_on_fork_pr_saves(monkeypatch, capsys) -> None:
    monkeypatch.setenv("EVENT_NAME", "pull_request")
    monkeypatch.setenv("PR_REPO", "fork/repo")
    monkeypatch.setenv("THIS_REPO", "owner/repo")
    assert mod.main(["--requested", "auto"]) == 0
    assert capsys.readouterr().out.strip() == "true"


def test_requires_requested_arg() -> None:
    with pytest.raises(SystemExit):
        mod.main([])


@pytest.mark.parametrize("pr_repo", ["owner/repo", "fork/repo"])
def test_auto_on_pull_request_target_skips(monkeypatch, gh_output, pr_repo) -> None:
    monkeypatch.setenv("EVENT_NAME", "pull_request_target")
    monkeypatch.setenv("PR_REPO", pr_repo)
    monkeypatch.setenv("THIS_REPO", "owner/repo")
    assert mod.main(["--requested", "auto"]) == 0
    assert "save=false\n" in gh_output.read_text()


def test_explicit_read_only_overrides_pr_default(monkeypatch, gh_output) -> None:
    monkeypatch.setenv("EVENT_NAME", "pull_request")
    assert mod.main(["--requested", "false"]) == 0
    assert "save=false\n" in gh_output.read_text()


@pytest.mark.parametrize(
    ("event", "backend", "requested", "scoped_save"),
    [
        ("pull_request", "github", "auto", "false"),
        ("pull_request", None, "auto", "false"),
        ("pull_request", "s3", "auto", "true"),
        ("push", "github", "auto", "true"),
        ("pull_request", "github", "true", "true"),
        ("pull_request_target", "s3", "auto", "false"),
    ],
)
def test_scoped_caches_are_restore_only_for_prs_on_github_storage(
    monkeypatch, gh_output, event, backend, requested, scoped_save
) -> None:
    monkeypatch.setenv("EVENT_NAME", event)
    if backend is None:
        monkeypatch.delenv("QGC_ACTIONS_CACHE_BACKEND", raising=False)
    else:
        monkeypatch.setenv("QGC_ACTIONS_CACHE_BACKEND", backend)
    assert mod.main(["--requested", requested]) == 0
    assert f"scoped_save={scoped_save}\n" in gh_output.read_text()
