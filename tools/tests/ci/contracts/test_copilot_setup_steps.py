"""Copilot setup steps workflow contracts."""

from __future__ import annotations

import re
from pathlib import Path

from ci.runner import apt_debs_artifact, apt_install_retry

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


def test_code_review_skips_the_cloud_agent_build_setup() -> None:
    """The review's ~30-minute credential must cover the review, not a Qt and CMake setup."""
    steps = load_workflow("copilot-code-review.yml")["jobs"]["copilot-setup-steps"]["steps"]
    assert not any("run" in step for step in steps)
    assert not any(step.get("uses", "").startswith("./.github/actions/") for step in steps)


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
    slow = [
        "Build Setup",
        "Restore apt downloads",
        "Install Dependencies",
        "Restore prek environments",
        "Configure",
    ]
    assert [step["name"] for step in steps if "timeout-minutes" in step] == slow
    shared = [name for name in slow if "if" not in get_step(steps, name)]

    def limits(event: str, names: list[str]) -> list[int]:
        context = {"github": {"event_name": event}}
        return [
            evaluate_expression(str(get_step(steps, name)["timeout-minutes"]), context)
            for name in names
        ]

    assert sum(limits("dynamic", slow)) < 20
    # Each keeps headroom over its usual session time (about 1.5, 0.5, 1, 0.2, and 2.5 minutes).
    assert all(limit >= 2 for limit in limits("dynamic", slow))
    # The master runs that save the sessions' caches must still finish on a slow mirror.
    session = limits("dynamic", shared)
    for event in ("schedule", "push", "workflow_dispatch"):
        cache_warming = limits(event, shared)
        assert all(warm > tight for warm, tight in zip(cache_warming, session, strict=True))
        assert sum(cache_warming) < 59


def test_sessions_restore_the_apt_downloads_master_runs_upload() -> None:
    """GitHub evicts the apt cache from the full pool within hours; artifacts are not evicted."""
    job = load_workflow("copilot-setup-steps.yml")["jobs"]["copilot-setup-steps"]
    steps = job["steps"]
    restore = get_step(steps, "Restore apt downloads")
    install = get_step(steps, "Install Dependencies")
    upload = get_step(steps, "Upload apt downloads")
    assert steps.index(restore) < steps.index(install) < steps.index(upload)
    # The restore reads the artifact list; without it the step only warns.
    assert job["permissions"]["actions"] == "read"
    assert restore["if"] == (
        "github.event_name == 'dynamic' || github.event_name == 'workflow_dispatch'"
    )
    # A failed setup step makes Copilot skip the rest of the setup.
    assert restore["continue-on-error"] is True
    assert restore["run"] == 'python3 "${GITHUB_WORKSPACE}/tools/ci/runner/apt_debs_artifact.py"'
    # Restores trust only master's runs.
    assert upload["if"] == "github.event_name != 'dynamic' && github.ref == 'refs/heads/master'"
    assert upload["with"]["name"] == apt_debs_artifact.ARTIFACT_NAME
    upload_dir = Path(upload["with"]["path"].replace("~", str(Path.home()), 1)).parent
    assert upload_dir == apt_install_retry.archives_dir()
    assert (REPO_ROOT / apt_debs_artifact.PRODUCER_WORKFLOW).is_file()
