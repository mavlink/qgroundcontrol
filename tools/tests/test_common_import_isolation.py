"""Regression guards for explicit, isolated imports from ``common``.

CI setup scripts run on system Python (pre-venv, possibly 3.10) and under
sparse checkouts that contain only a subset of `tools/common/*.py`. Importing
one submodule must therefore NOT drag in siblings — otherwise a missing file
(ModuleNotFoundError) or a newer-Python-only feature in an unrelated submodule
(e.g. `enum.StrEnum` in common.logging) breaks scripts that never used it.
"""

from __future__ import annotations

import ast
import subprocess
import sys

from ._helpers import REPO_ROOT, TOOLS_DIR

CI_CRITICAL_SUBMODULES = [
    "artifact_metadata",
    "aws",
    "gh_actions",
    "build_config",
    "cmake",
    "cobertura",
    "git",
    "proc",
    "github_runs",
    "env",
    "format",
    "io",
    "tool_version",
]


def _loaded_common_submodules(import_stmt: str) -> set[str]:
    code = (
        "import sys\n"
        f"sys.path.insert(0, {str(TOOLS_DIR)!r})\n"
        f"{import_stmt}\n"
        "print(chr(10).join(m for m in sys.modules if m.startswith('common.')))\n"
    )
    out = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True, check=True)
    return {line.strip() for line in out.stdout.splitlines() if line.strip()}


def test_common_imports_remain_isolated():
    assert _loaded_common_submodules("import common") == set()
    for submodule in CI_CRITICAL_SUBMODULES:
        loaded = _loaded_common_submodules(f"import common.{submodule}")
        assert "common.logging" not in loaded, (
            f"importing common.{submodule} transitively loaded common.logging; "
            "setup scripts under sparse checkouts must not pull unrelated submodules"
        )


def test_production_tools_import_common_submodules_explicitly() -> None:
    """Keep runtime dependencies visible to sparse-checkout validation."""
    roots = (TOOLS_DIR, REPO_ROOT / ".github" / "scripts")
    violations: list[str] = []
    for root in roots:
        for path in root.rglob("*.py"):
            if any(part in {".venv", "__pycache__", "skills", "tests"} for part in path.parts):
                continue
            tree = ast.parse(path.read_text(encoding="utf-8"))
            for node in ast.walk(tree):
                if isinstance(node, ast.ImportFrom) and node.level == 0 and node.module == "common":
                    violations.append(f"{path.relative_to(REPO_ROOT)}:{node.lineno}")
    assert not violations, "import from the defining common.<module> instead: " + ", ".join(
        violations
    )


def test_ci_entrypoints_do_not_import_each_other() -> None:
    """Keep reusable logic in common instead of coupling standalone CI scripts."""
    scripts_dir = REPO_ROOT / ".github" / "scripts"
    script_names = {path.stem for path in scripts_dir.glob("*.py")}
    script_names.discard("ci_bootstrap")
    violations: list[str] = []
    for path in scripts_dir.glob("*.py"):
        tree = ast.parse(path.read_text(encoding="utf-8"))
        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                imported = [alias.name.split(".", maxsplit=1)[0] for alias in node.names]
                line = node.lineno
            elif isinstance(node, ast.ImportFrom) and node.level == 0 and node.module:
                imported = [node.module.split(".", maxsplit=1)[0]]
                line = node.lineno
            else:
                continue
            for module in imported:
                if module in script_names:
                    violations.append(f"{path.relative_to(REPO_ROOT)}:{line} imports {module}")

    assert not violations, "move shared CI logic into tools/common: " + ", ".join(violations)
