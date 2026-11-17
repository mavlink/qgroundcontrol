"""Repository-wide workflow policy that zizmor and actionlint do not check."""

from __future__ import annotations

import json
import re
from typing import TYPE_CHECKING, Any

import pytest

from ..._helpers import (
    action_names,
    get_steps,
    load_action,
    load_workflow,
    load_yaml,
    workflow_names,
)
from .._workflows import local_action, step_groups, uses_action, uses_lines, uses_references

if TYPE_CHECKING:
    from collections.abc import Iterator

HARDEN_RUNNER = "step-security/harden-runner"
# Owners whose actions may stay on a version tag, as in .github/zizmor.yml and .github/renovate.json.
GITHUB_OWNERS = {"actions", "github"}
COMMIT_SHA = re.compile(r"[0-9a-f]{40}")
VERSION_TAG = re.compile(r"v\d+(?:\.\d+){0,2}")
# Events whose jobs run with the base repository's token and secrets, whoever opened the pull request.
PRIVILEGED_TRIGGERS = {"pull_request_target", "workflow_run"}
# The workflows that may hold AWS credentials: Publish Builds, and two that build nothing from a
# pull request or a pushed branch.
AWS_WORKFLOWS = {"mirror-gstreamer.yml", "publish.yml", "runner-images.yml"}
AWS_CREDENTIALS = re.compile(
    r"secrets\.AWS_\w+|\./\.github/actions/aws-credentials|aws-actions/configure-aws-credentials"
)


def _executable_jobs() -> Iterator[tuple[str, dict[str, Any], list[dict[str, Any]]]]:
    """Every job with its own steps; a job that calls a reusable workflow has none."""
    for label, job, steps in step_groups(actions=False):
        if "steps" in job:
            yield label, job, steps


def _runson_routes(value: Any) -> Iterator[str]:
    if isinstance(value, dict):
        for key, child in value.items():
            if key in ("runs-on", "runs_on") and isinstance(child, str) and "runs-on=" in child:
                yield child
            else:
                yield from _runson_routes(child)
    elif isinstance(value, list):
        for child in value:
            yield from _runson_routes(child)


def _runs_only_on_runson(job: dict[str, Any]) -> bool:
    """A RunsOn label with no GitHub-hosted fallback, where Harden-Runner could never run."""
    route = job.get("runs-on")
    return isinstance(route, str) and "runs-on=" in route and "||" not in route


def test_executable_jobs_are_bounded_and_hardened() -> None:
    for label, job, steps in _executable_jobs():
        assert steps, f"{label} must have steps"
        timeout = job.get("timeout-minutes")
        assert (isinstance(timeout, int) and not isinstance(timeout, bool) and timeout > 0) or (
            isinstance(timeout, str) and timeout.startswith("${{") and timeout.endswith("}}")
        ), f"{label} must set a positive timeout-minutes"
        if _runs_only_on_runson(job):
            continue
        assert uses_action(steps[0], HARDEN_RUNNER), (
            f"{label} must run {HARDEN_RUNNER} before other steps"
        )


def test_harden_runner_audits_only_github_hosted_runners() -> None:
    # The community tier cannot audit self-hosted runners; RunsOn would need StepSecurity Enterprise.
    hosted_only = "runner.environment == 'github-hosted'"
    for label, job, steps in _executable_jobs():
        if _runs_only_on_runson(job):
            continue
        condition = str(steps[0].get("if", ""))
        if any(_runson_routes(job)):
            assert hosted_only in condition, f"{label} may run on RunsOn"
        else:
            assert "runner.environment" not in condition, label


def test_runson_selection_keeps_independent_forks_on_hosted_runners() -> None:
    upstream = "github.repository_owner == 'mavlink'"
    for name in workflow_names():
        jobs = load_workflow(name)["jobs"]
        for job_id, job in jobs.items():
            if _runs_only_on_runson(job):
                needs = job.get("needs", [])
                needs = [needs] if isinstance(needs, str) else needs
                assert any(upstream in str(jobs[need].get("if", "")) for need in needs), (
                    f"{name}:{job_id} must depend on a job limited to upstream workflows"
                )
                continue
            for runner in _runson_routes(job):
                normalized = " ".join(runner.removeprefix("${{").removesuffix("}}").split())
                assert normalized.startswith(f"{upstream} && "), (
                    f"{name}:{job_id} must limit RunsOn selection to upstream workflows"
                )
                assert "||" in runner, (
                    f"{name}:{job_id} must provide a GitHub-hosted runner fallback"
                )


def _uses_values(value: Any) -> Iterator[str]:
    """Every `uses:` in a document: steps' actions and jobs' reusable workflows."""
    if isinstance(value, dict):
        for key, child in value.items():
            if key == "uses" and isinstance(child, str):
                yield child
            else:
                yield from _uses_values(child)
    elif isinstance(value, list):
        for child in value:
            yield from _uses_values(child)


def _pin_problem(uses: str, comment: str) -> str:
    """Why a `uses:` value and the comment after it break the pin policy; "" when they follow it."""
    if uses.startswith("./"):
        return ""
    action, separator, ref = uses.rpartition("@")
    if not (separator and action):
        return "an external action must include a ref"
    if COMMIT_SHA.fullmatch(ref) and VERSION_TAG.fullmatch(comment):
        return ""
    if action.partition("/")[0] in GITHUB_OWNERS:
        return "" if VERSION_TAG.fullmatch(ref) else "use a version tag, not a branch"
    return "pin to a full commit SHA followed by its version, as in `owner/repo@<sha> # v1.2.3`"


def test_third_party_actions_are_pinned_to_commits_and_githubs_own_to_version_tags() -> None:
    # zizmor requires the commit SHA, but not the version comment Renovate updates the pin from,
    # and its ref-pin policy for GitHub's own actions accepts any ref, a branch included.
    for source, document, references in uses_references():
        # The comment is read from the file's text, so that must show every reference YAML sees.
        assert sorted(uses for uses, _ in references) == sorted(_uses_values(document)), (
            f"{source}: write each `uses:` as a plain value on one line"
        )
        problems = {
            uses: problem
            for uses, comment in references
            if (problem := _pin_problem(uses, comment))
        }
        assert not problems, source


@pytest.mark.parametrize(
    ("line", "allowed"),
    [
        (f"- uses: owner/repo@{'a' * 40} # v1.2.3", True),
        (f"  uses: owner/repo/.github/workflows/called.yml@{'a' * 40} # v1", True),
        ("- uses: actions/checkout@v7", True),
        (f"- uses: github/codeql-action/init@{'a' * 40} # v4.1.0", True),
        ("  uses: ./.github/workflows/called.yml", True),
        # A tag can be moved, and a branch moves by design.
        ("- uses: owner/repo@v1.2.3", False),
        ("  uses: owner/repo/.github/workflows/called.yml@v1", False),
        ("- uses: actions/checkout@main", False),
        # Renovate reads the pinned version from the comment.
        (f"- uses: owner/repo@{'a' * 40}", False),
        (f"- uses: owner/repo@{'a' * 40} # main", False),
        ("- uses: owner/repo", False),
    ],
)
def test_the_pin_policy_reads_each_reference_with_its_version_comment(
    line: str, allowed: bool
) -> None:
    ((uses, comment),) = uses_lines(f"steps:\n      {line}\n        run: echo 'uses: no' # v9\n")
    assert (not _pin_problem(uses, comment)) is allowed


def _steps_with_local_actions(steps: list[dict[str, Any]]) -> Iterator[dict[str, Any]]:
    """The steps, followed by those of every local action they run, at any depth."""
    for step in steps:
        yield step
        if name := local_action(step):
            yield from _steps_with_local_actions(get_steps(load_action(name)))


def test_privileged_workflows_check_out_only_their_own_ref() -> None:
    # These jobs' checkouts must stay the default: the base branch for pull_request_target, the
    # default branch for workflow_run. Another ref or repository could bring in a pull request's
    # code, which the job would then run with the base repository's token.
    checkouts = 0
    for name in workflow_names():
        workflow = load_workflow(name)
        triggers = workflow["on"]
        if not PRIVILEGED_TRIGGERS & set([triggers] if isinstance(triggers, str) else triggers):
            continue
        for job_id, job in workflow["jobs"].items():
            for step in _steps_with_local_actions(job.get("steps") or []):
                if uses_action(step, "actions/checkout"):
                    options = step.get("with") or {}
                    assert not {"ref", "repository"} & set(options), f"{name}:{job_id}"
                    checkouts += 1
    assert checkouts


def test_only_publishing_and_infrastructure_workflows_hold_aws_credentials() -> None:
    # A build job runs whatever the pushed branch or pull request contains, so no build workflow,
    # and no action a build can use, is given credentials for the download bucket.
    holders = {
        name for name in workflow_names() if AWS_CREDENTIALS.search(json.dumps(load_workflow(name)))
    }
    assert holders == AWS_WORKFLOWS
    actions = {
        name for name in action_names() if AWS_CREDENTIALS.search(json.dumps(load_action(name)))
    }
    assert actions == {"aws-credentials"}


def test_registry_attestations_raise_node_s_header_limit() -> None:
    # GHCR's responses overflow Node's default header limit when attestations are pushed.
    pushed = [
        (label, step)
        for label, _, steps in step_groups()
        for step in steps
        if uses_action(step, "actions/attest")
        and (step.get("with") or {}).get("push-to-registry", False) is not False
    ]
    assert pushed
    for label, step in pushed:
        options = (step.get("env") or {}).get("NODE_OPTIONS", "")
        assert "--max-http-header-size=" in options, f"{label}: {step.get('name')}"


def _pr_title_pattern() -> str:
    steps = get_steps(load_workflow("pr-checks.yml"), "title")
    (check,) = (step for step in steps if "TITLE_PATTERN" in (step.get("env") or {}))
    return check["env"]["TITLE_PATTERN"]


@pytest.mark.parametrize(
    ("title", "valid"),
    [
        ("feat: add x", True),
        ("fix(Vehicle): guard null activeVehicle in telemetry handler", True),
        ("refactor(Comms)!: rework z", True),
        ("deps(deps): update all dependencies", True),
        ("docs: update QGC guide translations (Crowdin)", True),
        ("chore: update PX4 metadata", True),
        ('Revert "feat: add x"', True),
        ("Merge branch 'Stable_V5.0' into master", True),
        ("fixup! feat: add x", True),
        ("New Crowdin updates", False),
        ("Update stuff", False),
        ("feat:nospace", False),
        ("feature: x", False),
        ("Fix: capitalized type", False),
        ("feat(): empty scope", False),
    ],
)
def test_pr_title_check_agrees_with_the_commit_message_hook(title: str, valid: bool) -> None:
    # A squash merge commits the title, so both checks must agree on it.
    assert bool(re.search(_pr_title_pattern(), title)) == valid
    (hook,) = (
        hook
        for repo in load_yaml(".pre-commit-config.yaml")["repos"]
        for hook in repo["hooks"]
        if hook["id"] == "conventional-commit-msg"
    )
    # pygrep --multiline fails the commit when its pattern matches.
    rejected = re.search(hook["entry"], f"{title}\n", re.MULTILINE | re.DOTALL)
    assert (rejected is None) == valid


def test_pr_title_check_limits_the_subject_to_80_characters() -> None:
    pattern = _pr_title_pattern()
    assert re.search(pattern, f"fix(Vehicle): {'x' * 80}")
    assert not re.search(pattern, f"fix(Vehicle): {'x' * 81}")
