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
def test_auto_on_pull_request_skips(monkeypatch, capsys) -> None:
    monkeypatch.setenv("EVENT_NAME", "pull_request")
    monkeypatch.setenv("PR_REPO", "owner/repo")
    monkeypatch.setenv("THIS_REPO", "owner/repo")
    assert mod.main(["--requested", "auto"]) == 0
    assert capsys.readouterr().out.strip() == "false"


@pytest.mark.usefixtures("gh_output")
def test_auto_on_fork_pr_skips(monkeypatch, capsys) -> None:
    monkeypatch.setenv("EVENT_NAME", "pull_request")
    monkeypatch.setenv("PR_REPO", "fork/repo")
    monkeypatch.setenv("THIS_REPO", "owner/repo")
    assert mod.main(["--requested", "auto"]) == 0
    assert capsys.readouterr().out.strip() == "false"


def test_requires_requested_arg() -> None:
    with pytest.raises(SystemExit):
        mod.main([])
