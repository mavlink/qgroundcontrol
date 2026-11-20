"""Every script a sparse-checkout job runs is inside what the job checks out, with its imports.

Lightweight jobs check out a few directories instead of the repository. A script, a module it
imports, a local action, or a file that action reads outside those directories fails only when the
job runs, which for workflow_run, pull_request_target and scheduled workflows is after the merge.
"""

from __future__ import annotations

import ast
import fnmatch
import re
from typing import TYPE_CHECKING, Any

from .._helpers import REPO_ROOT, TOOLS_DIR, get_steps, load_action
from ._workflows import local_action, step_groups, uses_action

if TYPE_CHECKING:
    from collections.abc import Iterator
    from pathlib import Path

# Repository files a step names: the scripts it runs and the files setup-python and uv read,
# relative to the workspace or behind `./`, `$GITHUB_WORKSPACE/` or `${GITHUB_WORKSPACE}/`.
NAMED_FILE = re.compile(
    r"(?:(?<=GITHUB_WORKSPACE\}/)|(?<=GITHUB_WORKSPACE/)|(?<=\./)|(?<![\w./-]))"
    r"((?:tools|deploy|\.github|\.clusterfuzzlite)/[\w./-]+\.(?:py|toml|lock))"
)
# uv builds the environment on whichever Python the runner has unless this file names one.
PYTHON_VERSION_PIN = {"tools/uv.lock": "tools/.python-version"}


def _strings(value: Any) -> Iterator[str]:
    if isinstance(value, str):
        yield value
    elif isinstance(value, dict):
        for child in value.values():
            yield from _strings(child)
    elif isinstance(value, list):
        for child in value:
            yield from _strings(child)


def _step_files(step: dict[str, Any]) -> set[str]:
    """Files the step's script or inputs name; a checkout's inputs list what it fetches instead."""
    if uses_action(step, "actions/checkout"):
        return set()
    texts = _strings([step.get("run"), step.get("with")])
    return {name for text in texts for name in NAMED_FILE.findall(text)}


def _named_files(steps: list[dict[str, Any]]) -> set[str]:
    """Files the steps name, with each local action they use and what its steps name in turn."""
    named: set[str] = set()
    for step in steps:
        named |= _step_files(step)
        if name := local_action(step):
            named.add(f".github/actions/{name}/action.yml")
            named |= _named_files(get_steps(load_action(name)))
    return named


def _type_checking_only(tree: ast.AST) -> set[ast.AST]:
    """Nodes inside `if TYPE_CHECKING:` blocks, whose imports never run."""
    skipped: set[ast.AST] = set()
    for node in ast.walk(tree):
        if isinstance(node, ast.If):
            test = node.test
            if (isinstance(test, ast.Name) and test.id == "TYPE_CHECKING") or (
                isinstance(test, ast.Attribute) and test.attr == "TYPE_CHECKING"
            ):
                for statement in node.body:
                    skipped.update(ast.walk(statement))
    return skipped


def _module_files(root: Path, module: str) -> list[Path]:
    """Existing package `__init__.py` files and module file of dotted *module* under *root*."""
    parts = module.split(".")
    candidates = [
        root.joinpath(*parts[:depth], "__init__.py") for depth in range(1, len(parts) + 1)
    ]
    candidates.append(root.joinpath(*parts).with_suffix(".py"))
    return [path for path in candidates if path.is_file()]


def _imported_files(source: Path, roots: list[Path]) -> set[Path]:
    """Repository files *source* imports at run time, resolving absolute imports under *roots*.

    Standard-library and installed modules resolve to no repository file.
    """
    tree = ast.parse(source.read_text(encoding="utf-8"))
    skipped = _type_checking_only(tree)
    files: set[Path] = set()
    for node in ast.walk(tree):
        if node in skipped:
            continue
        if isinstance(node, ast.Import):
            modules = [(0, alias.name) for alias in node.names]
        elif isinstance(node, ast.ImportFrom):
            # `from ci.planning import detect_changes` may import a submodule.
            base = node.module or ""
            modules = [(node.level, base)]
            modules += [(node.level, f"{base}.{alias.name}".strip(".")) for alias in node.names]
        else:
            continue
        for level, module in modules:
            for root in [source.parents[level - 1]] if level else roots:
                files.update(_module_files(root, module) if module else [])
    return files


def _import_closure(script: Path) -> set[Path]:
    """Every repository file importing *script* loads, directly or through another module."""
    # Scripts put tools/ on sys.path, and Python adds the script's own directory.
    roots = [TOOLS_DIR, script.parent]
    closure: set[Path] = set()
    queue = [script]
    while queue:
        for imported in _imported_files(queue.pop(), roots) - closure:
            closure.add(imported)
            queue.append(imported)
    return closure


def _checked_out(path: str, entries: list[str]) -> bool:
    """Whether non-cone sparse-checkout *entries* (files, directories, globs) include *path*."""
    for entry in entries:
        name = entry.strip("/")
        if path == name or path.startswith(f"{name}/") or fnmatch.fnmatchcase(path, name):
            return True
    return False


def _sparse_jobs() -> Iterator[tuple[str, list[str], list[dict[str, Any]]]]:
    """Yield (label, sparse-checkout entries, steps) for each job whose only checkouts are sparse."""
    for label, _, steps in step_groups(actions=False):
        checkouts = [
            step.get("with") or {} for step in steps if uses_action(step, "actions/checkout")
        ]
        if checkouts and all("sparse-checkout" in options for options in checkouts):
            # Entries name files and directories exactly: cone mode takes directories only, and
            # adds every file beside them and at the repository root.
            assert all(
                options.get("sparse-checkout-cone-mode") is False for options in checkouts
            ), label
            entries = [
                entry for options in checkouts for entry in options["sparse-checkout"].split()
            ]
            yield label, entries, steps


def test_sparse_checkout_jobs_check_out_what_they_run() -> None:
    missing: dict[str, list[str]] = {}
    jobs = 0
    for label, entries, steps in _sparse_jobs():
        jobs += 1
        required = _named_files(steps)
        required |= {pin for lock, pin in PYTHON_VERSION_PIN.items() if lock in required}
        for script in [name for name in required if name.endswith(".py")]:
            required |= {
                path.relative_to(REPO_ROOT).as_posix()
                for path in _import_closure(REPO_ROOT / script)
            }
        if absent := sorted(path for path in required if not _checked_out(path, entries)):
            missing[label] = absent
    assert jobs
    assert not missing


def test_every_script_a_workflow_or_action_names_exists() -> None:
    named = {
        (label, name)
        for label, _, steps in step_groups()
        for step in steps
        for name in _step_files(step)
    }
    assert named
    assert not sorted(
        f"{label}: {name}" for label, name in named if not (REPO_ROOT / name).is_file()
    )
