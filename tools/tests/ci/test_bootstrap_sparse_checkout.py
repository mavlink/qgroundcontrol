"""Drift guard: every workflow or composite action that sparse-checks out
`tools/_bootstrap.py` must also include every `ci.*`, `common.*` and
`qgc_tools.*` module (with its package `__init__.py` files) that the scripts it
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
import fnmatch
from typing import TYPE_CHECKING

import pytest

from .._helpers import REPO_ROOT, action_names, load_workflow, load_yaml, workflow_names

if TYPE_CHECKING:
    from collections.abc import Iterator
    from pathlib import Path

WORKFLOWS_DIR = REPO_ROOT / ".github" / "workflows"
ACTIONS_DIR = REPO_ROOT / ".github" / "actions"
TOOLS_DIR = REPO_ROOT / "tools"

TOOLS_BOOTSTRAP = "tools/_bootstrap.py"
BOOTSTRAP_TRIPWIRES: frozenset[str] = frozenset({TOOLS_BOOTSTRAP})
TOOL_PACKAGES: frozenset[str] = frozenset({"ci", "common", "qgc_tools"})

# Always required when a tripwire is present, regardless of which script runs.
BASE_BOOTSTRAP_PATHS: frozenset[str] = frozenset(
    {
        "tools/_bootstrap.py",
        "tools/common/__init__.py",
        "tools/qgc_tools/__init__.py",
    }
)


def _module_paths(module: str) -> list[str]:
    """Map `ci.pkg.mod` to its existing package `__init__.py` files and module file."""
    parts = module.split(".")
    candidates = [f"tools/{'/'.join(parts[:i])}/__init__.py" for i in range(1, len(parts) + 1)]
    candidates.append(f"tools/{'/'.join(parts)}.py")
    return [rel for rel in candidates if (REPO_ROOT / rel).is_file()]


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


def _iter_tool_imports(source: Path) -> Iterator[str]:
    """Yield `ci`/`common`/`qgc_tools` module names imported at runtime (directly or relatively) by `source`."""
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
            if node.level == 0 and node.module.split(".")[0] in TOOL_PACKAGES:
                yield node.module
                # `from ci.planning import detect_changes` imports a submodule.
                for alias in node.names:
                    yield f"{node.module}.{alias.name}"
            elif node.level >= 1 and inside_common:
                yield f"common.{node.module}"
        elif isinstance(node, ast.Import):
            for alias in node.names:
                if alias.name.split(".")[0] in TOOL_PACKAGES:
                    yield alias.name


def _required_tool_paths(script: Path) -> frozenset[str]:
    """Closure of every tools/ package and module file required to import `script`."""
    required: set[str] = set()
    queue: list[Path] = [script]
    while queue:
        cur = queue.pop()
        for module in _iter_tool_imports(cur):
            for rel in _module_paths(module):
                if rel not in required:
                    required.add(rel)
                    queue.append(REPO_ROOT / rel)
    return frozenset(required)


def _scripts_in_block(entries: frozenset[str]) -> list[Path]:
    """Return every checked-out .py file (excluding the bootstrap) in a sparse-checkout block."""
    return [
        REPO_ROOT / e
        for e in entries
        if e.endswith(".py") and e not in BOOTSTRAP_TRIPWIRES and (REPO_ROOT / e).is_file()
    ]


def _imports_bootstrap(source: Path) -> bool:
    """Whether `source` runs `import _bootstrap as _bootstrap` (standalone stdlib scripts do not)."""
    try:
        tree = ast.parse(source.read_text(encoding="utf-8"))
    except (OSError, SyntaxError):
        return False
    return any(
        isinstance(node, ast.Import) and any(alias.name == "_bootstrap" for alias in node.names)
        for node in ast.walk(tree)
    )


def _missing_paths(required: set[str], entries: frozenset[str]) -> list[str]:
    """Return required files not covered by an exact file or parent-directory entry."""
    return sorted(
        path
        for path in required
        if not any(path == entry or path.startswith(f"{entry.rstrip('/')}/") for entry in entries)
    )


def test_missing_paths_accepts_parent_directory_entries() -> None:
    required = {"tools/common/__init__.py", "tools/common/file_traversal.py"}

    assert _missing_paths(required, frozenset({"tools/common"})) == []
    assert _missing_paths(required, frozenset({"tools/common/__init__.py"})) == [
        "tools/common/file_traversal.py"
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
    sources = [WORKFLOWS_DIR / name for name in workflow_names()]
    sources += [ACTIONS_DIR / name / "action.yml" for name in action_names()]
    for path in sources:
        doc = load_yaml(path)
        if not isinstance(doc, dict):
            continue
        yield from _iter_checkout_steps(doc, str(path.relative_to(REPO_ROOT)))


def test_bootstrap_sparse_checkout_matches_canonical() -> None:
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
        scripts = _scripts_in_block(entries)
        if not entries & BOOTSTRAP_TRIPWIRES and not any(map(_imports_bootstrap, scripts)):
            continue
        required: set[str] = set(BASE_BOOTSTRAP_PATHS)
        # `import common` runs __init__ at runtime; an eager facade that pulls
        # every submodule would need them all present even under a thin sparse set.
        required |= _required_tool_paths(TOOLS_DIR / "common" / "__init__.py")
        for script in scripts:
            required |= _required_tool_paths(script)
        missing = _missing_paths(required, entries)
        if missing:
            drift.append(f"{context} missing: {missing}")

    if drift:
        pytest.fail(
            "Workflows/actions referencing the bootstrap are missing files required by their scripts:\n  "
            + "\n  ".join(drift)
        )


def test_ci_scripts_checkout_includes_packaging_and_action_fixtures() -> None:
    workflow = load_workflow("ci-scripts.yml")
    job = workflow["jobs"]["test-ci-scripts"]
    ((_, entries),) = _iter_checkout_steps({"jobs": {"test-ci-scripts": job}}, "ci-scripts.yml")
    required = {
        ".clusterfuzzlite/build.py",
        ".clusterfuzzlite/read_mavlink_config.py",
        ".github/COPYING.md",
        ".github/actions/run-unit-tests/action.yml",
        "deploy/installer/packages/org.mavlink.qgroundcontrol/meta/installscript.js",
        "deploy/macos/MacOSXBundleInfo.plist.in",
        "deploy/multipass/run_multipass.py",
    }
    assert not _missing_paths(required, entries)
    for event in ("pull_request", "push"):
        paths = frozenset(workflow["on"][event]["paths"])
        assert all(
            any(fnmatch.fnmatchcase(path, pattern) for pattern in paths) for path in required
        )


BUILD_RESULTS_YML = WORKFLOWS_DIR / "build-results.yml"

EXPECTED_BUILD_RESULTS_PATHS: frozenset[str] = frozenset(
    {
        ".github/actions/setup-python",
        # The reporting scripts read build.platform_workflows.
        ".github/build-config.json",
        "tools/ci/reporting/report_context.py",
        "tools/_bootstrap.py",
        "tools/ci/reporting/collect_build_status.py",
        "tools/ci/reporting/download_artifacts.py",
        "tools/ci/reporting/generate_build_results_comment.py",
        "tools/ci/reporting/junit_report.py",
        "tools/ci/reporting/workflow_runs.py",
        "tools/common/__init__.py",
        "tools/qgc_tools/__init__.py",
        "tools/common/build_config.py",
        "tools/common/coverage_summary.py",
        "tools/common/file_traversal.py",
        "tools/common/format.py",
        "tools/common/gh_actions.py",
        "tools/qgc_tools/python_env.py",
        "tools/common/io.py",
        "tools/common/markdown.py",
        "tools/common/platform.py",
        "tools/common/proc.py",
        "tools/common/xml.py",
        "tools/pyproject.toml",
        "tools/uv.lock",
        "tools/setup/install_python.py",
        "tools/ci/cache/cache_backend.py",
    }
)


def test_build_results_jobs_check_out_once_before_local_actions() -> None:
    """The comment job's only checkout covers its actions, locks and Python import closure."""
    job, expected_actions = "post-pr-comment", {".github/actions/setup-python"}

    steps = load_workflow(BUILD_RESULTS_YML.name)["jobs"][job]["steps"]
    checkouts = [
        index
        for index, step in enumerate(steps)
        if str(step.get("uses", "")).startswith("actions/checkout")
    ]
    assert len(checkouts) == 1
    blocks = dict(_iter_checkout_steps({"jobs": {job: {"steps": steps}}}, "build-results.yml"))
    (actual,) = blocks.values()
    assert not _missing_paths(set(EXPECTED_BUILD_RESULTS_PATHS), actual)

    local_actions = {
        index: step["uses"].removeprefix("./")
        for index, step in enumerate(steps)
        if str(step.get("uses", "")).startswith("./")
    }
    assert set(local_actions.values()) == expected_actions
    assert all(index > checkouts[0] for index in local_actions)
    assert not _missing_paths(expected_actions, actual)
