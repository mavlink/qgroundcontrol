"""Contracts for the semantic-release wrapper."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest

if TYPE_CHECKING:
    from pathlib import Path
import release

from ._helpers import completed


def test_arguments_default_to_safe_dry_run() -> None:
    defaults = release.parse_args([])
    assert (defaults.run, defaults.install) == (False, False)
    explicit = release.parse_args(["--run", "--install"])
    assert (explicit.run, explicit.install) == (True, True)


def test_node_version_gate(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(release, "probe_version", lambda _tool: (22, 4, 1))
    assert release.check_node() == 22

    for version in (None, (17, 9, 0)):
        monkeypatch.setattr(release, "probe_version", lambda _tool, value=version: value)
        with pytest.raises(SystemExit):
            release.check_node()


def test_semantic_release_command_defaults_to_dry_run(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    commands: list[list[str]] = []
    monkeypatch.setattr(
        release.subprocess,
        "run",
        lambda command, **_kwargs: commands.append(command) or completed(returncode=7),
    )
    assert release.run_semantic_release(dry_run=True) == 7
    assert commands == [["npx", "--yes", f"semantic-release@{release.SR_VERSION}", "--dry-run"]]


def test_main_requires_config_and_token_before_running(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(release, "find_repo_root", lambda _path: tmp_path)
    monkeypatch.setattr(release, "check_node", lambda: 22)
    calls: list[bool] = []
    monkeypatch.setattr(
        release,
        "run_semantic_release",
        lambda *, dry_run: calls.append(dry_run) or 0,
    )

    assert release.main([]) == 1
    (tmp_path / ".releaserc.json").write_text("{}", encoding="utf-8")
    assert release.main([]) == 0
    assert calls == [True]

    monkeypatch.delenv("GITHUB_TOKEN", raising=False)
    assert release.main(["--run"]) == 1
    monkeypatch.setenv("GITHUB_TOKEN", "test-token")
    assert release.main(["--run"]) == 0
    assert calls == [True, False]
