"""Tests for release_tag_check.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
import release_tag_check
from _helpers import completed

if TYPE_CHECKING:
    from pathlib import Path


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


def test_main_fails_when_git_fails(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    _stub_git(monkeypatch, "", returncode=128)
    monkeypatch.setenv("GITHUB_REF_NAME", "v5.1.4")
    output = tmp_path / "output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output))
    assert release_tag_check.main([]) == 1
    assert not output.exists()


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


@pytest.mark.parametrize(
    ("ref_name", "stdout", "expected"),
    [
        ("v5.1.4", "origin/Stable_V5.1\n", "true"),
        ("v5.1.4", "", "false"),
        ("v5.1.5-rc1", "origin/Stable_V5.1\n", "false"),
    ],
)
def test_main_writes_release_tag_output(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, ref_name: str, stdout: str, expected: str
) -> None:
    _stub_git(monkeypatch, stdout)
    monkeypatch.setenv("GITHUB_REF_NAME", ref_name)
    output = tmp_path / "output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output))
    assert release_tag_check.main([]) == 0
    assert f"release_tag={expected}" in output.read_text()
