"""Copilot setup steps workflow contracts."""

from __future__ import annotations

import re

from ..._helpers import REPO_ROOT, evaluate_expression, get_step, load_workflow


def test_master_runs_warm_the_caches_copilot_sessions_restore() -> None:
    """Sessions restore only default-branch GitHub caches, and master's Linux builds save to S3."""
    on = load_workflow("copilot-setup-steps.yml")["on"]
    # No pull_request trigger: build-results.yml would then have to wait for this workflow.
    assert set(on) == {"workflow_dispatch", "schedule", "push"}
    assert on["schedule"] == [{"cron": "17 4 * * *"}]
    assert on["push"]["branches"] == ["master"]
    paths = set(on["push"]["paths"])
    assert ".github/workflows/copilot-setup-steps.yml" in paths
    # Each action whose cache key or contents feed the sessions re-runs the workflow.
    for action in ("build-setup", "install-dependencies", "qt-install"):
        assert f".github/actions/{action}/**" in paths
    assert {".github/build-config.json", "tools/setup/install_dependencies/**"} <= paths


def test_configures_with_just() -> None:
    workflow = load_workflow("copilot-setup-steps.yml")
    assert get_step(workflow["jobs"]["copilot-setup-steps"]["steps"], "Configure")["run"] == (
        "just configure"
    )
    justfile = (REPO_ROOT / "justfile").read_text(encoding="utf-8")
    recipe = re.search(r"^configure:\n((?:[ \t]+.*\n)+)", justfile, re.MULTILINE)
    assert recipe
    assert "./tools/configure.py" in recipe[1]


def test_session_steps_time_out_inside_copilot_s_setup_budget() -> None:
    """A stalled step must fail inside Copilot's ~20-minute setup budget, not cancel the session."""
    steps = load_workflow("copilot-setup-steps.yml")["jobs"]["copilot-setup-steps"]["steps"]
    slow = ["Build Setup", "Install Dependencies", "Restore prek environments", "Configure"]
    assert [step["name"] for step in steps if "timeout-minutes" in step] == slow

    def limits(event: str) -> list[int]:
        context = {"github": {"event_name": event}}
        return [
            evaluate_expression(get_step(steps, name)["timeout-minutes"], context) for name in slow
        ]

    session = limits("dynamic")
    assert sum(session) < 20
    # Each keeps headroom over its usual session time (about 2, 1.5, 0.2, and 2 minutes).
    assert all(limit >= 2 for limit in session)
    # The master runs that save the sessions' caches must still finish on a slow mirror.
    for event in ("schedule", "push", "workflow_dispatch"):
        cache_warming = limits(event)
        assert all(warm > tight for warm, tight in zip(cache_warming, session, strict=True))
        assert sum(cache_warming) < 59
