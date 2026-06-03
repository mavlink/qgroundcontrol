#!/usr/bin/env python3
"""Tests for tools/common/env.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

from common.env import is_ci, is_debug, is_github_actions, is_pr_event, runner_arch

if TYPE_CHECKING:
    import pytest


def test_is_ci_when_ci_set(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("CI", "true")
    monkeypatch.delenv("GITHUB_ACTIONS", raising=False)
    assert is_ci() is True


def test_is_ci_when_github_actions(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.delenv("CI", raising=False)
    monkeypatch.setenv("GITHUB_ACTIONS", "true")
    assert is_ci() is True


def test_is_ci_false_when_neither(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.delenv("CI", raising=False)
    monkeypatch.delenv("GITHUB_ACTIONS", raising=False)
    assert is_ci() is False


def test_is_ci_rejects_unset_value(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("CI", "")
    monkeypatch.delenv("GITHUB_ACTIONS", raising=False)
    assert is_ci() is False


def test_is_pr_event(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("GITHUB_EVENT_NAME", "pull_request")
    assert is_pr_event() is True


def test_is_pr_event_false_for_push(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("GITHUB_EVENT_NAME", "push")
    assert is_pr_event() is False


def test_runner_arch_uses_env_first(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("RUNNER_ARCH", "ARM64")
    assert runner_arch() == "ARM64"


def test_runner_arch_falls_back_to_platform(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.delenv("RUNNER_ARCH", raising=False)
    monkeypatch.setattr("platform.machine", lambda: "x86_64")
    assert runner_arch() == "x86_64"


def test_is_debug_truthy_values(monkeypatch: pytest.MonkeyPatch) -> None:
    for value in ("1", "true", "TRUE", "yes", "on"):
        monkeypatch.setenv("DEBUG", value)
        assert is_debug() is True, f"failed for {value!r}"


def test_is_debug_falsy_values(monkeypatch: pytest.MonkeyPatch) -> None:
    for value in ("0", "false", "no", "off", ""):
        monkeypatch.setenv("DEBUG", value)
        assert is_debug() is False, f"failed for {value!r}"


def test_is_github_actions(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("GITHUB_ACTIONS", "true")
    assert is_github_actions() is True
    monkeypatch.setenv("GITHUB_ACTIONS", "false")
    assert is_github_actions() is False
