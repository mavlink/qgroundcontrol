"""Shared helpers for tools tests."""

from __future__ import annotations

import copy
import functools
import importlib.util
import os
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING, Any

import yaml

if TYPE_CHECKING:
    from collections.abc import Mapping
    from types import ModuleType

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


def write_files(root: Path, files: dict[str, str]) -> None:
    """Write each relative path in *files* under *root*, creating directories, with LF endings."""
    for name, content in files.items():
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="\n")


def commit_files(repo: Path, files: dict[str, str], message: str) -> str:
    """Write *files* into *repo*, commit every change, and return the new commit's SHA."""
    write_files(repo, files)
    git(repo, "add", "-A")
    git(repo, "commit", "-qm", message)
    return git(repo, "rev-parse", "HEAD")


def read_github_file(path: Path) -> dict[str, str]:
    """Parse a GITHUB_OUTPUT or GITHUB_ENV file: `name=value` lines and `name<<DELIMITER` blocks."""
    values: dict[str, str] = {}
    lines = iter(path.read_text(encoding="utf-8").splitlines())
    for line in lines:
        equals, heredoc = line.find("="), line.find("<<")
        if heredoc > 0 and (equals < 0 or heredoc < equals):
            delimiter = line[heredoc + 2 :]
            block = []
            for body in lines:
                if body == delimiter:
                    break
                block.append(body)
            else:
                raise ValueError(f"{path}: {line[:heredoc]} has no closing {delimiter!r}")
            values[line[:heredoc]] = "\n".join(block)
        elif equals > 0:
            values[line[:equals]] = line[equals + 1 :]
        elif line:
            raise ValueError(f"{path}: malformed line {line!r}")
    return values


def load_script(path: str | Path) -> ModuleType:
    """Execute a script outside the tools/ packages (relative to REPO_ROOT) as a fresh module."""
    path = REPO_ROOT / path
    spec = importlib.util.spec_from_file_location(path.stem, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


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


def workflow_names() -> list[str]:
    """File names of every workflow in .github/workflows, sorted."""
    return sorted(path.name for path in (REPO_ROOT / ".github/workflows").glob("*.yml"))


def action_names() -> list[str]:
    """Names of every local action (.github/actions/<name>/action.yml), sorted."""
    return sorted(path.parent.name for path in (REPO_ROOT / ".github/actions").glob("*/action.yml"))


def get_steps(document: dict[str, Any], job: str | None = None) -> list[dict[str, Any]]:
    """Steps of *job* in a workflow, or of a local action when *job* is None (none for JavaScript)."""
    return document["jobs"][job]["steps"] if job else document["runs"].get("steps", [])


def get_step(steps: list[dict[str, Any]], name: str) -> dict[str, Any]:
    """The single step whose name or id is *name*."""
    (match,) = [step for step in steps if name in (step.get("name"), step.get("id"))]
    return match


_EXPRESSION_TOKEN = re.compile(
    r"\s*(?:(?P<op>==|!=|&&|\|\||!|\(|\))|'(?P<str>(?:[^']|'')*)'|(?P<num>\d+)"
    r"|(?P<name>[A-Za-z_][\w-]*(?:\.[A-Za-z_][\w-]*)*))"
)


def evaluate_expression(expression: str, context: Mapping[str, Any]) -> Any:
    """Evaluate a GitHub Actions expression of literals, context paths, ==, !=, !, && and ||.

    Paths read nested *context* mappings (a missing one is null); comparisons ignore case, and
    && and || return an operand, as in GitHub's evaluator. Function calls are not supported.
    """
    text = expression.strip()
    if text.startswith("${{") and text.endswith("}}"):
        text = text[3:-2]
    tokens: list[tuple[str, Any]] = []
    position = 0
    while text[position:].strip():
        match = _EXPRESSION_TOKEN.match(text, position)
        if not match:
            raise ValueError(f"cannot evaluate {expression!r} at {text[position:]!r}")
        position = match.end()
        if match["op"]:
            tokens.append(("op", match["op"]))
        elif match["str"] is not None:
            tokens.append(("value", match["str"].replace("''", "'")))
        elif match["num"]:
            tokens.append(("value", int(match["num"])))
        elif match["name"] in {"true", "false", "null"}:
            tokens.append(("value", {"true": True, "false": False, "null": None}[match["name"]]))
        else:
            value: Any = context
            for part in match["name"].split("."):
                value = value.get(part) if isinstance(value, dict) else None
            tokens.append(("value", value))

    def peek() -> tuple[str, Any] | None:
        return tokens[0] if tokens else None

    def take(op: str) -> bool:
        if peek() == ("op", op):
            tokens.pop(0)
            return True
        return False

    def primary() -> Any:
        if take("("):
            value = either()
            if not take(")"):
                raise ValueError(f"unbalanced parentheses in {expression!r}")
            return value
        if take("!"):
            return not primary()
        kind, value = tokens.pop(0)
        if kind != "value":
            raise ValueError(f"unexpected {value!r} in {expression!r}")
        return value

    def comparison() -> Any:
        left = primary()
        for op in ("==", "!="):
            if take(op):
                right = primary()
                same = (
                    left.casefold() == right.casefold()
                    if isinstance(left, str) and isinstance(right, str)
                    else left == right
                )
                return same if op == "==" else not same
        return left

    def both() -> Any:
        value = comparison()
        while take("&&"):
            right = comparison()
            value = right if value else value
        return value

    def either() -> Any:
        value = both()
        while take("||"):
            right = both()
            value = value if value else right
        return value

    result = either()
    if tokens:
        raise ValueError(f"unexpected {tokens[0][1]!r} in {expression!r}")
    return result


@dataclass(frozen=True)
class PrMerge:
    """A repository checked out at GitHub's pull_request merge commit, plus its depth-2 CI clone."""

    full: Path
    shallow: Path
    fork_point: str
    base_tip: str
    pr_head: str


def pr_merge_checkout(
    root: Path, *, base: dict[str, str], newer_base: dict[str, str], pr: dict[str, str]
) -> PrMerge:
    """Fork a PR from *base*, move the base branch on with *newer_base*, then merge the PR into it.

    The merge mirrors a pull_request checkout, whose first parent is newer than the fork point
    that the event's base SHA may still name; the depth-2 clone has only the merge and its parents.
    """
    full = root / "full"
    full.mkdir(parents=True)
    git(full, "init", "-q")
    fork_point = commit_files(full, base, "Fork point")
    git(full, "checkout", "-qb", "pr")
    pr_head = commit_files(full, pr, "PR change")
    git(full, "checkout", "-q", "master")
    base_tip = commit_files(full, newer_base, "Newer base")
    git(full, "merge", "-q", "--no-ff", "-m", "Merge PR", pr_head)
    shallow = root / "shallow"
    git(root, "clone", "-q", "--depth=2", full.as_uri(), str(shallow))
    return PrMerge(full, shallow, fork_point, base_tip, pr_head)
