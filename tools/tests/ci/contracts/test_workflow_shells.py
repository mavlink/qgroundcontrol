"""Workflow and composite-action run steps keep the status of a command piped into tee."""

from __future__ import annotations

import re
from typing import TYPE_CHECKING, Any

from ..._helpers import action_names, load_action, load_workflow, workflow_names

if TYPE_CHECKING:
    from collections.abc import Iterator

TEE = re.compile(r"\|\s*tee\b")


def _default_shell(document: dict[str, Any], inherited: str | None = None) -> str | None:
    return (document.get("defaults") or {}).get("run", {}).get("shell", inherited)


def _run_steps() -> Iterator[tuple[str, str | None, str]]:
    """Yield (location, effective shell, script) for every run step."""
    for name in workflow_names():
        workflow = load_workflow(name)
        workflow_shell = _default_shell(workflow)
        for job_name, job in (workflow.get("jobs") or {}).items():
            job_shell = _default_shell(job, workflow_shell)
            for step in job.get("steps", []):
                if "run" in step:
                    where = f"{name}:{job_name}:{step.get('name')}"
                    yield where, step.get("shell", job_shell), step["run"]
    for name in action_names():
        for step in load_action(name)["runs"].get("steps", []):
            if "run" in step:
                yield f"{name}:{step.get('name')}", step.get("shell"), step["run"]


def test_steps_piping_into_tee_keep_the_command_status() -> None:
    # The default `bash -e {0}` has no pipefail, so `command | tee log` would exit with tee's
    # status; an explicit `shell: bash` runs `bash --noprofile --norc -eo pipefail {0}`.
    teed = {where: shell for where, shell, run in _run_steps() if TEE.search(run)}
    assert "analysis.yml:analyze:Run analysis" in teed
    assert {where: shell for where, shell in teed.items() if shell != "bash"} == {}
