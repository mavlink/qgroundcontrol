"""Tests for tools/common/git.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from common.git import ZERO_SHA, merge_checkout_parent, pr_diff_refs

from ._helpers import git, pr_merge_checkout

if TYPE_CHECKING:
    from pathlib import Path

    from ._helpers import PrMerge


@pytest.fixture
def pr_merge(tmp_path: Path, isolated_git: None) -> PrMerge:
    return pr_merge_checkout(
        tmp_path,
        base={"a.txt": "base\n"},
        newer_base={"a.txt": "newer base\n", "base.txt": "base only\n"},
        pr={"pr.txt": "pr\n"},
    )


@pytest.mark.parametrize("clone", ["full", "shallow"])
def test_merge_checkout_diffs_against_its_first_parent(pr_merge: PrMerge, clone: str) -> None:
    repo = getattr(pr_merge, clone)
    assert pr_diff_refs(pr_merge.fork_point, pr_merge.pr_head, cwd=repo) == (
        pr_merge.base_tip,
        "HEAD",
    )
    assert git(repo, "diff", "--name-only", f"{pr_merge.base_tip}...HEAD") == "pr.txt"


def test_older_base_sha_would_include_newer_base_changes(pr_merge: PrMerge) -> None:
    changed = git(pr_merge.full, "diff", "--name-only", f"{pr_merge.fork_point}...HEAD")
    assert changed.splitlines() == ["a.txt", "base.txt", "pr.txt"]


def test_non_merge_checkout_keeps_base_and_head(pr_merge: PrMerge) -> None:
    git(pr_merge.full, "checkout", "-q", pr_merge.pr_head)
    refs = pr_diff_refs(pr_merge.fork_point, pr_merge.pr_head, cwd=pr_merge.full)
    assert refs == (pr_merge.fork_point, pr_merge.pr_head)
    assert merge_checkout_parent(pr_merge.pr_head, cwd=pr_merge.full) is None


@pytest.mark.parametrize("head", [None, "", "HEAD"])
def test_head_is_not_a_merge_source(pr_merge: PrMerge, head: str | None) -> None:
    assert merge_checkout_parent(head, cwd=pr_merge.full) is None
    assert pr_diff_refs(pr_merge.base_tip, head, cwd=pr_merge.full) == (pr_merge.base_tip, "HEAD")


def test_unset_base_uses_the_default_branch(pr_merge: PrMerge) -> None:
    git(pr_merge.full, "checkout", "-q", "pr")
    assert pr_diff_refs(None, cwd=pr_merge.full) == ("master", "HEAD")
    assert pr_diff_refs("", "", cwd=pr_merge.full) == ("master", "HEAD")


@pytest.mark.parametrize(
    ("base", "reason"),
    [
        (ZERO_SHA, "New branch push"),
        (None, "Default branch not available"),
        ("missing-base", "No merge base of missing-base and HEAD in the fetched history"),
    ],
)
def test_unusable_bases_select_everything(
    pr_merge: PrMerge, capsys: pytest.CaptureFixture[str], base: str | None, reason: str
) -> None:
    git(pr_merge.full, "branch", "-q", "-m", "master", "topic")
    assert pr_diff_refs(base, cwd=pr_merge.full) is None
    assert capsys.readouterr().err == f"{reason}; checking everything\n"


def test_shallow_clone_without_the_merge_base_selects_everything(
    pr_merge: PrMerge, capsys: pytest.CaptureFixture[str]
) -> None:
    # A depth-2 clone lacks the fork point, so only the merge checkout path can name the PR diff.
    assert pr_diff_refs(pr_merge.fork_point, cwd=pr_merge.shallow) is None
    assert "No merge base" in capsys.readouterr().err
