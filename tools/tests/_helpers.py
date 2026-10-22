"""Shared helpers for tools tests."""

from __future__ import annotations

import copy
import functools
import os
import subprocess
from pathlib import Path
from typing import Any

import yaml

REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_DIR = REPO_ROOT / "tools"

# yaml.safe_load always uses the pure-Python parser, roughly 10x slower than libyaml's.
_YAML_LOADER = getattr(yaml, "CSafeLoader", yaml.SafeLoader)

# Pins git to a known identity and defaults, ignoring user and system config (signing, hooks, ...).
GIT_ENV = {
    "GIT_CONFIG_GLOBAL": os.devnull,
    "GIT_CONFIG_NOSYSTEM": "1",
    "GIT_AUTHOR_NAME": "QGC Test",
    "GIT_AUTHOR_EMAIL": "test@example.invalid",
    "GIT_COMMITTER_NAME": "QGC Test",
    "GIT_COMMITTER_EMAIL": "test@example.invalid",
    "GIT_CONFIG_COUNT": "4",
    "GIT_CONFIG_KEY_0": "commit.gpgsign",
    "GIT_CONFIG_VALUE_0": "false",
    "GIT_CONFIG_KEY_1": "tag.gpgsign",
    "GIT_CONFIG_VALUE_1": "false",
    "GIT_CONFIG_KEY_2": "core.hooksPath",
    "GIT_CONFIG_VALUE_2": os.devnull,
    "GIT_CONFIG_KEY_3": "init.defaultBranch",
    "GIT_CONFIG_VALUE_3": "master",
}
# Exported by git hooks (e.g. pre-commit); they would redirect commands away from a temp repository.
GIT_REPO_VARS = (
    "GIT_ALTERNATE_OBJECT_DIRECTORIES",
    "GIT_COMMON_DIR",
    "GIT_CONFIG_PARAMETERS",
    "GIT_DIR",
    "GIT_INDEX_FILE",
    "GIT_OBJECT_DIRECTORY",
    "GIT_PREFIX",
    "GIT_WORK_TREE",
)


def completed(
    stdout: str = "", returncode: int = 0, stderr: str = ""
) -> subprocess.CompletedProcess:
    """Fake subprocess.CompletedProcess for stubbing external commands."""
    return subprocess.CompletedProcess(args=[], returncode=returncode, stdout=stdout, stderr=stderr)


def git(repo: Path, *args: str) -> str:
    """Run git in *repo*, isolated from user config and any enclosing repository; return stdout."""
    env = {key: value for key, value in os.environ.items() if key not in GIT_REPO_VARS}
    env.update(GIT_ENV)
    return subprocess.run(
        ["git", *args], cwd=repo, env=env, check=True, capture_output=True, text=True
    ).stdout.strip()


@functools.cache
def _parse_yaml(path: Path) -> Any:
    document = yaml.load(path.read_text(encoding="utf-8"), Loader=_YAML_LOADER)
    # YAML 1.1 reads a workflow's bare `on:` key as boolean True.
    if isinstance(document, dict) and True in document:
        document["on"] = document.pop(True)
    return document


def load_yaml(path: str | Path) -> Any:
    """Parse a YAML file (relative paths resolve against REPO_ROOT); callers get a private copy."""
    return copy.deepcopy(_parse_yaml(REPO_ROOT / path))


def load_workflow(name: str) -> dict[str, Any]:
    """Parsed .github/workflows/<name>, with the trigger mapping under "on"."""
    return load_yaml(f".github/workflows/{name}")


def load_action(name: str) -> dict[str, Any]:
    """Parsed .github/actions/<name>/action.yml."""
    return load_yaml(f".github/actions/{name}/action.yml")


def get_steps(document: dict[str, Any], job: str | None = None) -> list[dict[str, Any]]:
    """Steps of *job* in a workflow, or of a composite action when *job* is None."""
    return document["jobs"][job]["steps"] if job else document["runs"]["steps"]


def get_step(steps: list[dict[str, Any]], name: str) -> dict[str, Any]:
    """The single step whose name or id is *name*."""
    (match,) = [step for step in steps if name in (step.get("name"), step.get("id"))]
    return match
