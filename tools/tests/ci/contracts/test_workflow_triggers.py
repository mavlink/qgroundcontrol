"""Trigger filters that several events or workflows must keep in step."""

from __future__ import annotations

from typing import Any

from ci.planning.detect_changes import PYTHON_ENVIRONMENT_FILES

from ..._helpers import evaluate_expression, load_workflow, workflow_names

# Paths that deliberately start only a push build.
PUSH_ONLY_PATHS = {
    # RunsOn reads runs-on.yml from the default branch, so a pull request would not exercise it.
    "vm-builds.yml": {".github/runs-on.yml"},
    # A pull request that changes only the locked Python environment builds Linux alone.
    "devcontainer.yml": set(PYTHON_ENVIRONMENT_FILES),
}
# The two forms of the Stable* branch check: change detection, whose verdict the build jobs read,
# and the script that fails its job.
REUSABLE_DETECTION = "./.github/workflows/_detect-changes.yml"
RELEASE_BRANCH_CHECK = "tools/ci/publish/release_branch_check.py"
TAG_PUSH = {"github": {"event_name": "push", "ref_type": "tag"}}


def _triggers(name: str) -> dict:
    triggers = load_workflow(name)["on"]
    return triggers if isinstance(triggers, dict) else {}


def test_push_and_pull_request_path_filters_agree() -> None:
    """A path added to one event alone would build on master without first building on its pull request."""
    compared = set()
    for name in workflow_names():
        push = (_triggers(name).get("push") or {}).get("paths")
        pull_request = (_triggers(name).get("pull_request") or {}).get("paths")
        if push is None or pull_request is None:
            continue
        push_only = PUSH_ONLY_PATHS.get(name, set())
        assert push_only <= set(push), name
        assert [path for path in push if path not in push_only] == pull_request, name
        compared.add(name)
    assert set(PUSH_ONLY_PATHS) <= compared


def test_release_tags_start_every_tag_workflow_through_one_filter() -> None:
    """release.yml drafts the release for the tags whose packages the platform workflows build."""
    filters = {
        name: tags
        for name in workflow_names()
        if (tags := (_triggers(name).get("push") or {}).get("tags"))
    }
    assert "release.yml" in filters
    assert len({tuple(tags) for tags in filters.values()}) == 1, filters


def _checks_the_stable_branch(job: dict[str, Any]) -> bool:
    """Whether *job* runs on every event and, on a tag push, checks the tag before anything else."""
    if "if" in job:
        return False
    if job.get("uses") == REUSABLE_DETECTION:
        return True
    runs = [step for step in job.get("steps") or [] if "run" in step]
    return (
        bool(runs)
        and RELEASE_BRANCH_CHECK in runs[0]["run"]
        and evaluate_expression(runs[0].get("if", "true"), TAG_PUSH) is True
    )


def _needed_jobs(jobs: dict[str, Any], job_id: str) -> set[str]:
    """Every job that *job_id* waits for, directly or through another job."""
    needs = jobs[job_id].get("needs") or []
    direct = {needs} if isinstance(needs, str) else set(needs)
    return direct.union(*(_needed_jobs(jobs, needed) for needed in direct))


def test_release_tag_workflows_check_the_stable_branch_before_their_other_jobs() -> None:
    """A vX.Y.Z tag that no Stable* branch contains is not built, published, or drafted as a release."""
    checked = set()
    for name in workflow_names():
        if not (_triggers(name).get("push") or {}).get("tags"):
            continue
        jobs = load_workflow(name)["jobs"]
        gates = {job_id for job_id, job in jobs.items() if _checks_the_stable_branch(job)}
        assert gates, name
        for job_id in jobs.keys() - gates:
            assert _needed_jobs(jobs, job_id) & gates, f"{name}:{job_id}"
        checked.add(name)
    assert {"linux.yml", "release.yml"} <= checked
