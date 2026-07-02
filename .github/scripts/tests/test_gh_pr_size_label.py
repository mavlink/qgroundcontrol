"""Tests for gh_pr_size_label.py."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

import gh_pr_size_label as mod
import pytest
from _helpers import completed

if TYPE_CHECKING:
    import subprocess
    from pathlib import Path


def test_list_size_labels_filters_to_size_prefix(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(mod, "gh", lambda *a, **kw: completed("size/M\nsize/L\n"))
    assert mod.list_size_labels("owner/repo", "42") == ["size/L", "size/M"]

def test_list_size_labels_empty_when_none(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(mod, "gh", lambda *a, **kw: completed(""))
    assert mod.list_size_labels("owner/repo", "42") == []

def test_list_size_labels_exits_on_gh_failure(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(mod, "gh", lambda *a, **kw: completed("", returncode=1, stderr="boom"))
    with pytest.raises(SystemExit):
        mod.list_size_labels("owner/repo", "42")

def test_remove_label_url_encodes_slash(monkeypatch: pytest.MonkeyPatch) -> None:
    calls: list[tuple[Any, ...]] = []

    def fake_gh(*args: str, **kwargs: Any) -> subprocess.CompletedProcess:
        calls.append(args)
        return completed()

    monkeypatch.setattr(mod, "gh", fake_gh)
    assert mod.remove_label("owner/repo", "42", "size/XL") is True
    assert "repos/owner/repo/issues/42/labels/size%2FXL" in calls[0]

def test_remove_label_treats_404_as_success(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(
        mod, "gh", lambda *a, **kw: completed(returncode=1, stderr="HTTP 404: Not Found"),
    )
    assert mod.remove_label("owner/repo", "42", "size/M") is True

def test_remove_label_returns_false_on_other_errors(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(
        mod, "gh", lambda *a, **kw: completed(returncode=1, stderr="HTTP 500: internal"),
    )
    assert mod.remove_label("owner/repo", "42", "size/M") is False

def test_cmd_current_writes_label(gh_output: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setenv("PR_NUMBER", "42")
    monkeypatch.setattr(mod, "gh", lambda *a, **kw: completed("size/M\n"))

    assert mod.main(["current"]) == 0
    assert "label=size/M\n" in gh_output.read_text()

def test_cmd_current_writes_empty_when_no_label(gh_output: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setenv("PR_NUMBER", "42")
    monkeypatch.setattr(mod, "gh", lambda *a, **kw: completed(""))

    assert mod.main(["current"]) == 0
    assert "label=\n" in gh_output.read_text()

def test_cmd_prune_noop_when_one_label(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setenv("PR_NUMBER", "42")
    monkeypatch.setattr(mod, "list_size_labels", lambda *_: ["size/M"])
    removed: list[str] = []
    monkeypatch.setattr(mod, "remove_label", lambda repo, pr, label: removed.append(label) or True)
    assert mod.main(["prune"]) == 0
    assert removed == []

def test_cmd_prune_removes_old_label_when_multiple(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setenv("PR_NUMBER", "42")
    monkeypatch.setenv("OLD_LABEL", "size/S")
    monkeypatch.setattr(mod, "list_size_labels", lambda *_: ["size/M", "size/S"])
    removed: list[str] = []
    monkeypatch.setattr(mod, "remove_label", lambda repo, pr, label: removed.append(label) or True)
    assert mod.main(["prune"]) == 0
    assert removed == ["size/S"]

def test_cmd_prune_falls_back_to_alphabetic_when_old_label_missing(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setenv("PR_NUMBER", "42")
    monkeypatch.delenv("OLD_LABEL", raising=False)
    monkeypatch.setattr(mod, "list_size_labels", lambda *_: ["size/L", "size/M", "size/XL"])
    removed: list[str] = []
    monkeypatch.setattr(mod, "remove_label", lambda repo, pr, label: removed.append(label) or True)
    assert mod.main(["prune"]) == 0
    assert removed == ["size/M", "size/XL"]

def test_main_exits_when_repo_missing(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.delenv("GH_REPO", raising=False)
    monkeypatch.delenv("GITHUB_REPOSITORY", raising=False)
    with pytest.raises(SystemExit):
        mod.main(["current", "--pr-number", "42"])

def test_main_exits_when_pr_number_missing(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.delenv("PR_NUMBER", raising=False)
    with pytest.raises(SystemExit):
        mod.main(["current"])
