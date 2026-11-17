"""Tests for tools/qgc_tools/release_tag_check.py."""

from __future__ import annotations

import pytest
from qgc_tools import release_tag_check

from ._helpers import completed


def _stub_git(monkeypatch: pytest.MonkeyPatch, stdout: str, returncode: int = 0) -> None:
    monkeypatch.setattr(release_tag_check, "run_git", lambda *a, **k: completed(stdout, returncode))


def test_lists_containing_stable_branches(monkeypatch: pytest.MonkeyPatch) -> None:
    _stub_git(monkeypatch, "  origin/Stable_V5.1\n\n  origin/Stable_V5.0\n")
    assert release_tag_check.stable_branches_containing() == [
        "origin/Stable_V5.1",
        "origin/Stable_V5.0",
    ]


def test_empty_when_not_on_stable(monkeypatch: pytest.MonkeyPatch) -> None:
    _stub_git(monkeypatch, "")
    assert release_tag_check.stable_branches_containing() == []


def test_raises_when_git_fails(monkeypatch: pytest.MonkeyPatch) -> None:
    _stub_git(monkeypatch, "", returncode=128)
    with pytest.raises(release_tag_check.GitQueryError):
        release_tag_check.stable_branches_containing()


def test_check_release_tag_raises_when_git_fails(monkeypatch: pytest.MonkeyPatch) -> None:
    _stub_git(monkeypatch, "", returncode=128)
    with pytest.raises(release_tag_check.GitQueryError):
        release_tag_check.check_release_tag("v5.1.4")


@pytest.mark.parametrize(
    ("ref_name", "expected"),
    [
        ("v5.1.4", True),
        ("v5.10.12", True),
        ("v5.1.5-rc1", False),
        ("v5.2.0-dev", False),
        ("5.1.4", False),
    ],
)
def test_is_release_tag(ref_name: str, expected: bool) -> None:
    assert release_tag_check.is_release_tag(ref_name) is expected


def test_a_release_version_is_the_tag_s_numbers() -> None:
    version = release_tag_check.release_version
    # Numbers, not text: v5.1.10 is newer than v5.1.6, and v5.10.0 than v5.2.0.
    tags = ("v5.1.6", "v5.1.10", "v5.2.0", "v5.10.0")
    assert [version(tag) for tag in tags] == [(5, 1, 6), (5, 1, 10), (5, 2, 0), (5, 10, 0)]
    assert version("v5.2.0-dev") is None
    assert version("5.1.6") is None


@pytest.mark.parametrize(
    ("ref_name", "stdout", "expected"),
    [
        ("v5.1.4", "origin/Stable_V5.1\n", True),
        ("v5.1.4", "", False),
        ("v5.1.5-rc1", "origin/Stable_V5.1\n", False),
    ],
)
def test_check_release_tag(
    monkeypatch: pytest.MonkeyPatch, ref_name: str, stdout: str, expected: bool
) -> None:
    _stub_git(monkeypatch, stdout)
    assert release_tag_check.check_release_tag(ref_name) is expected


def test_check_release_tag_asks_about_the_tags_commit(monkeypatch: pytest.MonkeyPatch) -> None:
    # Publish Builds runs in a checkout of the default branch, where HEAD is not the tag.
    asked: list[tuple[str, ...]] = []

    def run_git(*args: str, **_: object):
        asked.append(args)
        return completed("origin/Stable_V5.1\n")

    monkeypatch.setattr(release_tag_check, "run_git", run_git)

    assert release_tag_check.check_release_tag("v5.1.5", commit="3a67d31") is True
    assert release_tag_check.check_release_tag("v5.1.5") is True
    assert [args[3] for args in asked] == ["3a67d31", "HEAD"]
