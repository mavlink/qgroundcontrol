"""Publish workflow contracts: which repositories and runs publish, and which only report."""

from __future__ import annotations

import re
from typing import Any

import pytest
from ci.publish.aws_upload import RECORD_READ_VARIABLE
from ci.publish.publish_set import PUBLISHABLE_CONCLUSIONS

from ..._helpers import (
    REPO_ROOT,
    evaluate_expression,
    get_step,
    get_steps,
    load_workflow,
)

WORKFLOW = "publish.yml"
UPSTREAM = "mavlink/qgroundcontrol"
FORK = "someone/qgroundcontrol"


def _context(
    event_name: str,
    *,
    repo: str = UPSTREAM,
    ref_name: str = "master",
    ref_type: str = "branch",
    dry_run: bool | None = None,
    force_latest: bool | None = None,
    force_release: bool | None = None,
    opt_in: str = "",
    build: dict[str, Any] | None = None,
) -> dict[str, Any]:
    """The contexts of a run of *repo*, whose default branch is master.

    A workflow_run run is always on the default branch; a dispatch names its own ref. The
    build that completed is a push build of *repo* that passed, where *build* does not differ.
    """
    event: dict[str, Any] = {"repository": {"default_branch": "master"}}
    if event_name == "workflow_run":
        event["workflow_run"] = {
            "event": "push",
            "conclusion": "success",
            "head_repository": {"full_name": repo},
            **(build or {}),
        }
    return {
        "github": {
            "event_name": event_name,
            "repository": repo,
            "repository_owner": repo.split("/")[0],
            "ref_name": ref_name,
            "ref_type": ref_type,
            "event": event,
        },
        "inputs": {
            "dry_run": dry_run,
            "force_latest": force_latest,
            "force_release": force_release,
        },
        "vars": {"QGC_PUBLISH_DAILY_RELEASE": opt_in},
    }


def _decide_env(name: str) -> str:
    return get_step(get_steps(load_workflow(WORKFLOW), "decide"), "decide")["env"][name]


@pytest.mark.parametrize(
    ("context", "runs"),
    [
        (_context("workflow_run"), True),
        (_context("workflow_dispatch"), True),
        (_context("workflow_run", repo=FORK), False),
        (_context("workflow_run", repo=FORK, opt_in="true"), True),
        # Anyone can ask what would be published.
        (_context("workflow_dispatch", repo=FORK), True),
    ],
)
def test_a_fork_decides_nothing_for_its_builds_unless_it_opts_in(
    context: dict[str, Any], runs: bool
) -> None:
    condition = load_workflow(WORKFLOW)["jobs"]["decide"]["if"]
    assert bool(evaluate_expression(condition, context)) is runs


@pytest.mark.parametrize(
    ("context", "runs"),
    [
        (_context("workflow_run"), True),
        # Every pull request's builds complete too, and so do merge queue and manual builds.
        (_context("workflow_run", build={"event": "pull_request"}), False),
        (_context("workflow_run", build={"event": "merge_group"}), False),
        (_context("workflow_run", build={"event": "workflow_dispatch"}), False),
        # A branch is published although a job that builds no package failed.
        (_context("workflow_run", build={"conclusion": "failure"}), True),
        # A build that was stopped, or never ran, says nothing of its commit.
        (_context("workflow_run", build={"conclusion": "cancelled"}), False),
        (_context("workflow_run", build={"conclusion": "timed_out"}), False),
        (_context("workflow_run", build={"conclusion": "skipped"}), False),
        (_context("workflow_run", build={"conclusion": "action_required"}), False),
        (_context("workflow_run", build={"conclusion": "startup_failure"}), False),
        (_context("workflow_run", build={"head_repository": {"full_name": FORK}}), False),
        (_context("workflow_run", build={"head_repository": None}), False),
        # A run started by hand has no build that completed.
        (_context("workflow_dispatch"), True),
        (_context("workflow_dispatch", ref_name="feature"), True),
    ],
)
def test_a_build_that_cannot_publish_starts_no_job(context: dict[str, Any], runs: bool) -> None:
    # publish_decide.py makes the same checks and more, on a runner. These only spare it one.
    jobs = load_workflow(WORKFLOW)["jobs"]
    assert bool(evaluate_expression(jobs["decide"]["if"], context)) is runs
    # No other job starts without decide: each needs it, and reads an output it then lacks.
    for name, job in jobs.items():
        if name != "decide":
            needs = job["needs"] if isinstance(job["needs"], list) else [job["needs"]]
            assert "decide" in needs, name
            assert "needs.decide.outputs." in job["if"] or "needs.verify.outputs." in job["if"]


@pytest.mark.parametrize(
    ("context", "daily", "s3"),
    [
        (_context("workflow_run"), True, True),
        (_context("workflow_run", repo="mavlink/other"), False, True),
        (_context("workflow_run", repo=FORK, opt_in="true"), True, False),
        (_context("workflow_dispatch", repo=FORK), False, False),
    ],
)
def test_a_fork_never_gets_the_download_bucket_and_gets_a_daily_release_only_by_opting_in(
    context: dict[str, Any], daily: bool, s3: bool
) -> None:
    assert bool(evaluate_expression(_decide_env("PUBLISH_DAILY_RELEASE"), context)) is daily
    assert bool(evaluate_expression(_decide_env("PUBLISH_S3"), context)) is s3


@pytest.mark.parametrize(
    ("context", "dry_run"),
    [
        (_context("workflow_run"), False),
        (_context("workflow_dispatch", dry_run=False), False),
        (_context("workflow_dispatch", dry_run=True), True),
        # Another branch's copy of the workflow and scripts, or a tag's, even one named master.
        (_context("workflow_dispatch", ref_name="feature", dry_run=False), True),
        (_context("workflow_dispatch", ref_name="Stable_V5.1", dry_run=False), True),
        (_context("workflow_dispatch", ref_name="master", ref_type="tag", dry_run=False), True),
    ],
)
def test_only_the_default_branch_s_copy_of_the_workflow_publishes(
    context: dict[str, Any], dry_run: bool
) -> None:
    assert bool(evaluate_expression(_decide_env("PUBLISH_DRY_RUN"), context)) is dry_run


@pytest.mark.parametrize(
    "context", [_context("workflow_run"), _context("workflow_dispatch", dry_run=False)]
)
def test_upstream_publishes_for_real_with_no_repository_variable_set(
    context: dict[str, Any],
) -> None:
    assert bool(evaluate_expression(load_workflow(WORKFLOW)["jobs"]["decide"]["if"], context))
    assert not evaluate_expression(_decide_env("PUBLISH_DRY_RUN"), context)
    for target in ("PUBLISH_DAILY_RELEASE", "PUBLISH_RELEASE_ASSETS", "PUBLISH_S3"):
        assert evaluate_expression(_decide_env(target), context), target
    # Nothing else can hold a run back: the forks' opt-in is the only variable the workflow
    # reads, and the decision is passed no setting but these.
    text = (REPO_ROOT / ".github/workflows" / WORKFLOW).read_text(encoding="utf-8")
    assert set(re.findall(r"\bvars\.(\w+)", text)) == {"QGC_PUBLISH_DAILY_RELEASE"}
    decide = get_step(get_steps(load_workflow(WORKFLOW), "decide"), "decide")
    assert set(decide["env"]) == {
        "GH_TOKEN",
        "PUBLISH_REF",
        "PUBLISH_COMMIT",
        "PUBLISH_DRY_RUN",
        "PUBLISH_FORCE_LATEST",
        "PUBLISH_FORCE_RELEASE",
        "PUBLISH_DAILY_RELEASE",
        "PUBLISH_RELEASE_ASSETS",
        "PUBLISH_S3",
    }


def test_what_a_request_names_reaches_the_decision_through_the_environment() -> None:
    workflow = load_workflow(WORKFLOW)
    inputs = workflow["on"]["workflow_dispatch"]["inputs"]
    assert (inputs["ref"]["required"], inputs["commit"]["required"]) == (True, False)
    # Empty unless the request names a commit: the decision then takes the newest finished one.
    assert "default" not in inputs["commit"]
    decide = get_step(get_steps(workflow, "decide"), "decide")
    assert decide["env"]["PUBLISH_REF"] == "${{ inputs.ref }}"
    assert decide["env"]["PUBLISH_COMMIT"] == "${{ inputs.commit }}"
    # Free text never becomes part of a command line: the scripts read it, and check it.
    for name, job in workflow["jobs"].items():
        for step in job["steps"]:
            assert "${{" not in str(step.get("run", "")), (name, step.get("name"))


def test_the_decision_admits_the_conclusions_that_some_ref_is_published_from() -> None:
    # The job's condition spares a runner, and must not turn away a build the script takes.
    condition = load_workflow(WORKFLOW)["jobs"]["decide"]["if"]
    admitted = set(re.findall(r"workflow_run\.conclusion == '(\w+)'", condition))
    assert admitted == set().union(*PUBLISHABLE_CONCLUSIONS.values())


@pytest.mark.parametrize(
    ("name", "variable", "job"),
    [
        ("force_latest", "PUBLISH_FORCE_LATEST", "s3-latest"),
        ("force_release", "PUBLISH_FORCE_RELEASE", "github-release"),
    ],
)
@pytest.mark.parametrize(
    ("event", "run", "value", "force"),
    [
        # A completed build has no inputs, and would not be heard if it had.
        ("workflow_run", {}, None, False),
        ("workflow_run", {}, True, False),
        ("workflow_dispatch", {}, False, False),
        ("workflow_dispatch", {}, True, True),
        # Another branch's copy of the workflow, or a tag's, even one named master.
        ("workflow_dispatch", {"ref_name": "feature"}, True, False),
        ("workflow_dispatch", {"ref_name": "Stable_V5.1"}, True, False),
        ("workflow_dispatch", {"ref_type": "tag"}, True, False),
    ],
)
def test_only_a_run_started_by_hand_on_the_default_branch_can_force_a_target(
    name: str, variable: str, job: str, event: str, run: dict[str, str], value: bool, force: bool
) -> None:
    def context(event: str, forced: str, value: bool | None) -> dict[str, Any]:
        return _context(
            event,
            ref_name=run.get("ref_name", "master"),
            ref_type=run.get("ref_type", "branch"),
            force_latest=value if forced == "force_latest" else None,
            force_release=value if forced == "force_release" else None,
        )

    workflow = load_workflow(WORKFLOW)
    inputs = {"ref", "commit", "dry_run", "force_latest", "force_release"}
    assert set(workflow["on"]["workflow_dispatch"]["inputs"]) == inputs
    other = "force_release" if name == "force_latest" else "force_latest"
    # Where the decision is made, and in the job that writes the target.
    for expression in (_decide_env(variable), workflow["jobs"][job]["env"][variable]):
        assert bool(evaluate_expression(expression, context(event, name, value))) is force
        # Each input forces its own target only.
        assert not evaluate_expression(expression, context("workflow_dispatch", other, True))


def test_a_tag_s_packages_reach_its_github_release_in_a_job_of_their_own() -> None:
    workflow = load_workflow(WORKFLOW)
    job = workflow["jobs"]["github-release"]
    # Independent of the S3 jobs: either fails, and is run again, without the other.
    assert job["needs"] == ["decide", "verify"]
    assert job["if"] == (
        "needs.verify.outputs.publish == 'true' && needs.decide.outputs.release == 'true'"
    )
    assert all(
        "github-release" not in other.get("needs", []) for other in workflow["jobs"].values()
    )
    assert workflow["jobs"]["decide"]["outputs"]["release"] == "${{ steps.decide.outputs.release }}"
    # Release assets are all it writes.
    assert job["permissions"] == {"contents": "write"}
    assert not any("secrets." in str(value) for value in job["env"].values())
    commands = [str(step.get("run", "")) for step in get_steps(workflow, "github-release")]
    confirm = next(i for i, run in enumerate(commands) if "confirm --target release" in run)
    attach = next(i for i, run in enumerate(commands) if "release_assets.py" in run)
    assert confirm < attach
    assert get_steps(workflow, "github-release")[attach]["if"] == (
        "steps.confirm.outputs.proceed == 'true'"
    )


@pytest.mark.parametrize(
    ("context", "attached"),
    [
        (_context("workflow_run"), True),
        (_context("workflow_run", repo=FORK), False),
        # A fork that opted in to GitHub releases of its own gets its tags' packages too.
        (_context("workflow_run", repo=FORK, opt_in="true"), True),
    ],
)
def test_a_fork_s_draft_release_gets_packages_only_when_the_fork_opts_in(
    context: dict[str, Any], attached: bool
) -> None:
    expression = _decide_env("PUBLISH_RELEASE_ASSETS")
    assert bool(evaluate_expression(expression, context)) is attached


def test_the_record_is_read_from_its_public_url_which_needs_no_permission_of_the_role() -> None:
    # aws_upload.py reads with credentials only when its variable or its option says so. The
    # workflow sets neither: the role has not been given the permissions that such a read takes.
    workflow = load_workflow(WORKFLOW)
    settings = [workflow.get("env", {})]
    for job in workflow["jobs"].values():
        settings += [job.get("env", {}), *(step.get("env", {}) for step in job["steps"])]
        assert not any("--record-read" in str(step.get("run", "")) for step in job["steps"])
    assert not any(RECORD_READ_VARIABLE in env for env in settings)


@pytest.mark.parametrize(
    ("job", "upload", "invalidate"),
    [
        ("s3-builds", "aws_upload.py publish ", "aws_upload.py invalidate --distribution-id "),
        ("s3-latest", "aws_upload.py promote ", "aws_upload.py invalidate --latest "),
    ],
)
def test_a_directory_is_invalidated_after_its_upload_and_also_when_that_had_nothing_to_do(
    job: str, upload: str, invalidate: str
) -> None:
    steps = get_steps(load_workflow(WORKFLOW), job)
    commands = [str(step.get("run", "")) for step in steps]
    uploads = next(i for i, run in enumerate(commands) if upload in run)
    invalidates = next(i for i, run in enumerate(commands) if invalidate in run)
    assert uploads < invalidates
    assert [run for run in commands if "aws_upload.py" in run] == [
        commands[uploads],
        commands[invalidates],
    ]
    assert steps[invalidates]["env"] == {"DISTRIBUTION_ID": "${{ secrets.AWS_DISTRIBUTION_ID }}"}
    # After a failed invalidation the re-run job finds the directory up to date and skips the
    # upload. The invalidation must run all the same, with credentials.
    assert steps[uploads]["if"] == "steps.confirm.outputs.proceed == 'true'"
    assert "if" not in steps[invalidates]
    assert get_step(steps, "Configure AWS credentials")["if"] == "env.PUBLISH_DRY_RUN != 'true'"


@pytest.mark.parametrize(
    ("builds", "latest", "runs"),
    [
        ("success", "true", True),
        # builds/<tag>/ already held the release, so its job had nothing to do.
        ("skipped", "true", True),
        ("failure", "true", False),
        ("cancelled", "true", False),
        ("success", "false", False),
    ],
)
def test_latest_follows_a_builds_job_that_passed_or_had_nothing_to_do(
    builds: str, latest: str, runs: bool
) -> None:
    condition = load_workflow(WORKFLOW)["jobs"]["s3-latest"]["if"]
    # Without a status function a job is skipped as soon as a job it needs is.
    assert "!cancelled() &&" in condition
    needs = {
        "decide": {"outputs": {"latest": latest}},
        "verify": {"outputs": {"publish": "true"}},
        "s3-builds": {"result": builds},
    }
    expression = condition.replace("!cancelled() &&", "")
    assert bool(evaluate_expression(expression, {"needs": needs})) is runs
