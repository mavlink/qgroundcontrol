"""Repository-wide rules for the runs a newer run may cancel, the runs that upload to an app store, and the token that opens bot pull requests."""

from __future__ import annotations

import re

from ..._helpers import evaluate_expression, load_workflow, workflow_names
from .._workflows import step_groups, uses_action


def _cancels_in_progress(concurrency: object, event: str, ref: str) -> bool:
    cancel = (
        concurrency.get("cancel-in-progress", False) if isinstance(concurrency, dict) else False
    )
    if isinstance(cancel, bool):
        return cancel
    return bool(evaluate_expression(cancel, {"github": {"event_name": event, "ref": ref}}))


def test_a_newer_run_cancels_only_a_pull_requests_superseded_run() -> None:
    # A cancelled push, merge-group, scheduled or manual run leaves its commit unbuilt or its
    # release half published, and so does a cancelled run that another workflow's completion
    # started. Workflows started only by hand or by a timer replace their own runs as they see fit.
    refs = {
        "push": ["refs/heads/master", "refs/heads/Stable_V5.0", "refs/tags/v5.0.0"],
        "merge_group": ["refs/heads/gh-readonly-queue/master/pr-1-0123abc"],
        "schedule": ["refs/heads/master"],
        "workflow_dispatch": ["refs/heads/master", "refs/tags/v5.0.0"],
        "workflow_run": ["refs/heads/master"],
    }
    pull_request_blocks = 0
    for name in workflow_names():
        workflow = load_workflow(name)
        triggers = workflow["on"]
        events = {triggers} if isinstance(triggers, str) else set(triggers)
        if not events & {"push", "pull_request", "merge_group", "workflow_run"}:
            continue
        blocks = [workflow.get("concurrency")]
        blocks += [job.get("concurrency") for job in workflow["jobs"].values()]
        for block in filter(None, blocks):
            for event in sorted(events & set(refs)):
                for ref in refs[event]:
                    assert not _cancels_in_progress(block, event, ref), f"{name}: {event} on {ref}"
            if "pull_request" in events:
                assert _cancels_in_progress(block, "pull_request", "refs/pull/1/merge"), name
                pull_request_blocks += 1
    assert pull_request_blocks


def _group(concurrency: dict, event: str, ref: str, run_id: int) -> str:
    github = {
        "workflow": "Workflow",
        "event_name": event,
        "ref": ref,
        "ref_type": "tag" if ref.startswith("refs/tags/") else "branch",
        "run_id": run_id,
    }
    return re.sub(
        r"\$\{\{(.*?)\}\}",
        lambda expression: str(evaluate_expression(expression[1], {"github": github})),
        concurrency["group"],
    )


# Workflows whose push runs publish in their own jobs, as the newest push must do last.
_PUSH_RUNS_PUBLISH = {"devcontainer.yml", "docker.yml"}
# Workflows that publish from a release tag's push run only (TestFlight); their branch pushes
# have a group each.
_TAG_PUSH_RUNS_PUBLISH = {"ios.yml"}


def test_a_run_started_by_hand_never_displaces_a_push_run_that_is_waited_for() -> None:
    # A group keeps one waiting run, which a newer run of the group replaces. Publish Builds
    # publishes a commit from the push runs of the workflows it lists and counts no other
    # run. A run started by hand must therefore not wait in a ref's group: it would replace
    # the push run that waits there, and hold the group ahead of the pushes that follow.
    waited_for = set(load_workflow("publish.yml")["on"]["workflow_run"]["workflows"])
    published = _PUSH_RUNS_PUBLISH | _TAG_PUSH_RUNS_PUBLISH
    checked = set()
    for name in workflow_names():
        workflow = load_workflow(name)
        if workflow["name"] not in waited_for and name not in published:
            continue
        assert {"push", "workflow_dispatch"} <= set(workflow["on"]), name
        block = workflow["concurrency"]
        for ref in ("refs/heads/master", "refs/heads/Stable_V5.1", "refs/tags/v5.1.5"):
            push = _group(block, "push", ref, 1)
            by_hand = _group(block, "workflow_dispatch", ref, 3)
            # The pushes to one ref take turns; a run started by hand waits for nobody.
            if name not in _TAG_PUSH_RUNS_PUBLISH or ref.startswith("refs/tags/"):
                assert _group(block, "push", ref, 2) == push, f"{name}: {ref}"
            assert by_hand != push, f"{name}: {ref}"
            assert _group(block, "workflow_dispatch", ref, 4) != by_hand, f"{name}: {ref}"
        checked.add(name)
    assert len(checked) == len(waited_for) + len(published)


# What uploads a build to an app store: the actions that do, the tools a script would call, and
# the service account that may write to the Play Store.
_STORE_UPLOAD_ACTIONS = ("r0adkll/upload-google-play", "apple-actions/upload-testflight-build")
_STORE_UPLOAD_TOOL = re.compile(r"\b(?:altool|iTMSTransporter|fastlane|androidpublisher)\b")
_STORE_UPLOAD_SECRET = "secrets.GOOGLE_SERVICE_ACCOUNT"


def _uploads_to_a_store(step: dict) -> bool:
    given = f"{step.get('with', '')}{step.get('env', '')}{step.get('run', '')}"
    return (
        any(uses_action(step, action) for action in _STORE_UPLOAD_ACTIONS)
        or bool(_STORE_UPLOAD_TOOL.search(str(step.get("run", ""))))
        or _STORE_UPLOAD_SECRET in given
    )


def _conjuncts(condition: str) -> list[str]:
    """The operands of a condition that is a conjunction at its top level, else the condition."""
    text = condition.strip().removeprefix("${{").removesuffix("}}")
    operands, depth, start = [], 0, 0
    for match in re.finditer(r"'(?:[^']|'')*'|\(|\)|&&|\|\|", text):
        depth += {"(": 1, ")": -1}.get(match[0], 0)
        if depth == 0 and match[0] == "||":
            return [" ".join(text.split())]
        if depth == 0 and match[0] == "&&":
            operands.append(text[start : match.start()])
            start = match.end()
    return [" ".join(operand.split()) for operand in [*operands, text[start:]]]


def test_a_build_is_uploaded_to_an_app_store_only_by_a_push() -> None:
    # A tag's push starts the Play Store production rollout and the TestFlight upload, a
    # master push the Play Store beta release. A run of the same ref started by hand, by a
    # pull request, the merge queue or a timer would release that build a second time, so
    # each upload step names the push event itself, among conditions that must all hold.
    uploads = {}
    for label, _, steps in step_groups():
        for step in steps:
            if _uploads_to_a_store(step):
                uploads[label, step.get("name")] = _conjuncts(str(step.get("if", "")))
    assert {
        ("android.yml:build", "Deploy to Play Store"),
        ("ios.yml:build", "Upload to TestFlight"),
    } <= set(uploads)
    for upload, conditions in uploads.items():
        assert "github.event_name == 'push'" in conditions, upload


def _writes(permissions: object) -> bool:
    if isinstance(permissions, dict):
        return "write" in permissions.values()
    return permissions == "write-all"


# Workflows whose waiting job may give way to a newer one, which does the same work for the same
# target from the state it finds.
_NEWEST_WAITING_JOB_SUFFICES = {
    "build-results.yml": "a job renders the pull request's comment or review from its current runs",
    "pages.yml": "a deploy rebuilds the whole site",
}


def test_jobs_another_workflow_starts_with_write_access_take_turns_without_being_replaced() -> None:
    # Such a job publishes on behalf of the run that completed (a comment, the Pages site, the
    # builds). Two at once would interleave, so it needs a group. A group keeps one waiting job
    # only, unless it queues: where an earlier job decides whether the run acts at all, a
    # workflow-level group would let a run with nothing to do replace a waiting one that acts,
    # so the group must be the acting job's own. And where the waiting job publishes something
    # that no later job does (one commit's builds), the group must queue.
    checked = 0
    replaceable = set()
    for name in workflow_names():
        workflow = load_workflow(name)
        if "workflow_run" not in workflow["on"]:
            continue
        jobs = workflow["jobs"]
        decides = any("needs." in str(job.get("if", "")) for job in jobs.values())
        if decides:
            assert "concurrency" not in workflow, name
        for job_id, job in jobs.items():
            if not _writes(job.get("permissions", workflow.get("permissions"))):
                continue
            block = job.get("concurrency") or workflow.get("concurrency")
            assert isinstance(block, dict) and block.get("group"), f"{name}:{job_id}"
            assert block.get("cancel-in-progress") is False, f"{name}:{job_id}"
            if block.get("queue") != "max":
                assert name in _NEWEST_WAITING_JOB_SUFFICES, f"{name}:{job_id} must queue"
                replaceable.add(name)
            checked += 1
    assert checked
    assert replaceable == set(_NEWEST_WAITING_JOB_SUFFICES)


def test_bot_pull_requests_start_their_checks() -> None:
    # Pull requests opened with GITHUB_TOKEN start no workflows, so required checks never report.
    # Only the upstream repository holds the PX4BuildBot token that opens them instead.
    creators = 0
    for label, job, steps in step_groups(actions=False):
        for step in steps:
            if uses_action(step, "peter-evans/create-pull-request"):
                assert "secrets.PX4BUILDBOT_ACCESSTOKEN" in step["with"].get("token", ""), label
                assert "github.repository_owner == 'mavlink'" in str(job.get("if")), label
                creators += 1
    assert creators
