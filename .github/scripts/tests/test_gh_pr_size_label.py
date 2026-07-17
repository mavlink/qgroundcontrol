"""PR size-label discovery and pruning contracts."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

import gh_pr_size_label as mod
import pytest
from _helpers import completed

if TYPE_CHECKING:
    import subprocess
    from pathlib import Path


def test_list_size_labels_filters_sorts_and_surfaces_api_failures(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    for output, expected in (("size/M\nsize/L\n", ["size/L", "size/M"]), ("", [])):
        monkeypatch.setattr(mod, "gh", lambda *_args, output=output, **_kwargs: completed(output))
        assert mod.list_size_labels("owner/repo", "42") == expected
    monkeypatch.setattr(mod, "gh", lambda *_args, **_kwargs: completed(returncode=1, stderr="boom"))
    with pytest.raises(SystemExit):
        mod.list_size_labels("owner/repo", "42")


def test_remove_label_encodes_names_and_handles_idempotent_404(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    calls: list[tuple[Any, ...]] = []

    def fake_gh(*args: str, **_kwargs: Any) -> subprocess.CompletedProcess[str]:
        calls.append(args)
        return completed()

    monkeypatch.setattr(mod, "gh", fake_gh)
    assert mod.remove_label("owner/repo", "42", "size/XL") is True
    assert "repos/owner/repo/issues/42/labels/size%2FXL" in calls[0]

    for stderr, expected in (("HTTP 404: Not Found", True), ("HTTP 500: internal", False)):
        monkeypatch.setattr(
            mod,
            "gh",
            lambda *_args, stderr=stderr, **_kwargs: completed(returncode=1, stderr=stderr),
        )
        assert mod.remove_label("owner/repo", "42", "size/M") is expected


def test_current_command_writes_present_or_empty_label(
    gh_output: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setenv("PR_NUMBER", "42")
    for response, expected in (("size/M\n", "label=size/M\n"), ("", "label=\n")):
        gh_output.write_text("")
        monkeypatch.setattr(
            mod, "gh", lambda *_args, response=response, **_kwargs: completed(response)
        )
        assert mod.main(["current"]) == 0
        assert expected in gh_output.read_text()


def test_prune_removes_only_stale_labels_and_requires_context(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setenv("PR_NUMBER", "42")
    removed: list[str] = []
    monkeypatch.setattr(
        mod, "remove_label", lambda _repo, _pr, label: removed.append(label) or True
    )

    cases = [
        (["size/M"], "", []),
        (["size/M", "size/S"], "size/S", ["size/S"]),
        (["size/L", "size/M", "size/XL"], "", ["size/M", "size/XL"]),
    ]
    for labels, old_label, expected in cases:
        removed.clear()
        monkeypatch.setattr(mod, "list_size_labels", lambda *_args, labels=labels: labels)
        if old_label:
            monkeypatch.setenv("OLD_LABEL", old_label)
        else:
            monkeypatch.delenv("OLD_LABEL", raising=False)
        assert mod.main(["prune"]) == 0
        assert removed == expected

    monkeypatch.delenv("GH_REPO")
    monkeypatch.delenv("GITHUB_REPOSITORY", raising=False)
    with pytest.raises(SystemExit):
        mod.main(["current", "--pr-number", "42"])
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.delenv("PR_NUMBER")
    with pytest.raises(SystemExit):
        mod.main(["current"])
