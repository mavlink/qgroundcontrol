"""Shared pytest fixtures for tools tests."""

from __future__ import annotations

import os
from typing import TYPE_CHECKING

import pytest
from hypothesis import settings

from ._helpers import GIT_ENV, GIT_REPO_VARS, git

if TYPE_CHECKING:
    from collections.abc import Iterator
    from pathlib import Path

# On loaded xdist workers Hypothesis's per-example deadline would cause flaky failures.
settings.register_profile("qgc", deadline=None)
settings.load_profile("qgc")


def pytest_terminal_summary(
    terminalreporter: pytest.TerminalReporter, config: pytest.Config
) -> None:
    """Print pytest-randomly's seed even under -q, which hides the header that reports it."""
    seed = getattr(config.option, "randomly_seed", None)
    if isinstance(seed, int):
        terminalreporter.write_line(f"Reproduce the test order with --randomly-seed={seed}")


@pytest.fixture(autouse=True)
def _restore_environ() -> Iterator[None]:
    """Undo environment changes that monkeypatch does not track.

    Code under test exports variables for its child processes, and monkeypatch.delenv() of an unset
    variable records nothing to undo. Under xdist and random order a leak would reach whichever test
    runs next on that worker.
    """
    saved = os.environ.copy()
    yield
    if os.environ != saved:
        for name in os.environ.keys() - saved.keys():
            del os.environ[name]
        os.environ.update(saved)


@pytest.fixture
def gh_output(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    """Point GITHUB_OUTPUT at a fresh temp file and return its path."""
    path = tmp_path / "gh_output"
    path.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(path))
    return path


@pytest.fixture
def isolated_git(monkeypatch: pytest.MonkeyPatch) -> None:
    """Isolate every git command in the test, including the code under test's, from user config."""
    for name in GIT_REPO_VARS:
        monkeypatch.delenv(name, raising=False)
    for name, value in GIT_ENV.items():
        monkeypatch.setenv(name, value)


@pytest.fixture
def git_repo(tmp_path: Path, isolated_git: None) -> Path:
    """tmp_path initialized as an empty git repository."""
    git(tmp_path, "init", "-q")
    return tmp_path
