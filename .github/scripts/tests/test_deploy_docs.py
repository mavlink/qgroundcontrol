#!/usr/bin/env python3
"""Documentation deployment filesystem and Git contracts."""

from __future__ import annotations

import subprocess
from typing import TYPE_CHECKING

from deploy_docs import deploy_branch, sanitize_branch

if TYPE_CHECKING:
    from pathlib import Path

    import pytest


def test_branch_sanitization() -> None:
    expected = {
        "main": "main",
        "feature/docs-update": "feature_docs-update",
        "v1.2.3-rc1": "v1.2.3-rc1",
        "my branch": "my_branch",
        "release_v2.0-beta": "release_v2.0-beta",
    }
    for branch, safe_branch in expected.items():
        assert sanitize_branch(branch) == safe_branch


def _git_init(repo: Path) -> None:
    subprocess.run(["git", "init", "-q", str(repo)], check=True)
    for key, value in (("user.email", "t@e"), ("user.name", "t"), ("commit.gpgsign", "false")):
        subprocess.run(["git", "-C", str(repo), "config", key, value], check=True)
    (repo / "seed").write_text("seed")
    subprocess.run(["git", "-C", str(repo), "add", "seed"], check=True)
    subprocess.run(["git", "-C", str(repo), "commit", "-q", "-m", "seed"], check=True)


def test_deploy_copies_nested_files_commits_once_and_pushes(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    source = tmp_path / "src"
    target = tmp_path / "target"
    (source / "assets").mkdir(parents=True)
    target.mkdir()
    (source / "index.html").write_text("<html/>")
    (source / "assets" / "x.css").write_text("body{}")
    _git_init(target)

    real_run = subprocess.run

    def fake_run(cmd, *args, **kwargs):
        if isinstance(cmd, list) and cmd[:2] == ["git", "push"]:
            return subprocess.CompletedProcess(cmd, 0)
        return real_run(cmd, *args, **kwargs)

    monkeypatch.setattr("common.proc.subprocess.run", fake_run)
    kwargs = {
        "source_dir": source,
        "target_dir": target,
        "branch": "feature/docs",
        "target_branch": "main",
        "commit_message": "Docs update",
        "author_email": "bot@example.test",
        "author_name": "bot",
    }
    assert deploy_branch(**kwargs) is True
    assert deploy_branch(**kwargs) is False
    assert (target / "feature_docs" / "index.html").read_text() == "<html/>"
    assert (target / "feature_docs" / "assets" / "x.css").read_text() == "body{}"
    log = subprocess.run(
        ["git", "-C", str(target), "log", "--oneline"],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    assert "Docs update" in log
