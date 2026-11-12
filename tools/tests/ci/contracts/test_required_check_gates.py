"""Each always-triggered build workflow ends in the gate job that a ruleset is meant to require.

A skipped matrix job reports its unexpanded name, so a ruleset should require these gate names
instead of build job names; which checks are required is a repository setting that no file here
records. Where a gate is required, renaming it, or a gate that misses a job, would block or bypass
every PR.
"""

from __future__ import annotations

import pytest

from ..._helpers import load_workflow
from .._workflows import uses_action

GATES = [
    ("linux.yml", "linux-ok", "Linux"),
    ("windows.yml", "windows-ok", "Windows"),
    ("macos.yml", "macos-ok", "MacOS"),
    ("android.yml", "android-ok", "Android"),
    ("ios.yml", "ios-ok", "iOS"),
]


@pytest.mark.parametrize(("workflow_file", "gate_id", "check_name"), GATES)
def test_gate_requires_every_job_and_reports_on_every_pull_request(
    workflow_file: str, gate_id: str, check_name: str
) -> None:
    workflow = load_workflow(workflow_file)
    # A path-filtered trigger would leave a required check pending forever.
    assert workflow["on"]["pull_request"] is None
    assert "merge_group" in workflow["on"]

    jobs = workflow["jobs"]
    gate = jobs.pop(gate_id)
    assert gate["name"] == check_name
    assert gate["if"] == "always()"
    assert sorted(gate["needs"]) == sorted(jobs)

    (check,) = (step for step in gate["steps"] if uses_action(step, "re-actors/alls-green"))
    assert check["with"]["jobs"] == "${{ toJSON(needs) }}"
    # Only jobs that change detection or the planner can skip may be skipped.
    conditional = sorted(name for name, job in jobs.items() if "if" in job)
    assert sorted(check["with"]["allowed-skips"].split(", ")) == conditional
