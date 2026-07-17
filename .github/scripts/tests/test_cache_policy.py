"""Cache-write policy contracts."""

from __future__ import annotations

from typing import TYPE_CHECKING

import cache_policy as mod
import pytest

if TYPE_CHECKING:
    from pathlib import Path


def test_explicit_and_auto_policies_write_expected_decision(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    gh_output: Path,
) -> None:
    cases = [
        (["--requested", "true"], {}, "true"),
        (["--requested", "false"], {}, "false"),
        (["--requested", "auto"], {"EVENT_NAME": "push", "THIS_REPO": "owner/repo"}, "true"),
        (
            ["--requested", "auto"],
            {"EVENT_NAME": "pull_request", "THIS_REPO": "owner/repo", "PR_REPO": "owner/repo"},
            "false",
        ),
        (
            ["--requested", "auto"],
            {"EVENT_NAME": "pull_request", "THIS_REPO": "owner/repo", "PR_REPO": "fork/repo"},
            "false",
        ),
    ]
    for args, environment, expected in cases:
        for key in ("EVENT_NAME", "THIS_REPO", "PR_REPO"):
            monkeypatch.delenv(key, raising=False)
        for key, value in environment.items():
            monkeypatch.setenv(key, value)
        gh_output.write_text("")
        assert mod.main(args) == 0
        assert capsys.readouterr().out.strip() == expected
        assert f"save={expected}" in gh_output.read_text()


def test_requested_policy_is_required() -> None:
    with pytest.raises(SystemExit):
        mod.main([])
