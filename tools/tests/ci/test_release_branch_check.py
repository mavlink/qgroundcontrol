"""Tests for tools/ci/publish/release_branch_check.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from ci.publish import release_branch_check

from .._helpers import git

if TYPE_CHECKING:
    from pathlib import Path


@pytest.fixture
def clone(tmp_path: Path, isolated_git: None, monkeypatch: pytest.MonkeyPatch) -> Path:
    """A clone of an origin whose master is one commit ahead of Stable_V5.0."""
    origin = tmp_path / "origin"
    origin.mkdir()
    git(origin, "init", "-q")
    git(origin, "commit", "-q", "--allow-empty", "-m", "stable")
    git(origin, "branch", "Stable_V5.0")
    git(origin, "commit", "-q", "--allow-empty", "-m", "master only")
    checkout = tmp_path / "clone"
    git(tmp_path, "clone", "-q", str(origin), str(checkout))
    monkeypatch.chdir(checkout)
    return checkout


def test_refuses_a_commit_on_no_stable_branch(
    clone: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    # A local branch is not enough: the platform workflows check origin/Stable*.
    git(clone, "branch", "Stable_V5.1")
    assert release_branch_check.main([]) == 1
    out = capsys.readouterr().out
    assert out.startswith("::error::Releases must be cut from a commit on a Stable* branch")
    assert "its release would have no builds" in out


@pytest.mark.parametrize("ref", ["HEAD~1", "origin/Stable_V5.0"])
def test_accepts_a_commit_on_a_stable_branch(
    clone: Path, ref: str, capsys: pytest.CaptureFixture[str]
) -> None:
    assert release_branch_check.main(["--ref", ref]) == 0
    assert capsys.readouterr().out == f"{ref} is on Stable branch(es): origin/Stable_V5.0\n"


def test_checks_the_commit_an_annotated_release_tag_points_at(
    clone: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    # release.yml passes refs/tags/<tag>; annotated tags must resolve to their commit.
    git(clone, "tag", "-a", "v5.0.1", "-m", "v5.0.1", "HEAD~1")
    git(clone, "tag", "-a", "v5.1.0", "-m", "v5.1.0", "HEAD")
    assert release_branch_check.main(["--ref", "refs/tags/v5.0.1"]) == 0
    assert release_branch_check.main(["--ref", "refs/tags/v5.1.0"]) == 1
    assert "refs/tags/v5.1.0 is not on any origin/Stable* branch" in capsys.readouterr().out


def test_accepts_master_once_a_stable_branch_contains_it(
    clone: Path, tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    git(tmp_path / "origin", "branch", "-f", "Stable_V5.0", "master")
    git(clone, "fetch", "-q", "origin")
    assert release_branch_check.main(["--ref", git(clone, "rev-parse", "HEAD")]) == 0
    assert "origin/Stable_V5.0" in capsys.readouterr().out


def test_fails_when_git_cannot_answer(clone: Path, capsys: pytest.CaptureFixture[str]) -> None:
    assert release_branch_check.main(["--ref", "no-such-ref"]) == 1
    assert capsys.readouterr().out.startswith(
        "::error::Cannot determine whether no-such-ref is on a Stable* branch"
    )
