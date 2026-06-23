#!/usr/bin/env python3
"""Tests for deploy_docs.py."""

from __future__ import annotations

import subprocess
from typing import TYPE_CHECKING
from unittest.mock import patch

from deploy_docs import deploy_branch, sanitize_branch

if TYPE_CHECKING:
    from pathlib import Path


class TestSanitizeBranch:
    def test_simple(self) -> None:
        assert sanitize_branch("main") == "main"

    def test_slashes(self) -> None:
        assert sanitize_branch("feature/docs-update") == "feature_docs-update"

    def test_special_chars(self) -> None:
        assert sanitize_branch("v1.2.3-rc1") == "v1.2.3-rc1"

    def test_spaces(self) -> None:
        assert sanitize_branch("my branch") == "my_branch"

    def test_preserves_dots_dashes_underscores(self) -> None:
        assert sanitize_branch("release_v2.0-beta") == "release_v2.0-beta"


def _git_init(repo: Path) -> None:
    subprocess.run(["git", "init", "-q", str(repo)], check=True)
    subprocess.run(["git", "-C", str(repo), "config", "user.email", "t@e"], check=True)
    subprocess.run(["git", "-C", str(repo), "config", "user.name", "t"], check=True)
    subprocess.run(["git", "-C", str(repo), "config", "commit.gpgsign", "false"], check=True)
    (repo / "seed").write_text("seed")
    subprocess.run(["git", "-C", str(repo), "add", "seed"], check=True)
    subprocess.run(["git", "-C", str(repo), "commit", "-q", "-m", "seed"], check=True)


class TestDeployBranch:
    """deploy_branch() filesystem + git behavior — push is stubbed."""

    def _common_kwargs(self, source: Path, target: Path) -> dict[str, str]:
        return {
            "branch": "feature/docs",
            "target_branch": "main",
            "commit_message": "Docs update",
            "author_email": "bot@example.test",
            "author_name": "bot",
        }

    def test_copies_files_commits_and_pushes(self, tmp_path: Path) -> None:
        source = tmp_path / "src"
        source.mkdir()
        (source / "index.html").write_text("<html/>")
        (source / "assets").mkdir()
        (source / "assets" / "x.css").write_text("body{}")
        target = tmp_path / "tgt"
        target.mkdir()
        _git_init(target)

        real_run = subprocess.run

        def fake_run(cmd, *args, **kwargs):
            # Intercept the push so the test doesn't hit a remote
            if isinstance(cmd, list) and cmd[:2] == ["git", "push"]:
                return subprocess.CompletedProcess(cmd, 0)
            return real_run(cmd, *args, **kwargs)

        with patch("common.proc.subprocess.run", side_effect=fake_run):
            made_commit = deploy_branch(
                source_dir=source,
                target_dir=target,
                **self._common_kwargs(source, target),
            )
        assert made_commit is True
        assert (target / "feature_docs" / "index.html").read_text() == "<html/>"
        assert (target / "feature_docs" / "assets" / "x.css").read_text() == "body{}"
        log = subprocess.run(
            ["git", "-C", str(target), "log", "--oneline"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout
        assert "Docs update" in log

    def test_returns_false_when_no_changes(self, tmp_path: Path) -> None:
        source = tmp_path / "src"
        source.mkdir()
        (source / "page.html").write_text("hello")
        target = tmp_path / "tgt"
        target.mkdir()
        _git_init(target)

        kwargs = self._common_kwargs(source, target)

        real_run = subprocess.run

        def fake_run(cmd, *args, **kwargs):
            if isinstance(cmd, list) and cmd[:2] == ["git", "push"]:
                return subprocess.CompletedProcess(cmd, 0)
            return real_run(cmd, *args, **kwargs)

        with patch("common.proc.subprocess.run", side_effect=fake_run):
            assert deploy_branch(source_dir=source, target_dir=target, **kwargs) is True
            # Second run with unchanged source — no commit, returns False.
            assert deploy_branch(source_dir=source, target_dir=target, **kwargs) is False
