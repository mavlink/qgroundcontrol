"""Copilot setup steps workflow contracts."""

from __future__ import annotations

from pathlib import Path

from ci.runner import apt_debs_artifact, apt_install_retry

from ..._helpers import REPO_ROOT, evaluate_expression, load_workflow
from .._workflows import LOCAL_ACTION, local_action, uses_action


def _job() -> dict:
    return load_workflow("copilot-setup-steps.yml")["jobs"]["copilot-setup-steps"]


def test_code_review_skips_the_cloud_agent_build_setup() -> None:
    """The review's ~30-minute credential must cover the review, not a Qt and CMake setup."""
    steps = load_workflow("copilot-code-review.yml")["jobs"]["copilot-setup-steps"]["steps"]
    assert not any("run" in step for step in steps)
    assert not any(step.get("uses", "").startswith(LOCAL_ACTION) for step in steps)


def test_session_steps_time_out_inside_copilot_s_setup_budget() -> None:
    """A stalled step must fail inside Copilot's ~20-minute setup budget, not cancel the session."""
    job = _job()
    bounded = [step for step in job["steps"] if "timeout-minutes" in step]
    # No step that runs a script or a local action may stall outside the budget.
    assert all(
        "timeout-minutes" in step for step in job["steps"] if "run" in step or local_action(step)
    )

    def limits(event: str, steps: list[dict]) -> list[int]:
        context = {"github": {"event_name": event}}
        return [evaluate_expression(str(step["timeout-minutes"]), context) for step in steps]

    assert sum(limits("dynamic", bounded)) < 20
    # The master runs that save the sessions' caches must still finish on a slow mirror.
    shared = [step for step in bounded if "if" not in step]
    session = limits("dynamic", shared)
    for event in ("schedule", "push", "workflow_dispatch"):
        cache_warming = limits(event, shared)
        assert all(warm > tight for warm, tight in zip(cache_warming, session, strict=True))
        assert sum(cache_warming) < job["timeout-minutes"]


def test_sessions_restore_the_apt_downloads_master_runs_upload() -> None:
    """GitHub evicts the apt cache from the full pool within hours; artifacts are not evicted."""
    job = _job()
    (restore,) = (step for step in job["steps"] if "apt_debs_artifact.py" in step.get("run", ""))
    (upload,) = (
        step
        for step in job["steps"]
        if uses_action(step, "actions/upload-artifact")
        and step["with"]["name"] == apt_debs_artifact.ARTIFACT_NAME
    )
    assert job["steps"].index(restore) < job["steps"].index(upload)
    # The restore reads the artifact list; without the permission the step only warns.
    assert job["permissions"]["actions"] == "read"
    upload_dir = Path(upload["with"]["path"].replace("~", str(Path.home()), 1)).parent
    assert upload_dir == apt_install_retry.archives_dir()
    assert (REPO_ROOT / apt_debs_artifact.PRODUCER_WORKFLOW).samefile(
        REPO_ROOT / ".github/workflows/copilot-setup-steps.yml"
    )
