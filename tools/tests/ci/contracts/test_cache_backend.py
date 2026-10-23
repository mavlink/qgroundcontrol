"""setup-python runs the cache backend probe with a JavaScript action's environment."""

from __future__ import annotations

import re
from typing import Any

import pytest

from ..._helpers import REPO_ROOT, get_steps, load_action, load_workflow, workflow_names

SETUP_PYTHON = ".github/actions/setup-python"
PROBE = "tools/ci/cache/cache_backend.py"


def test_probe_runs_inside_a_javascript_action_before_setup_uv() -> None:
    probe, setup_uv, *_ = get_steps(load_action("setup-python"))
    # A shell step would not see the URLs the runner injects into JavaScript actions.
    assert probe["uses"].startswith("actions/github-script@")
    assert probe["with"]["script"] == f"await exec.exec('python3', ['{PROBE}'])"
    assert (REPO_ROOT / PROBE).is_file()
    assert setup_uv["uses"].startswith("astral-sh/setup-uv@")
    for name in ("enable-cache", "cache-python"):
        assert setup_uv["with"][name] == "${{ env.QGC_ACTIONS_CACHE_BACKEND == 's3' }}"


def _uses_setup_python(steps: list[dict[str, Any]], seen: tuple[str, ...] = ()) -> bool:
    for step in steps:
        uses = str(step.get("uses", ""))
        if uses == f"./{SETUP_PYTHON}":
            return True
        name = uses.removeprefix("./.github/actions/").strip("/")
        if (
            uses.startswith("./.github/actions/")
            and name not in seen
            and _uses_setup_python(get_steps(load_action(name)), (*seen, name))
        ):
            return True
    return False


def _sparse_checkouts_of_setup_python_jobs() -> list[Any]:
    params = []
    for workflow in workflow_names():
        for name, job in (load_workflow(workflow).get("jobs") or {}).items():
            steps = job.get("steps") or []
            if not _uses_setup_python(steps):
                continue
            for step in steps:
                paths = (step.get("with") or {}).get("sparse-checkout")
                if str(step.get("uses", "")).startswith("actions/checkout") and paths:
                    params.append(pytest.param(set(paths.split()), id=f"{workflow}:{name}"))
    return params


@pytest.mark.parametrize("entries", _sparse_checkouts_of_setup_python_jobs())
def test_sparse_checkouts_of_setup_python_jobs_include_the_files_it_runs(
    entries: set[str],
) -> None:
    action = (REPO_ROOT / SETUP_PYTHON / "action.yml").read_text(encoding="utf-8")
    required = {SETUP_PYTHON, *re.findall(r"tools/[\w./-]+\.(?:py|toml|lock)", action)}
    assert PROBE in required
    missing = {
        file
        for file in required
        if not any(file == entry or file.startswith(f"{entry.rstrip('/')}/") for entry in entries)
    }
    assert not missing
