"""Tests for cache_policy.py."""

from __future__ import annotations

from pathlib import Path

import pytest

import cache_policy as mod


def test_explicit_true_passes_through(monkeypatch, capsys, tmp_path: Path) -> None:
    output_file = tmp_path / "gh_output"
    output_file.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    assert mod.main(["--requested", "true"]) == 0
    assert capsys.readouterr().out.strip() == "true"
    assert "save=true" in output_file.read_text()


def test_explicit_false_passes_through(monkeypatch, capsys, tmp_path: Path) -> None:
    output_file = tmp_path / "gh_output"
    output_file.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    assert mod.main(["--requested", "false"]) == 0
    assert capsys.readouterr().out.strip() == "false"
    assert "save=false" in output_file.read_text()


def test_auto_on_push_saves(monkeypatch, capsys, tmp_path: Path) -> None:
    monkeypatch.setenv("EVENT_NAME", "push")
    monkeypatch.setenv("THIS_REPO", "owner/repo")
    monkeypatch.delenv("PR_REPO", raising=False)
    output_file = tmp_path / "gh_output"
    output_file.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    assert mod.main(["--requested", "auto"]) == 0
    assert capsys.readouterr().out.strip() == "true"


def test_auto_on_pull_request_skips(monkeypatch, capsys, tmp_path: Path) -> None:
    monkeypatch.setenv("EVENT_NAME", "pull_request")
    monkeypatch.setenv("PR_REPO", "owner/repo")
    monkeypatch.setenv("THIS_REPO", "owner/repo")
    output_file = tmp_path / "gh_output"
    output_file.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    assert mod.main(["--requested", "auto"]) == 0
    assert capsys.readouterr().out.strip() == "false"


def test_auto_on_fork_pr_skips(monkeypatch, capsys, tmp_path: Path) -> None:
    monkeypatch.setenv("EVENT_NAME", "pull_request")
    monkeypatch.setenv("PR_REPO", "fork/repo")
    monkeypatch.setenv("THIS_REPO", "owner/repo")
    output_file = tmp_path / "gh_output"
    output_file.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    assert mod.main(["--requested", "auto"]) == 0
    assert capsys.readouterr().out.strip() == "false"


def test_requires_requested_arg() -> None:
    with pytest.raises(SystemExit):
        mod.main([])
