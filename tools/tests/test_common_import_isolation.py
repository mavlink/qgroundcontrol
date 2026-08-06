"""Regression guard: the `common` facade must not eagerly import submodules.

CI setup scripts run on system Python (pre-venv, possibly 3.10) and under
sparse checkouts that contain only a subset of `tools/common/*.py`. Importing
one submodule must therefore NOT drag in siblings — otherwise a missing file
(ModuleNotFoundError) or a newer-Python-only feature in an unrelated submodule
(e.g. `enum.StrEnum` in common.logging) breaks scripts that never used it.
"""

from __future__ import annotations

import subprocess
import sys

import pytest

from ._helpers import TOOLS_DIR

CI_CRITICAL_SUBMODULES = [
    "gh_actions",
    "build_config",
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


def test_importing_package_loads_no_submodules():
    assert _loaded_common_submodules("import common") == set()


@pytest.mark.parametrize("sub", CI_CRITICAL_SUBMODULES)
def test_submodule_import_does_not_pull_logging(sub):
    loaded = _loaded_common_submodules(f"import common.{sub}")
    assert "common.logging" not in loaded, (
        f"importing common.{sub} transitively loaded common.logging; "
        f"setup scripts under sparse checkouts must not pull unrelated submodules"
    )
