#!/usr/bin/env python3
"""Drift guard: every workflow that sparse-checks out `ci_bootstrap.py` must
also include the rest of the bootstrap shim plus every `common.*` module that
the script it runs imports transitively.

GitHub Actions can't share the bootstrap file list via a composite action
(local actions require a prior checkout), so each lightweight script job
hand-rolls the same sparse-checkout block. This test fails when any of them
drifts out of sync — e.g. when `tools/common/__init__.py` got dropped from
docker.yml and only kept working because `common` is a namespace package, or
when a script grew a new `from common.git import ...` without the workflow
adding `tools/common/git.py` to its sparse-checkout list.
"""

from __future__ import annotations

import ast
from pathlib import Path
from typing import Iterator

import pytest
import yaml

REPO_ROOT = Path(__file__).resolve().parents[3]
WORKFLOWS_DIR = REPO_ROOT / ".github" / "workflows"
TOOLS_DIR = REPO_ROOT / "tools"
SCRIPTS_DIR = REPO_ROOT / ".github" / "scripts"

BOOTSTRAP_TRIPWIRE = ".github/scripts/ci_bootstrap.py"

# Always required when the tripwire is present, regardless of which script runs.
BASE_BOOTSTRAP_PATHS: frozenset[str] = frozenset(
    {
        ".github/scripts/ci_bootstrap.py",
        "tools/_bootstrap.py",
        "tools/common/__init__.py",
    }
)


def _common_module_to_path(module: str) -> str | None:
    """Map `common.foo[.bar]` to `tools/common/foo[/bar].py` if it exists on disk."""
    rel = "tools/" + module.replace(".", "/") + ".py"
    return rel if (REPO_ROOT / rel).is_file() else None


def _iter_common_imports(source: Path) -> Iterator[str]:
    """Yield `common.<dotted>` module names imported (directly or relatively) by `source`."""
    try:
        tree = ast.parse(source.read_text(encoding="utf-8"))
    except (OSError, SyntaxError):
        return
    inside_common = TOOLS_DIR / "common" in source.parents
    for node in ast.walk(tree):
        if isinstance(node, ast.ImportFrom):
            if node.module is None:
                continue
            if node.level == 0 and node.module.split(".")[0] == "common":
                yield node.module
            elif node.level >= 1 and inside_common:
                yield f"common.{node.module}"
        elif isinstance(node, ast.Import):
            for alias in node.names:
                if alias.name.split(".")[0] == "common":
                    yield alias.name


def _required_common_paths(script: Path) -> frozenset[str]:
    """Closure of every `common.*` module file required to import `script`."""
    required: set[str] = set()
    queue: list[Path] = [script]
    while queue:
        cur = queue.pop()
        for module in _iter_common_imports(cur):
            rel = _common_module_to_path(module)
            if rel and rel not in required:
                required.add(rel)
                queue.append(REPO_ROOT / rel)
    return frozenset(required)


def _scripts_in_block(entries: frozenset[str]) -> list[Path]:
    """Return the .github/scripts/*.py files (excluding the tripwire) in a sparse-checkout block."""
    return [
        REPO_ROOT / e
        for e in entries
        if e.startswith(".github/scripts/")
        and e.endswith(".py")
        and e != BOOTSTRAP_TRIPWIRE
        and (REPO_ROOT / e).is_file()
    ]


def _iter_sparse_checkout_blocks(workflow_path: Path) -> Iterator[tuple[str, frozenset[str]]]:
    """Yield (step-context, paths) for each sparse-checkout step in a workflow."""
    doc = yaml.safe_load(workflow_path.read_text(encoding="utf-8"))
    if not isinstance(doc, dict):
        return
    for job_name, job in (doc.get("jobs") or {}).items():
        if not isinstance(job, dict):
            continue
        for index, step in enumerate(job.get("steps") or []):
            if not isinstance(step, dict):
                continue
            if not str(step.get("uses", "")).startswith("actions/checkout"):
                continue
            paths = (step.get("with") or {}).get("sparse-checkout")
            if not isinstance(paths, str):
                continue
            entries = frozenset(p.strip() for p in paths.splitlines() if p.strip())
            yield f"{workflow_path.name}::{job_name}[step {index}]", entries


def test_bootstrap_sparse_checkout_matches_canonical() -> None:
    if not WORKFLOWS_DIR.is_dir():
        pytest.skip("workflows dir not in checkout")

    drift: list[str] = []
    for workflow in sorted(WORKFLOWS_DIR.glob("*.yml")):
        for context, entries in _iter_sparse_checkout_blocks(workflow):
            if BOOTSTRAP_TRIPWIRE not in entries:
                continue
            required: set[str] = set(BASE_BOOTSTRAP_PATHS)
            for script in _scripts_in_block(entries):
                required |= _required_common_paths(script)
            missing = sorted(required - entries)
            if missing:
                drift.append(f"{context} missing: {missing}")

    if drift:
        pytest.fail(
            "Workflows referencing the bootstrap shim are missing files required by their scripts:\n  "
            + "\n  ".join(drift)
        )
