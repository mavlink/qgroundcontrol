"""Contracts for CI environment detection."""

from __future__ import annotations

from typing import TYPE_CHECKING

from common.env import is_ci

if TYPE_CHECKING:
    import pytest


def test_ci_detection_accepts_ci_or_github_actions(monkeypatch: pytest.MonkeyPatch) -> None:
    for ci, github, expected in (
        ("true", None, True),
        (None, "true", True),
        (None, None, False),
        ("", None, False),
    ):
        monkeypatch.delenv("CI", raising=False)
        monkeypatch.delenv("GITHUB_ACTIONS", raising=False)
        if ci is not None:
            monkeypatch.setenv("CI", ci)
        if github is not None:
            monkeypatch.setenv("GITHUB_ACTIONS", github)
        assert is_ci() is expected
