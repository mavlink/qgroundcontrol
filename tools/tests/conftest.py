"""Shared pytest fixtures for tools tests."""

from __future__ import annotations

import json
import os
from typing import TYPE_CHECKING, Any

import pytest
from hypothesis import settings
from hypothesis.configuration import set_hypothesis_home_dir

from ._helpers import GIT_ENV, GIT_REPO_VARS, REPO_ROOT, git

if TYPE_CHECKING:
    from collections.abc import Callable, Iterator
    from pathlib import Path

# On loaded xdist workers Hypothesis's per-example deadline would cause flaky failures.
settings.register_profile("qgc", deadline=None)
settings.load_profile("qgc")
set_hypothesis_home_dir(REPO_ROOT / ".cache" / "hypothesis")

# The runner's context and files, plus what CI steps export (setup-python's cache backend).
_CI_VARIABLE_PREFIXES = ("GITHUB_", "RUNNER_", "ACTIONS_", "RUNS_ON_")
_CI_VARIABLES = frozenset({"CI", "QGC_CI_DEBUG", "QGC_ACTIONS_CACHE_BACKEND"})


def pytest_terminal_summary(
    terminalreporter: pytest.TerminalReporter, config: pytest.Config
) -> None:
    """Print pytest-randomly's seed even under -q, which hides the header that reports it."""
    seed = getattr(config.option, "randomly_seed", None)
    if isinstance(seed, int):
        terminalreporter.write_line(f"Reproduce the test order with --randomly-seed={seed}")


@pytest.fixture(autouse=True, scope="session")
def _hide_ci_environment() -> Iterator[None]:
    """Run every test, and module-scoped fixtures, as on a developer machine.

    Code under test appends to the runner's GITHUB_OUTPUT and GITHUB_STEP_SUMMARY and branches on
    CI, GITHUB_REF and RUNNER_*; a test that needs one sets it.
    """
    with pytest.MonkeyPatch.context() as patch:
        for name in list(os.environ):
            if name in _CI_VARIABLES or name.startswith(_CI_VARIABLE_PREFIXES):
                patch.delenv(name)
        yield


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


def _runner_file(variable: str, path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    path.write_text("")
    monkeypatch.setenv(variable, str(path))
    return path


@pytest.fixture
def gh_output(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    """Point GITHUB_OUTPUT at a fresh empty file and return its path."""
    return _runner_file("GITHUB_OUTPUT", tmp_path / "gh_output", monkeypatch)


@pytest.fixture
def gh_env(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    """Point GITHUB_ENV at a fresh empty file and return its path."""
    return _runner_file("GITHUB_ENV", tmp_path / "gh_env", monkeypatch)


@pytest.fixture
def gh_path(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    """Point GITHUB_PATH at a fresh empty file and return its path."""
    return _runner_file("GITHUB_PATH", tmp_path / "gh_path", monkeypatch)


@pytest.fixture
def gh_summary(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    """Point GITHUB_STEP_SUMMARY at a fresh empty file and return its path."""
    return _runner_file("GITHUB_STEP_SUMMARY", tmp_path / "gh_summary", monkeypatch)


@pytest.fixture
def workflow_run_event(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Callable[..., Path]:
    """Return a writer that points GITHUB_EVENT_PATH at a workflow_run payload with given fields."""

    def write(**run: Any) -> Path:
        path = tmp_path / "event.json"
        path.write_text(json.dumps({"workflow_run": run}), encoding="utf-8")
        monkeypatch.setenv("GITHUB_EVENT_PATH", str(path))
        return path

    return write


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
