"""Contracts for external-tool version probing."""

from __future__ import annotations

import re
from typing import TYPE_CHECKING

import common.tool_version as tool_version
from common.tool_version import probe_version, version_prefix_matches

from ._helpers import completed

if TYPE_CHECKING:
    import pytest


def test_probe_version_parses_stdout_stderr_and_custom_patterns(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.setattr(tool_version.shutil, "which", lambda name: f"/usr/bin/{name}")
    cases = [
        (completed("clang-format version 21.1.7\n"), None, (21, 1, 7)),
        (completed("foo v4.13\n"), None, (4, 13)),
        (completed(stderr="javac 17.0.10\n"), None, (17, 0, 10)),
        (completed("release v2-7\n"), re.compile(r"v(\d+)-(\d+)"), (2, 7)),
    ]
    for result, pattern, expected in cases:
        monkeypatch.setattr(
            tool_version, "run_captured", lambda _args, result=result, **_kwargs: result
        )
        kwargs = {} if pattern is None else {"pattern": pattern}
        assert probe_version("tool", **kwargs) == expected


def test_probe_version_returns_none_when_unavailable_or_invalid(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.setattr(tool_version.shutil, "which", lambda _name: None)
    assert probe_version("missing") is None

    monkeypatch.setattr(tool_version.shutil, "which", lambda _name: "/bin/tool")
    for result in (completed(returncode=1), completed("not a version")):
        monkeypatch.setattr(
            tool_version, "run_captured", lambda _args, result=result, **_kwargs: result
        )
        assert probe_version("tool") is None


def test_custom_arguments_and_prefix_matching(monkeypatch: pytest.MonkeyPatch) -> None:
    calls: list[list[str]] = []
    monkeypatch.setattr(tool_version.shutil, "which", lambda _name: "/bin/git")
    monkeypatch.setattr(
        tool_version,
        "run_captured",
        lambda args, **_kwargs: calls.append(args) or completed("git version 2.42.0\n"),
    )
    assert probe_version("git", args=("version",)) == (2, 42, 0)
    assert calls == [["git", "version"]]

    for actual, expected, matches in (
        ((4, 13), "4.13.6", True),
        ((4, 13, 6), "4.13", True),
        ((4, 12, 6), "4.13.6", False),
        ((4, 13, 6), "latest", False),
    ):
        assert version_prefix_matches(actual, expected) is matches
