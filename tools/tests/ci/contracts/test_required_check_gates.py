"""Each always-triggered build workflow ends in the gate job that branch protection requires.

A skipped matrix job reports its unexpanded name, so the ruleset requires these gate names instead
of build job names. Renaming a gate, or a gate that misses a job, would block or bypass every PR.
"""

from __future__ import annotations

import pytest

from ..._helpers import load_workflow

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
    # A path-filtered trigger would leave the required check pending forever.
    assert workflow["on"]["pull_request"] is None
    assert "merge_group" in workflow["on"]

    jobs = workflow["jobs"]
    gate = jobs.pop(gate_id)
    assert gate["name"] == check_name
    assert gate["if"] == "always()"
    assert gate["permissions"] == {}
    assert sorted(gate["needs"]) == sorted(jobs)

    harden, check = gate["steps"]
    assert harden["uses"] == "step-security/harden-runner@v2"
    assert check["uses"] == "re-actors/alls-green@v1.3.0"
    assert check["with"]["jobs"] == "${{ toJSON(needs) }}"
    # Only jobs that change detection or the planner can skip may be skipped.
    conditional = sorted(name for name, job in jobs.items() if "if" in job)
    assert sorted(check["with"]["allowed-skips"].split(", ")) == conditional
