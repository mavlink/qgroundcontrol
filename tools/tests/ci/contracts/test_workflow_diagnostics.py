"""Optional diagnostics never fail a build, and interactive debugging stays opt-in and private."""

from ..._helpers import load_workflow, workflow_names
from .._workflows import local_action, step_groups, uses_action


def test_runner_debugging_ends_failed_branch_builds_only_when_a_dispatch_asks():
    debugged = 0
    for name in workflow_names():
        workflow = load_workflow(name)
        for job_id, job in workflow["jobs"].items():
            steps = job.get("steps") or []
            if not any(local_action(step) == "debug-runner" for step in steps):
                continue
            debug = steps[-1]
            assert local_action(debug) == "debug-runner", f"{name}:{job_id}"
            condition = debug["if"].removeprefix("${{ ").removesuffix(" }}")
            # Tag builds hold the release secrets, so only branch builds may open a session.
            assert {
                "failure()",
                "inputs.debug_runner",
                "github.ref_type == 'branch'",
            } <= set(condition.split(" && ")), f"{name}:{job_id}"
            # A forgotten session must not hold the runner until the job times out.
            assert "timeout-minutes" in debug, f"{name}:{job_id}"
            option = workflow["on"]["workflow_dispatch"]["inputs"]["debug_runner"]
            assert (option["type"], option["default"]) == ("boolean", False), name
            debugged += 1
    assert debugged


def test_debug_sessions_admit_only_the_actor():
    sessions = [
        step
        for _, _, steps in step_groups()
        for step in steps
        if uses_action(step, "mxschmitt/action-tmate")
    ]
    assert sessions
    for step in sessions:
        assert step["with"]["limit-access-to-actor"] is True


def test_build_timelines_never_fail_a_job_and_may_read_its_run():
    timelines = 0
    for label, job, steps in step_groups(actions=False):
        for step in steps:
            if uses_action(step, "Kesin11/actions-timeline"):
                assert step["continue-on-error"] is True, label
                assert job["permissions"]["actions"] == "read", label
                timelines += 1
    assert timelines
