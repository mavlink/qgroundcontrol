#!/usr/bin/env python3
"""Drift guard: every workflow or composite action that sparse-checks out the
bootstrap shim must also include every `common.*` module that the scripts it
checks out import transitively.

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
from typing import TYPE_CHECKING

import pytest
import yaml
from _helpers import REPO_ROOT

if TYPE_CHECKING:
    from collections.abc import Iterator
    from pathlib import Path

WORKFLOWS_DIR = REPO_ROOT / ".github" / "workflows"
ACTIONS_DIR = REPO_ROOT / ".github" / "actions"
TOOLS_DIR = REPO_ROOT / "tools"
SCRIPTS_DIR = REPO_ROOT / ".github" / "scripts"

CI_BOOTSTRAP = ".github/scripts/ci_bootstrap.py"
TOOLS_BOOTSTRAP = "tools/_bootstrap.py"
BOOTSTRAP_TRIPWIRES: frozenset[str] = frozenset({CI_BOOTSTRAP, TOOLS_BOOTSTRAP})

# Always required when a tripwire is present, regardless of which script runs.
BASE_BOOTSTRAP_PATHS: frozenset[str] = frozenset(
    {
        "tools/_bootstrap.py",
        "tools/common/__init__.py",
    }
)


def _common_module_to_path(module: str) -> str | None:
    """Map `common.foo[.bar]` to `tools/common/foo[/bar].py` if it exists on disk."""
    rel = "tools/" + module.replace(".", "/") + ".py"
    return rel if (REPO_ROOT / rel).is_file() else None


def _type_checking_body_nodes(tree: ast.AST) -> set[ast.AST]:
    """Nodes inside `if TYPE_CHECKING:` blocks — those imports never run at runtime."""
    skip: set[ast.AST] = set()
    for node in ast.walk(tree):
        if isinstance(node, ast.If):
            test = node.test
            is_tc = (isinstance(test, ast.Name) and test.id == "TYPE_CHECKING") or (
                isinstance(test, ast.Attribute) and test.attr == "TYPE_CHECKING"
            )
            if is_tc:
                for stmt in node.body:
                    skip.update(ast.walk(stmt))
    return skip


def _iter_common_imports(source: Path) -> Iterator[str]:
    """Yield `common.<dotted>` module names imported at runtime (directly or relatively) by `source`."""
    try:
        tree = ast.parse(source.read_text(encoding="utf-8"))
    except (OSError, SyntaxError):
        return
    inside_common = TOOLS_DIR / "common" in source.parents
    skip = _type_checking_body_nodes(tree)
    for node in ast.walk(tree):
        if node in skip:
            continue
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
    """Return every checked-out .py file (excluding the bootstrap shims) in a sparse-checkout block."""
    return [
        REPO_ROOT / e
        for e in entries
        if e.endswith(".py") and e not in BOOTSTRAP_TRIPWIRES and (REPO_ROOT / e).is_file()
    ]


def _iter_checkout_steps(doc: dict, source: str) -> Iterator[tuple[str, frozenset[str]]]:
    """Yield (step-context, paths) for each actions/checkout step with a sparse-checkout list."""
    if "jobs" in doc:
        groups = [
            (name, job.get("steps"))
            for name, job in (doc.get("jobs") or {}).items()
            if isinstance(job, dict)
        ]
    else:
        groups = [("runs", (doc.get("runs") or {}).get("steps"))]
    for group_name, steps in groups:
        for index, step in enumerate(steps or []):
            if not isinstance(step, dict):
                continue
            if not str(step.get("uses", "")).startswith("actions/checkout"):
                continue
            paths = (step.get("with") or {}).get("sparse-checkout")
            if not isinstance(paths, str):
                continue
            entries = frozenset(p.strip() for p in paths.splitlines() if p.strip())
            yield f"{source}::{group_name}[step {index}]", entries


def _iter_sparse_checkout_blocks() -> Iterator[tuple[str, frozenset[str]]]:
    """Yield sparse-checkout blocks from every workflow and composite action."""
    sources = sorted(WORKFLOWS_DIR.glob("*.yml")) + sorted(ACTIONS_DIR.glob("*/action.yml"))
    for path in sources:
        doc = yaml.safe_load(path.read_text(encoding="utf-8"))
        if not isinstance(doc, dict):
            continue
        yield from _iter_checkout_steps(doc, str(path.relative_to(REPO_ROOT)))


def test_bootstrap_sparse_checkout_matches_canonical() -> None:
    if not WORKFLOWS_DIR.is_dir():
        pytest.skip("workflows dir not in checkout")

    drift: list[str] = []
    for context, entries in _iter_sparse_checkout_blocks():
        stale = sorted(
            e
            for e in entries
            if e.endswith(".py")
            and not any(ch in e for ch in "*?[")
            and not (REPO_ROOT / e).is_file()
        )
        if stale:
            drift.append(f"{context} lists nonexistent files: {stale}")
        if not entries & BOOTSTRAP_TRIPWIRES:
            continue
        required: set[str] = set(BASE_BOOTSTRAP_PATHS)
        # `import common` runs __init__ at runtime; an eager facade that pulls
        # every submodule would need them all present even under a thin sparse set.
        required |= _required_common_paths(TOOLS_DIR / "common" / "__init__.py")
        scripts = _scripts_in_block(entries)
        if any(str(s.relative_to(REPO_ROOT)).startswith(".github/scripts/") for s in scripts):
            required.add(CI_BOOTSTRAP)
        for script in scripts:
            required |= _required_common_paths(script)
        missing = sorted(required - entries)
        if missing:
            drift.append(f"{context} missing: {missing}")

    if drift:
        pytest.fail(
            "Workflows/actions referencing the bootstrap shim are missing files required by their scripts:\n  "
            + "\n  ".join(drift)
        )


BOOTSTRAP_ACTION_YML = ACTIONS_DIR / "build-results-bootstrap" / "action.yml"

EXPECTED_BOOTSTRAP_PATHS: frozenset[str] = frozenset(
    {
        ".github/actions/download-all-artifacts",
        ".github/actions/collect-artifact-sizes",
        ".github/actions/replace-cache-entry",
        ".github/actions/setup-python",
        ".github/scripts/check_baseline_ready.py",
        ".github/scripts/ci_bootstrap.py",
        "tools/_bootstrap.py",
        ".github/scripts/collect_artifact_sizes.py",
        ".github/scripts/collect_build_status.py",
        ".github/scripts/download_artifacts.py",
        ".github/scripts/generate_build_results_comment.py",
        ".github/scripts/templates/build_results.md.j2",
        ".github/scripts/xml_utils.py",
        "tools/common/__init__.py",
        "tools/common/build_config.py",
        "tools/common/file_traversal.py",
        "tools/common/format.py",
        "tools/common/gh_actions.py",
        "tools/common/github_runs.py",
        "tools/common/io.py",
        "tools/common/markdown.py",
        "tools/common/platform.py",
        "tools/pyproject.toml",
        "tools/uv.lock",
        "tools/setup/install_python.py",
    }
)


def test_build_results_bootstrap_sparse_checkout_matches_expected() -> None:
    """Full-mirror guard for the build-results-bootstrap composite: the import
    closure above only covers .py files, so removing an action dir, template,
    or lockfile entry would still break post-pr-comment/save-baselines at
    runtime. Update EXPECTED_BOOTSTRAP_PATHS in lockstep when it changes."""
    if not BOOTSTRAP_ACTION_YML.exists():
        pytest.skip("build-results-bootstrap action.yml not in checkout")

    doc = yaml.safe_load(BOOTSTRAP_ACTION_YML.read_text(encoding="utf-8"))
    blocks = dict(_iter_checkout_steps(doc, str(BOOTSTRAP_ACTION_YML.relative_to(REPO_ROOT))))
    assert blocks, "build-results-bootstrap action.yml has no sparse-checkout step"
    (actual,) = blocks.values()

    missing = EXPECTED_BOOTSTRAP_PATHS - actual
    extra = actual - EXPECTED_BOOTSTRAP_PATHS

    msg_parts = []
    if missing:
        msg_parts.append(f"expected paths missing from composite: {sorted(missing)}")
    if extra:
        msg_parts.append(
            f"composite has paths not in EXPECTED_BOOTSTRAP_PATHS "
            f"(add them here if intentional): {sorted(extra)}"
        )
    if msg_parts:
        pytest.fail(" / ".join(msg_parts))
