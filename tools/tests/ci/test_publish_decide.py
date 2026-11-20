"""Tests for publish_decide.py against GitHub's REST answers for upstream's refs and runs."""

from __future__ import annotations

import json
import subprocess
from email.message import Message
from typing import TYPE_CHECKING, Any
from urllib.error import HTTPError
from urllib.parse import parse_qs, urlsplit

import common.gh_actions
import pytest
from ci.publish import aws_upload, publish_decide
from ci.publish.aws_upload import PublishedStateError
from ci.publish.publish_decide import (
    REQUIRED_WORKFLOWS,
    Ref,
    RewrittenHistoryError,
    builds_of,
    decide,
    main,
    publishing_ref,
    target_verdict,
    trigger_problem,
)

from .._helpers import completed, read_github_file

if TYPE_CHECKING:
    from collections.abc import Callable
    from pathlib import Path

REPO = "mavlink/qgroundcontrol"
S3 = "https://qgroundcontrol.s3.us-west-2.amazonaws.com"
# Four upstream master commits, oldest first, and the state of their push builds on 2026-11-11.
M1 = "77ed40dbfb99ead1b61e85a3f34a6df018946d32"  # Linux and MacOS failed
M2 = "36436db319c731b70ab25dd3178896c4c22c75f6"  # every platform passed
M3 = "13faea0245c22a1d763b96721fe5c1711edc52e2"  # every platform passed on a second attempt
M4 = "4fd4a545d78d43e7c3342861bfe23122037629f8"  # queued
# Stable_V5.1: the v5.1.5 tag's commit and the tip, six commits later, whose Android build failed.
S1 = "3a67d31f0c36bf3fe38ec52970d250a89d0aaf67"
S2 = "8229b895661dbaedc5673ea0569e757db94d3db2"
TAG_OBJECT = "9428722253158ae7600e45a72f2951c8e60b88b1"
# A commit on no branch.
STRAY = "0f0e0d0c0b0a09080706050403020100ffeeddcc"
WORKFLOWS = list(REQUIRED_WORKFLOWS)
# Why a commit gives way to a newer one.
NEWER_PASSED = (
    "is a newer commit of master that passed on every platform, which its own run publishes"
)
# Run ids in WORKFLOWS order (Linux, Windows, MacOS, Android).
RUN_IDS = {
    ("master", M1): (41243981648, 41243981563, 41243981683, 41243981846),
    ("master", M2): (41380314322, 41380314306, 41380314284, 41380314279),
    ("master", M3): (42969994287, 42969994035, 42969993894, 42969993919),
    ("master", M4): (43097397022, 43097396865, 43097396904, 43097396780),
    ("Stable_V5.1", S2): (40772256166, 40772256152, 40772256122, 40772256141),
    ("v5.1.5", S1): (36878132470, 36878129860, 36878129961, 36878130050),
}
PUSHED_AT = {
    M1: "2026-10-31T21:31:25Z",
    M2: "2026-11-02T02:56:00Z",
    M3: "2026-11-11T06:05:31Z",
    M4: "2026-11-11T20:40:02Z",
    S2: "2026-10-28T21:28:39Z",
    S1: "2026-10-01T14:40:06Z",
}


def _run(
    workflow: str,
    ref_name: str,
    commit: str,
    *,
    conclusion: str | None = "success",
    status: str = "completed",
    **fields: Any,
) -> dict[str, Any]:
    """A workflow run as the runs API and the ``workflow_run`` event describe it."""
    run_id = RUN_IDS.get((ref_name, commit), (1, 2, 3, 4))[WORKFLOWS.index(workflow)]
    created = PUSHED_AT.get(commit, "2026-11-11T21:00:00Z")
    return {
        "id": run_id,
        "name": workflow,
        "path": REQUIRED_WORKFLOWS[workflow],
        "event": "push",
        "status": status,
        "conclusion": conclusion,
        "head_branch": ref_name,
        "head_sha": commit,
        "run_number": 10899,
        "run_attempt": 1,
        "created_at": created,
        "run_started_at": created,
        "updated_at": created,
        "html_url": f"https://github.com/{REPO}/actions/runs/{run_id}",
        "repository": {"full_name": REPO},
        "head_repository": {"full_name": REPO, "fork": False},
        **fields,
    }


class Upstream:
    """Stand-in for ``gh api`` and the bucket's public endpoint, holding upstream's state."""

    def __init__(self, monkeypatch: pytest.MonkeyPatch) -> None:
        self.branches = {"master": [M1, M2, M3, M4], "Stable_V5.1": [S1, S2]}
        # Histories that no branch names any more: what a reset branch left behind.
        self.detached: list[list[str]] = []
        self.tags: dict[str, str] = {"v5.1.5": S1}
        self.runs: list[dict[str, Any]] = []
        self.release: dict[str, Any] | None = None
        # Published releases by tag, which is all that a token without write access sees.
        self.published: dict[str, dict[str, Any]] = {"v5.1.5": {"id": 401155735, "draft": False}}
        self.records: dict[str, Any] = {}
        self.requests: list[str] = []
        for commit in (M1, M2, M3, M4):
            self.push("master", commit)
        self.set("master", M1, Linux="failure", MacOS="failure")
        for workflow in ("Linux", "Windows", "MacOS", "Android"):
            self.set("master", M3, run_attempt=2, **{workflow: "success"})
            self.set("master", M4, **{workflow: "queued"})
        self.push("Stable_V5.1", S2)
        self.set("Stable_V5.1", S2, Android="failure")
        self.push("v5.1.5", S1)
        monkeypatch.setattr(common.gh_actions, "gh", self.gh)
        monkeypatch.setattr(aws_upload, "fetch_bytes", self.fetch)

    def push(self, ref_name: str, commit: str, **fields: Any) -> None:
        """Start every platform workflow for a push of *commit* to a branch or tag name."""
        self.runs += [_run(workflow, ref_name, commit, **fields) for workflow in WORKFLOWS]

    def set(self, ref_name: str, commit: str, **changes: Any) -> None:
        """Change the state of a workflow's run (``Linux="failure"``) or a field of all of them."""
        for run in self.runs:
            if (run["head_branch"], run["head_sha"]) != (ref_name, commit):
                continue
            state = changes.get(run["name"])
            if state in ("queued", "in_progress"):
                run.update(status=state, conclusion=None)
            elif state:
                run.update(status="completed", conclusion=state)
            run.update({key: value for key, value in changes.items() if key not in WORKFLOWS})

    def run(self, workflow: str, ref_name: str, commit: str) -> dict[str, Any]:
        """The ``workflow_run`` of the event that the completion of this run sends."""
        (found,) = (
            run
            for run in self.runs
            if (run["name"], run["head_branch"], run["head_sha"]) == (workflow, ref_name, commit)
        )
        return dict(found)

    def publish(self, commit: str, *targets: str, ref_name: str = "master") -> None:
        """Record *commit* as what the targets hold, as the publishing jobs do."""
        for target in targets:
            if target == "daily":
                self.release = {"id": 300, "body": f"<!-- daily-build-commit: {commit} -->"}
            else:
                prefix = "latest" if target == "latest" else f"builds/{ref_name}"
                ref = publishing_ref(ref_name)
                assert ref is not None
                self.records[f"{prefix}/published.json"] = {
                    "schema_version": 1,
                    "ref": ref.full,
                    "commit": commit,
                }

    def _relation(self, base: str, head: str) -> str | None:
        histories = [*self.branches.values(), *self.detached]
        known = {commit for commits in histories for commit in commits} | {STRAY}
        if not {base, head} <= known:
            return None
        if base == head:
            return "identical"
        for commits in histories:
            if base in commits and head in commits:
                return "ahead" if commits.index(head) > commits.index(base) else "behind"
        return "diverged"

    def _answer(self, path: str) -> Any:
        url = urlsplit(path.removeprefix(f"repos/{REPO}/"))
        kind, _, name = url.path.removeprefix("git/ref/").partition("/")
        if url.path.startswith("git/ref/") and kind == "heads" and name in self.branches:
            return {
                "ref": f"refs/heads/{name}",
                "object": {"type": "commit", "sha": self.branches[name][-1]},
            }
        if url.path == "git/ref/tags/v5.1.5" and "v5.1.5" in self.tags:
            # Upstream's release tags are annotated.
            return {"ref": "refs/tags/v5.1.5", "object": {"type": "tag", "sha": TAG_OBJECT}}
        if url.path.startswith("git/ref/") and kind == "tags" and name in self.tags:
            return {
                "ref": f"refs/tags/{name}",
                "object": {"type": "commit", "sha": self.tags[name]},
            }
        if url.path == f"git/tags/{TAG_OBJECT}":
            return {"tag": "v5.1.5", "object": {"type": "commit", "sha": self.tags["v5.1.5"]}}
        if url.path == "git/matching-refs/heads/Stable":
            return [
                {"ref": f"refs/heads/{name}", "object": {"type": "commit", "sha": commits[-1]}}
                for name, commits in self.branches.items()
                if name.startswith("Stable")
            ]
        if url.path.startswith("actions/workflows/") and url.path.endswith("/runs"):
            query = parse_qs(url.query)
            assert query["event"] == ["push"]
            workflow_path = f".github/workflows/{url.path.split('/')[2]}"
            runs = [
                run
                for run in self.runs
                if run["path"] == workflow_path
                and run["head_branch"] == query["branch"][0]
                and run["head_sha"] in query.get("head_sha", [run["head_sha"]])
            ]
            runs.sort(key=lambda run: run["created_at"], reverse=True)
            # One page of the listing, as GitHub answers it.
            size, page = int(query["per_page"][0]), int(query.get("page", ["1"])[0])
            assert 1 <= size <= 100
            start = (page - 1) * size
            return {"total_count": len(runs), "workflow_runs": runs[start : start + size]}
        if url.path == "releases/tags/daily":
            return self.release
        if url.path.startswith("releases/tags/"):
            return self.published.get(url.path.removeprefix("releases/tags/"))
        return None

    def gh(self, *args: str, check: bool = True, **_: object) -> subprocess.CompletedProcess:
        assert args[0] == "api"
        self.requests.append(args[1])
        compare = args[1].removeprefix(f"repos/{REPO}/compare/")
        if compare != args[1]:
            assert args[2:] == ("--jq", ".status")
            answer: Any = self._relation(*compare.split("..."))
            output = f"{answer}\n"
        else:
            answer = self._answer(args[1])
            output = json.dumps(answer)
        if answer is not None:
            return completed(output)
        if check:
            raise subprocess.CalledProcessError(1, list(args), "", "gh: Not Found (HTTP 404)")
        return completed('{"message":"Not Found"}', 1, "gh: Not Found (HTTP 404)")

    def fetch(self, url: str, **_: object) -> bytes:
        self.requests.append(url)
        key = url.removeprefix(f"{S3}/")
        if key not in self.records:
            # What S3 answers for a missing object of a bucket that cannot be listed.
            raise HTTPError(url, 403, "Forbidden", Message(), None)
        return json.dumps(self.records[key]).encode()


@pytest.fixture
def upstream(monkeypatch: pytest.MonkeyPatch) -> Upstream:
    return Upstream(monkeypatch)


def _completes(upstream: Upstream, workflow: str, ref_name: str, commit: str, **kwargs: Any):
    """Decide for the completion of *workflow*'s run, as the workflow_run event starts it."""
    return decide(REPO, run=upstream.run(workflow, ref_name, commit), **kwargs)


def _later_pushes(upstream: Upstream, count: int) -> None:
    """Push *count* more commits to master after M4, a minute apart, all still building."""
    for index in range(count):
        commit = f"{index + 1:040x}"
        upstream.branches["master"].append(commit)
        pushed = f"2026-11-12T{index // 60:02d}:{index % 60:02d}:00Z"
        upstream.push("master", commit, id=50000000000 + index, created_at=pushed)
        upstream.set("master", commit, **dict.fromkeys(WORKFLOWS, "queued"))


@pytest.mark.parametrize(
    ("change", "reason"),
    [
        ({"event": "pull_request"}, "was started by `pull_request`, not by a push"),
        ({"event": "merge_group"}, "was started by `merge_group`, not by a push"),
        ({"event": "workflow_dispatch"}, "was started by `workflow_dispatch`, not by a push"),
        # Stopped, or never run: such a run says nothing of its commit.
        ({"conclusion": "cancelled"}, "the Linux run concluded `cancelled`"),
        ({"conclusion": "timed_out"}, "the Linux run concluded `timed_out`"),
        ({"conclusion": "skipped"}, "the Linux run concluded `skipped`"),
        ({"conclusion": "neutral"}, "the Linux run concluded `neutral`"),
        ({"conclusion": "action_required"}, "the Linux run concluded `action_required`"),
        ({"conclusion": "stale"}, "the Linux run concluded `stale`"),
        ({"conclusion": "startup_failure"}, "the Linux run concluded `startup_failure`"),
        (
            {"head_repository": {"full_name": "someone/qgroundcontrol", "fork": True}},
            "the Linux run built someone/qgroundcontrol, not this repository",
        ),
        # Another workflow file that calls itself Linux.
        ({"path": ".github/workflows/evil.yml"}, "is not a platform workflow that publishes"),
        ({"name": "iOS", "path": ".github/workflows/ios.yml"}, "is not a platform workflow"),
    ],
)
def test_only_a_push_build_of_this_repository_that_was_not_stopped_is_considered(
    upstream: Upstream, change: dict[str, Any], reason: str
) -> None:
    run = {**upstream.run("Linux", "master", M3), **change}

    decision = decide(REPO, run=run)

    assert not decision.publish
    assert reason in decision.reason
    # Decided from the event alone: nothing was asked of the API.
    assert upstream.requests == []


def test_a_fork_pull_request_run_is_ignored() -> None:
    # The event of a fork's pull request: its branch may even be called master.
    run = _run(
        "Linux",
        "master",
        STRAY,
        event="pull_request",
        head_repository={"full_name": "someone/qgroundcontrol", "fork": True},
    )
    assert (
        trigger_problem(run, REPO) == "the Linux run was started by `pull_request`, not by a push"
    )


@pytest.mark.parametrize(
    "ref_name",
    ["feature/x", "dependabot/uv/tools/x", "v5.2.0-dev", "v5.1", "daily", "latest", "Stable/x", ""],
)
def test_other_refs_are_never_published(upstream: Upstream, ref_name: str) -> None:
    run = {**upstream.run("Linux", "master", M3), "head_branch": ref_name}

    decision = decide(REPO, run=run)

    assert not decision.publish
    assert "is not master, a Stable* branch or a vX.Y.Z release tag" in decision.reason
    assert upstream.requests == []


def test_names_decide_only_what_a_ref_could_be() -> None:
    assert publishing_ref("master") == Ref("branch", "master")
    assert publishing_ref("Stable_V5.1") == Ref("branch", "Stable_V5.1")
    assert publishing_ref("v5.1.5") == Ref("tag", "v5.1.5")
    assert Ref("branch", "Stable_V5.1").full == "refs/heads/Stable_V5.1"
    assert Ref("tag", "v5.1.5").full == "refs/tags/v5.1.5"
    # Unsafe in a path, although git allows them in a branch name.
    for name in ("Stable$(id)", "Stable V5", "Stable;x", "master/x", "Stable\n"):
        assert publishing_ref(name) is None


def test_the_run_that_completes_the_set_publishes_master_everywhere(upstream: Upstream) -> None:
    upstream.branches["master"].remove(M4)
    upstream.runs = [run for run in upstream.runs if run["head_sha"] != M4]

    decision = _completes(upstream, "Android", "master", M3)

    assert decision.publish
    assert (decision.ref, decision.commit) == (Ref("branch", "master"), M3)
    assert decision.selected == ("daily", "s3")
    assert [run["id"] for run in decision.runs] == list(RUN_IDS["master", M3])
    assert decision.targets == {
        "daily": "nothing is published yet",
        "s3": "nothing is published yet",
    }


def test_a_newer_push_still_building_does_not_hold_back_a_complete_commit(
    upstream: Upstream,
) -> None:
    # master's tip is M4, whose builds are queued.
    assert _completes(upstream, "Android", "master", M3).selected == ("daily", "s3")


@pytest.mark.parametrize(
    ("states", "waiting"),
    [
        ({"MacOS": "in_progress"}, "waiting for MacOS (in_progress)"),
        ({"Windows": "queued", "MacOS": "in_progress"}, "Windows (queued), MacOS (in_progress)"),
        # A run that was stopped did not complete its build, whoever stopped it.
        ({"Android": "cancelled"}, "waiting for Android (cancelled)"),
        ({"Android": "timed_out"}, "waiting for Android (timed_out)"),
        ({"Windows": "skipped"}, "waiting for Windows (skipped)"),
        ({"Linux": "action_required"}, "waiting for Linux (action_required)"),
        ({"Linux": "startup_failure"}, "waiting for Linux (startup_failure)"),
        (
            {"Linux": "failure", "MacOS": "cancelled"},
            "waiting for MacOS (cancelled)",
        ),
    ],
)
def test_nothing_is_published_until_every_platform_s_run_has_completed(
    upstream: Upstream, states: dict[str, str], waiting: str
) -> None:
    upstream.set("master", M3, **states)
    finished = next(workflow for workflow in WORKFLOWS if workflow not in states)

    decision = _completes(upstream, finished, "master", M3)

    assert not decision.publish
    assert waiting in decision.reason
    assert decision.selected == ()


def test_a_workflow_that_never_ran_for_the_commit_leaves_it_incomplete(upstream: Upstream) -> None:
    # A waiting build run gives way to a newer push, so a commit can lack a workflow's run.
    upstream.runs = [
        run for run in upstream.runs if (run["name"], run["head_sha"]) != ("MacOS", M3)
    ]

    decision = _completes(upstream, "Linux", "master", M3)

    assert "waiting for MacOS (no run)" in decision.reason


def test_real_upstream_commit_with_two_failed_platforms_gives_way_to_one_that_passed(
    upstream: Upstream,
) -> None:
    decision = _completes(upstream, "Windows", "master", M1)

    assert not decision.publish
    assert decision.reason == f"{M3} {NEWER_PASSED}"


def test_a_branch_commit_is_published_although_a_workflow_failed(
    upstream: Upstream,
    actions: Callable[..., None],
    workflow_run_event: Callable[..., Path],
    gh_output: Path,
    gh_summary: Path,
) -> None:
    # The emulator boot test failed after the APK was built: the Android run concluded
    # `failure`, and its completion is the event that completes the commit.
    upstream.set("master", M3, Android="failure")
    not_all_passed = (
        "every platform's run of this commit has completed, but not all passed (Android "
        "(failure)): it is published if a job that passed built every package"
    )

    decision = _completes(upstream, "Android", "master", M3)

    assert (decision.publish, decision.selected) == (True, ("daily", "s3"))
    assert decision.reason == not_all_passed
    assert [run["conclusion"] for run in decision.runs] == ["success"] * 3 + ["failure"]
    # Whichever run completes last starts the publish.
    assert _completes(upstream, "Linux", "master", M3).reason == not_all_passed

    # Verification is told how each run concluded, and holds the run to that.
    actions(PUBLISH_DAILY_RELEASE="true", PUBLISH_S3="true")
    workflow_run_event(**upstream.run("Android", "master", M3))
    assert main(["decide"]) == 0
    listed = json.loads(read_github_file(gh_output)["runs"])
    assert [(run["workflow"], run["conclusion"]) for run in listed] == [
        ("Linux", "success"),
        ("Windows", "success"),
        ("MacOS", "success"),
        ("Android", "failure"),
    ]
    summary = gh_summary.read_text(encoding="utf-8")
    assert f"[Android](https://github.com/{REPO}/actions/runs/42969993919) (`failure`)" in summary
    assert f"[Linux](https://github.com/{REPO}/actions/runs/42969994287)," in summary


def test_a_release_tag_waits_for_every_workflow_to_pass(upstream: Upstream) -> None:
    upstream.set("v5.1.5", S1, Android="failure")

    assert _completes(upstream, "Linux", "v5.1.5", S1).reason == "waiting for Android (failure)"
    assert decide(REPO, ref_name="v5.1.5").reason == "waiting for Android (failure)"
    # The failed run's own completion is turned away before anything is asked of the API.
    upstream.requests.clear()
    decision = _completes(upstream, "Android", "v5.1.5", S1)
    assert not decision.publish
    assert decision.reason == (
        "the Android run concluded `failure`, and a release tag is published only when every "
        "workflow passed"
    )
    assert upstream.requests == []


def test_a_failed_job_that_is_re_run_and_passes_does_not_publish_the_commit_again(
    upstream: Upstream,
) -> None:
    # M3 was published while its Android run stood at `failure`. The failed job is re-run
    # and passes: the targets hold the commit, and their records keep what was true then.
    upstream.set("master", M3, Android="failure")
    assert _completes(upstream, "Android", "master", M3).publish
    upstream.publish(M3, "daily", "s3")
    upstream.set("master", M3, Android="success", run_attempt=3)

    decision = _completes(upstream, "Android", "master", M3)

    assert not decision.publish
    assert decision.reason == "every target already holds this commit or a newer one"
    # A request by hand publishes it again, with the conclusions as they are now.
    again = decide(REPO, ref_name="master")
    assert again.selected == ("daily", "s3")
    assert again.reason == "every platform built this commit"


def test_a_commit_does_not_give_way_to_a_newer_one_whose_packages_may_be_missing(
    upstream: Upstream,
) -> None:
    # M3's Linux run failed: whether in a test or in the job that packages is not known here.
    # M2's failed Linux job is re-run and completes after it.
    upstream.set("master", M3, Linux="failure")
    upstream.set("master", M2, Linux="success", run_attempt=2)

    # Nothing is published yet: M3 lacked a package, so M2 is the newest that can be.
    decision = _completes(upstream, "Linux", "master", M2)
    assert (decision.publish, decision.selected) == (True, ("daily", "s3"))

    # M3 was published after all: the targets keep the older commit out.
    upstream.publish(M3, "daily", "s3")
    decision = _completes(upstream, "Linux", "master", M2)
    assert not decision.publish
    assert decision.targets == {
        "daily": f"holds the newer commit {M3}",
        "s3": f"holds the newer commit {M3}",
    }


def test_a_request_by_hand_takes_the_newest_commit_whose_builds_have_finished(
    upstream: Upstream,
) -> None:
    upstream.set("master", M3, MacOS="failure")
    assert decide(REPO, ref_name="master").commit == M3

    # A run that was cancelled leaves its commit unfinished: the one before it is taken.
    upstream.set("master", M3, MacOS="cancelled")
    assert decide(REPO, ref_name="master").commit == M2


def test_a_rerun_counts_by_its_latest_attempt(upstream: Upstream) -> None:
    # Run 42969994287: attempt 1 failed, then every job was re-run. A re-run keeps the run's id.
    upstream.set("master", M3, Linux="failure", run_attempt=1)
    first = _completes(upstream, "Android", "master", M3)
    assert (first.publish, first.runs[0]["run_attempt"], first.runs[0]["conclusion"]) == (
        True,
        1,
        "failure",
    )

    upstream.set("master", M3, Linux="in_progress", run_attempt=2)
    assert "waiting for Linux (in_progress)" in _completes(upstream, "Android", "master", M3).reason

    upstream.set("master", M3, Linux="success", run_attempt=2)
    decision = _completes(upstream, "Linux", "master", M3)
    assert decision.publish
    assert decision.runs[0]["run_attempt"] == 2


def test_the_event_s_run_counts_even_before_the_runs_api_shows_it_completed(
    upstream: Upstream,
) -> None:
    event = upstream.run("Android", "master", M3)
    upstream.set("master", M3, Android="in_progress")

    assert decide(REPO, run=event).publish


@pytest.mark.parametrize("state", ["in_progress", "cancelled", "failure"])
def test_an_event_of_an_earlier_attempt_does_not_stand_for_the_run_s_latest_attempt(
    upstream: Upstream, state: str
) -> None:
    # Android's attempt 2 passed and sent the event; attempt 3 was started before it was read.
    event = upstream.run("Android", "master", M3)
    upstream.set("master", M3, Android=state)
    rerun = next(r for r in upstream.runs if (r["name"], r["head_sha"]) == ("Android", M3))
    rerun["run_attempt"] = 3

    decision = decide(REPO, run=event)

    if state == "failure":
        # The commit is approved as attempt 3 left it, not as the event's attempt did.
        android = decision.runs[3]
        assert (android["run_attempt"], android["conclusion"]) == (3, "failure")
    else:
        assert not decision.publish
        assert decision.reason == f"waiting for Android ({state})"


def test_a_commit_is_found_however_many_pushes_followed_it(upstream: Upstream) -> None:
    # A page of newer pushes no longer hides the other platforms' runs for the commit.
    _later_pushes(upstream, 31)

    decision = _completes(upstream, "Android", "master", M3)

    assert decision.publish
    assert [run["id"] for run in decision.runs] == list(RUN_IDS["master", M3])
    # Each workflow was asked for the commit's runs, not for a listing that happens to reach it.
    assert sum(f"&head_sha={M3}&page=1" in request for request in upstream.requests) == 4


def test_a_manual_request_reads_further_pages_until_it_finds_a_complete_commit(
    upstream: Upstream, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(publish_decide, "PAGE_SIZE", 10)
    _later_pushes(upstream, 31)
    # MacOS has a single page to list: its waiting runs gave way to the pushes that followed.
    upstream.runs = [
        run
        for run in upstream.runs
        if run["name"] != "MacOS" or run["head_sha"] in (M1, M2, M3, M4)
    ]

    decision = decide(REPO, ref_name="master")

    # 32 pushes are still building; M3 is the 33rd, on the fourth page of ten.
    assert (decision.publish, decision.commit) == (True, M3)
    listings = [r for r in upstream.requests if "/runs?" in r and "head_sha" not in r]
    assert {r.rsplit("&page=", 1)[1] for r in listings} == {"1", "2", "3", "4"}


def test_a_newer_complete_commit_is_found_beyond_the_first_page(
    upstream: Upstream, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(publish_decide, "PAGE_SIZE", 10)
    _later_pushes(upstream, 31)
    upstream.set("master", M2, Linux="success", run_attempt=2)

    decision = _completes(upstream, "Linux", "master", M2)

    assert decision.reason == f"{M3} {NEWER_PASSED}"


def test_a_branch_reset_away_from_a_complete_commit_publishes_the_one_it_still_contains(
    upstream: Upstream,
) -> None:
    # Stable_V5.2 was A-B-C, with B and C built on every platform, and was reset to A-B-D.
    a, b, c, d = ("a" * 40, "b" * 40, "c" * 40, "d" * 40)
    upstream.branches["Stable_V5.2"] = [a, b, d]
    upstream.detached.append([a, b, c])
    for index, commit in enumerate((b, c, d)):
        upstream.push(
            "Stable_V5.2", commit, id=60000000000 + index, created_at=f"2026-11-1{index}T00:00:00Z"
        )
    upstream.set("Stable_V5.2", d, Android="in_progress")

    # By hand: C is the newest commit that finished, but only B can still be published.
    decision = decide(REPO, ref_name="Stable_V5.2")
    assert (decision.publish, decision.commit, decision.selected) == (True, b, ("s3",))

    # C, which no run can publish any more, does not stand in the way of B's own run either.
    assert _completes(upstream, "Linux", "Stable_V5.2", b).publish
    assert _completes(upstream, "Linux", "Stable_V5.2", c).reason == (
        f"{c} is not on branch Stable_V5.2, whose tip is {d}"
    )


def test_a_stable_branch_publishes_to_its_builds_directory_only(upstream: Upstream) -> None:
    upstream.set("Stable_V5.1", S2, Android="success")

    decision = _completes(upstream, "Android", "Stable_V5.1", S2)

    assert decision.publish
    assert (decision.ref, decision.commit) == (Ref("branch", "Stable_V5.1"), S2)
    assert decision.selected == ("s3",)
    assert f"{S3}/builds/Stable_V5.1/published.json" in upstream.requests
    assert f"repos/{REPO}/releases/tags/daily" not in upstream.requests


def test_real_stable_tip_whose_android_run_failed_is_left_to_verification(
    upstream: Upstream,
) -> None:
    # Run 40772256141 uploaded both APKs and concluded `failure`: whether a job that passed
    # built every package is found out after download.
    for workflow in ("Linux", "Android"):
        decision = _completes(upstream, workflow, "Stable_V5.1", S2)

        assert (decision.publish, decision.selected) == (True, ("s3",))
        assert "but not all passed (Android (failure))" in decision.reason
        assert decision.runs[3]["conclusion"] == "failure"


def test_a_release_tag_on_a_stable_branch_publishes_builds_and_latest(upstream: Upstream) -> None:
    decision = _completes(upstream, "MacOS", "v5.1.5", S1)

    assert decision.publish
    assert (decision.ref, decision.commit) == (Ref("tag", "v5.1.5"), S1)
    assert decision.selected == ("s3", "latest", "release")
    assert [run["id"] for run in decision.runs] == list(RUN_IDS["v5.1.5", S1])
    # The annotated tag was peeled, and its commit found on Stable_V5.1.
    assert f"repos/{REPO}/git/tags/{TAG_OBJECT}" in upstream.requests
    assert f"repos/{REPO}/compare/{S1}...{S2}" in upstream.requests
    assert f"{S3}/builds/v5.1.5/published.json" in upstream.requests
    assert f"{S3}/latest/published.json" in upstream.requests


def test_a_repushed_tag_counts_by_its_newest_runs(upstream: Upstream) -> None:
    # Pushing v5.1.5 again starts a second run of every workflow for the same commit.
    again = [
        _run(workflow, "v5.1.5", S1, id=37000000000 + index, created_at="2026-10-02T09:00:00Z")
        for index, workflow in enumerate(WORKFLOWS)
    ]
    again[2].update(status="in_progress", conclusion=None)
    upstream.runs += again

    decision = decide(REPO, run=again[0])

    assert decision.reason == "waiting for MacOS (in_progress)"

    again[2].update(status="completed", conclusion="success")
    assert [run["id"] for run in decide(REPO, run=again[0]).runs] == [r["id"] for r in again]


def test_a_commit_built_for_a_tag_and_for_its_branch_keeps_the_two_apart() -> None:
    # A tag pushed on a branch's tip: both pushes build the same commit.
    listed = {
        workflow: [
            _run(workflow, "v5.1.6", S2, id=1),
            _run(workflow, "Stable_V5.1", S2, id=2, conclusion="failure"),
            _run(workflow, "Stable_V5.1", S2, id=3, event="workflow_dispatch"),
            _run(workflow, "v5.1.6", S2, id=4, path=".github/workflows/evil.yml"),
            _run(workflow, "v5.1.6", S2, id=5, head_repository={"full_name": "someone/qgc"}),
        ]
        for workflow in WORKFLOWS
    }

    (tag_build,) = builds_of(listed, Ref("tag", "v5.1.6"), REPO)
    (branch_build,) = builds_of(listed, Ref("branch", "Stable_V5.1"), REPO)

    assert tag_build.passed
    assert {run["id"] for run in tag_build.runs.values()} == {1}
    assert not branch_build.passed
    assert not branch_build.waiting_for(("success", "failure"))
    assert {run["id"] for run in branch_build.runs.values()} == {2}

    # Whatever order the runs are listed in, the most recent push of the tag counts.
    for runs in listed.values():
        runs.insert(1, _run(runs[0]["name"], "v5.1.6", S2, id=6, created_at="2026-11-12T00:00:00Z"))
    (tag_build,) = builds_of(listed, Ref("tag", "v5.1.6"), REPO)
    assert {run["id"] for run in tag_build.runs.values()} == {6}


def test_a_branch_named_like_a_release_tag_is_not_a_release(upstream: Upstream) -> None:
    # A branch v5.1.6 was pushed (its workflow files can trigger on any branch); no such tag.
    upstream.branches["v5.1.6"] = [STRAY]
    upstream.push("v5.1.6", STRAY)

    decision = _completes(upstream, "Linux", "v5.1.6", STRAY)

    assert not decision.publish
    assert decision.reason == "refs/tags/v5.1.6 does not exist"


def test_a_branch_carrying_an_existing_tag_s_name_is_not_that_tag(upstream: Upstream) -> None:
    upstream.branches["v5.1.5"] = [STRAY]
    upstream.push("v5.1.5", STRAY)

    decision = _completes(upstream, "Linux", "v5.1.5", STRAY)

    assert not decision.publish
    assert decision.reason == f"{STRAY} is not what tag v5.1.5 names ({S1}): not a tag build"


def test_a_release_tag_off_every_stable_branch_is_not_published(upstream: Upstream) -> None:
    upstream.tags["v5.2.0"] = M2
    upstream.push("v5.2.0", M2)

    decision = _completes(upstream, "Linux", "v5.2.0", M2)

    assert not decision.publish
    assert decision.reason == "tag v5.2.0 is not on a Stable* branch"


def test_a_tag_carrying_a_branch_s_name_cannot_publish_as_the_branch(upstream: Upstream) -> None:
    # A tag named master on a commit of no branch, built by workflow files that start on it.
    upstream.tags["master"] = STRAY
    upstream.push("master", STRAY)

    decision = _completes(upstream, "Linux", "master", STRAY)

    assert not decision.publish
    assert decision.reason == f"{STRAY} is not on branch master, whose tip is {M4}"


def test_an_older_commit_completing_after_a_newer_one_was_published_is_skipped(
    upstream: Upstream,
) -> None:
    # M3 is published; then M2's failed Linux run is re-run and passes.
    upstream.publish(M3, "daily", "s3")
    upstream.set("master", M2, Linux="success", run_attempt=2)

    decision = _completes(upstream, "Linux", "master", M2)

    assert not decision.publish
    assert decision.reason == f"{M3} {NEWER_PASSED}"


def test_an_older_commit_never_replaces_a_newer_published_one(upstream: Upstream) -> None:
    # As above, but M3's runs have left the listing: only the targets remember it.
    upstream.publish(M3, "daily", "s3")
    upstream.runs = [run for run in upstream.runs if run["head_sha"] != M3]

    decision = _completes(upstream, "Linux", "master", M2)

    assert not decision.publish
    assert decision.selected == ()
    assert decision.targets == {
        "daily": f"holds the newer commit {M3}",
        "s3": f"holds the newer commit {M3}",
    }


def test_a_newer_complete_commit_takes_over_while_the_older_one_is_unpublished(
    upstream: Upstream,
) -> None:
    # Nothing is published yet; the late run of M2 must not queue ahead of M3's publish.
    assert not _completes(upstream, "Linux", "master", M2).publish
    assert _completes(upstream, "Linux", "master", M3).publish


def test_a_duplicate_trigger_finds_the_commit_published_and_does_nothing(
    upstream: Upstream,
) -> None:
    upstream.publish(M3, "daily", "s3")

    decision = _completes(upstream, "Linux", "master", M3)

    assert not decision.publish
    assert decision.reason == "every target already holds this commit or a newer one"
    assert decision.targets == {
        "daily": "already holds this commit",
        "s3": "already holds this commit",
    }


def test_a_target_that_is_behind_is_published_without_touching_the_current_one(
    upstream: Upstream,
) -> None:
    # The S3 job of M3's publish failed; the daily release succeeded.
    upstream.publish(M3, "daily")
    upstream.publish(M2, "s3")

    decision = _completes(upstream, "Linux", "master", M3)

    assert decision.publish
    assert decision.selected == ("s3",)
    assert decision.targets == {
        "daily": "already holds this commit",
        "s3": f"moves forward from {M2}",
    }


def test_a_release_tag_is_not_published_twice(upstream: Upstream) -> None:
    upstream.publish(S1, "s3", "latest", ref_name="v5.1.5")

    # Only the job of the GitHub Release starts, which checks that the release is complete.
    decision = _completes(upstream, "Linux", "v5.1.5", S1)
    assert decision.selected == ("release",)
    assert decision.targets == {
        "s3": "already holds this commit",
        "latest": "already holds this release",
        "release": "is already published: its job checks that it has these files, and changes none without force_release",
    }
    assert decide(REPO, ref_name="v5.1.5").selected == ("s3", "latest", "release")

    # latest/ held the release before it; builds/v5.1.5/ is complete.
    upstream.publish(M2, "latest", ref_name="v5.1.4")
    decision = _completes(upstream, "Linux", "v5.1.5", S1)
    assert decision.selected == ("latest", "release")
    assert decision.targets["latest"] == "moves forward from v5.1.4"


@pytest.mark.parametrize("newer", ["v5.1.6", "v5.2.0", "v5.10.0", "v6.0.0"])
def test_an_older_release_never_takes_latest_back_but_still_gets_its_builds_directory(
    upstream: Upstream, newer: str
) -> None:
    # v5.1.5 completes after a newer release was promoted: a late run, a re-run, or a patch
    # release on an older Stable line.
    upstream.publish(S2, "latest", ref_name=newer)

    decision = _completes(upstream, "Linux", "v5.1.5", S1)

    assert decision.selected == ("s3", "release")
    assert decision.targets == {
        "s3": "nothing is published yet",
        "latest": f"holds the newer release {newer}",
        "release": "is already published: its job checks that it has these files, and changes none without force_release",
    }


def test_latest_without_a_public_record_is_left_to_the_job_that_reads_the_bucket(
    upstream: Upstream,
) -> None:
    # latest/ holds whatever the build jobs uploaded for themselves, and no record. From here a
    # missing record and a private one look the same, so the decision neither skips latest/ nor
    # takes it for empty: its job starts, reads the record again and stops without one.
    left_to_the_job = (
        "has no public record of what it holds: its job reads the record again, and "
        "without one stops until the record is seeded or the release is published by hand with "
        "force_latest"
    )

    decision = _completes(upstream, "MacOS", "v5.1.5", S1)
    assert decision.selected == ("s3", "latest", "release")
    assert decision.targets["latest"] == left_to_the_job
    assert decide(REPO, ref_name="v5.1.5").targets["latest"] == left_to_the_job
    # A completed build cannot force it.
    forced = _completes(upstream, "MacOS", "v5.1.5", S1, force=("latest",))
    assert forced.targets["latest"] == left_to_the_job

    decision = decide(REPO, ref_name="v5.1.5", force=("latest",))
    assert decision.selected == ("s3", "latest", "release")
    assert decision.targets["latest"] == (
        "has no public record of what it holds, and is replaced as requested"
    )


def test_a_dry_run_reports_what_happens_to_a_latest_without_a_record(
    upstream: Upstream, actions: Callable[..., None], gh_output: Path, gh_summary: Path
) -> None:
    actions(PUBLISH_DRY_RUN="true")

    assert main(["decide", "--ref", "v5.1.5"]) == 0

    outputs = read_github_file(gh_output)
    assert (outputs["s3"], outputs["latest"], outputs["dry-run"]) == ("true", "true", "true")
    assert (
        "| Target `latest` | has no public record of what it holds: its job reads the record "
        "again, and without one stops until the record is seeded or the release is published "
        "by hand with force_latest |"
    ) in gh_summary.read_text(encoding="utf-8")


def test_latest_goes_back_to_an_older_release_only_by_hand_with_force_latest(
    upstream: Upstream,
) -> None:
    upstream.publish(S2, "latest", ref_name="v5.1.6")

    assert decide(REPO, ref_name="v5.1.5").selected == ("s3", "release")
    # A completed build has no say in it, whatever it is given.
    forced = _completes(upstream, "Linux", "v5.1.5", S1, force=("latest",))
    assert forced.selected == ("s3", "release")

    decision = decide(REPO, ref_name="v5.1.5", force=("latest",))
    assert decision.selected == ("s3", "latest", "release")
    assert decision.targets["latest"] == "goes back from v5.1.6, as requested"


def test_two_release_tags_on_one_commit_are_two_releases(upstream: Upstream) -> None:
    # v5.1.6 is tagged on the commit that v5.1.5 already released.
    upstream.publish(S1, "s3", "latest", ref_name="v5.1.5")
    upstream.tags["v5.1.6"] = S1
    upstream.push("v5.1.6", S1, id=37100000000, created_at="2026-10-03T09:00:00Z")

    decision = _completes(upstream, "Linux", "v5.1.6", S1)

    assert decision.selected == ("s3", "latest", "release")
    assert decision.targets == {
        "s3": "nothing is published yet",
        "latest": "moves forward from v5.1.5",
        "release": "has no published release: the packages go to the tag's draft",
    }
    # And the older tag of the two does not take latest/ back.
    upstream.publish(S1, "latest", ref_name="v5.1.6")
    assert decide(REPO, ref_name="v5.1.5").targets["latest"] == "holds the newer release v5.1.6"


def test_a_release_tag_moved_to_another_commit_replaces_what_latest_holds_of_it(
    upstream: Upstream,
) -> None:
    # v5.1.5 was published from S1; then the tag was moved forward to S2 and built again.
    upstream.publish(S1, "s3", "latest", ref_name="v5.1.5")
    del upstream.published["v5.1.5"]
    upstream.tags["v5.1.5"] = S2
    upstream.push("v5.1.5", S2, id=37200000000, created_at="2026-10-29T09:00:00Z")

    decision = _completes(upstream, "Linux", "v5.1.5", S2)

    # builds/v5.1.5/, the draft and latest/ all take the commit that the tag names now.
    assert decision.selected == ("s3", "latest", "release")
    assert decision.targets["s3"] == f"replaces {S1}"
    assert decision.targets["latest"] == f"replaces {S1} of the same release"

    # The same tag at the same commit is not published twice.
    upstream.publish(S2, "s3", "latest", ref_name="v5.1.5")
    assert _completes(upstream, "Linux", "v5.1.5", S2).targets["latest"] == (
        "already holds this release"
    )
    assert decide(REPO, ref_name="v5.1.5").targets["latest"] == (
        "holds this release, which is published again"
    )


def test_a_record_of_another_repository_is_reported_and_not_compared(
    upstream: Upstream, actions: Callable[..., None], gh_output: Path, gh_summary: Path
) -> None:
    # A fork's dry run reads upstream's bucket: the commits recorded there are not the fork's,
    # so its branches are not "rewritten" and its releases are not "older".
    foreign = {"schema_version": 1, "repository": "someone/qgroundcontrol"}
    upstream.records["builds/master/published.json"] = {
        **foreign,
        "ref": "refs/heads/master",
        "commit": "f" * 40,
    }
    upstream.records["latest/published.json"] = {
        **foreign,
        "ref": "refs/tags/v9.0.0",
        "commit": "e" * 40,
    }
    reason = (
        "holds a build of someone/qgroundcontrol, not of this repository: its commits are not "
        "compared, and the job that uploads refuses to replace it"
    )

    assert decide(REPO, ref_name="master").targets["s3"] == reason
    assert decide(REPO, ref_name="v5.1.5").targets["latest"] == reason
    # No comparison with the foreign commits was asked for.
    assert not any("f" * 40 in request or "e" * 40 in request for request in upstream.requests)

    actions(PUBLISH_DRY_RUN="true")
    assert main(["decide", "--ref", "master"]) == 0
    assert read_github_file(gh_output)["s3"] == "true"
    assert f"| Target `s3` | {reason} |" in gh_summary.read_text(encoding="utf-8")


def test_a_record_that_names_no_release_is_replaced_only_by_hand_with_force_latest(
    upstream: Upstream,
) -> None:
    # A record without the tag it was published from: not one this workflow writes.
    upstream.records["latest/published.json"] = {"schema_version": 1, "commit": M2}

    with pytest.raises(PublishedStateError, match="does not name the release"):
        _completes(upstream, "Linux", "v5.1.5", S1)
    with pytest.raises(PublishedStateError, match="by hand with force_latest to replace it"):
        decide(REPO, ref_name="v5.1.5")

    decision = decide(REPO, ref_name="v5.1.5", force=("latest",))
    assert decision.selected == ("s3", "latest", "release")
    assert decision.targets["latest"] == f"replaces {M2}, whose release is not recorded"


def test_a_rewritten_branch_is_refused_until_someone_clears_the_record(upstream: Upstream) -> None:
    # Stable_V5.1 was force-pushed: the published commit is no longer in its history.
    upstream.set("Stable_V5.1", S2, Android="success")
    upstream.publish(STRAY, "s3", ref_name="Stable_V5.1")

    with pytest.raises(RewrittenHistoryError, match=r"refs/heads/Stable_V5\.1 was rewritten"):
        _completes(upstream, "Linux", "Stable_V5.1", S2)


def test_a_manual_request_publishes_the_newest_complete_commit_of_a_branch(
    upstream: Upstream,
) -> None:
    # master's tip M4 is still building.
    decision = decide(REPO, ref_name="master")

    assert decision.publish
    assert decision.commit == M3
    assert decision.selected == ("daily", "s3")


def test_a_manual_request_publishes_again_what_the_targets_hold_but_never_goes_back(
    upstream: Upstream,
) -> None:
    # After a platform was rebuilt for the same commit, a request by hand republishes the set.
    upstream.publish(M3, "daily", "s3")

    decision = decide(REPO, ref_name="master")

    assert decision.selected == ("daily", "s3")
    assert decision.targets["s3"] == "holds this commit, which is published again"

    # The newest complete commit is M3, but S3 already holds a later one.
    upstream.publish(M4, "s3")
    decision = decide(REPO, ref_name="master")
    assert decision.selected == ("daily",)
    assert decision.targets["s3"] == f"holds the newer commit {M4}"


def test_a_manual_request_can_name_an_older_commit_than_the_newest_finished_one(
    upstream: Upstream,
) -> None:
    # M3 finished, but the job that packages its Linux build failed, so it lacks a package and
    # is what a request for the branch alone decides for. M2 is the last commit that can publish.
    upstream.set("master", M3, Linux="failure")
    assert decide(REPO, ref_name="master").commit == M3

    decision = decide(REPO, ref_name="master", commit=M2)

    assert (decision.publish, decision.commit) == (True, M2)
    assert decision.selected == ("daily", "s3")
    assert [run["head_sha"] for run in decision.runs] == [M2] * 4

    # The rules for a commit are those of any other: it gives way to a newer one that passed.
    upstream.set("master", M3, Linux="success")
    assert decide(REPO, ref_name="master", commit=M2).reason == f"{M3} {NEWER_PASSED}"
    # And every platform must have finished building it.
    assert decide(REPO, ref_name="master", commit=M4).reason.startswith(
        "waiting for Linux (queued)"
    )


def test_a_commit_named_by_hand_never_takes_a_target_back(upstream: Upstream) -> None:
    # M3 was published although its Linux run failed in a job that builds no package.
    upstream.set("master", M3, Linux="failure")
    upstream.publish(M3, "daily", "s3")

    decision = decide(REPO, ref_name="master", commit=M2)

    assert not decision.publish
    assert decision.reason == "every target already holds this commit or a newer one"
    assert decision.targets == {
        "daily": f"holds the newer commit {M3}",
        "s3": f"holds the newer commit {M3}",
    }

    # One target is behind: the commit goes there, and only there.
    upstream.publish(M1, "s3")
    decision = decide(REPO, ref_name="master", commit=M2)
    assert decision.selected == ("s3",)
    assert decision.targets["s3"] == f"moves forward from {M1}"


@pytest.mark.parametrize(
    "commit",
    [M2[:10], M2.upper(), f"{M2}0", "master", "HEAD~1", f"{M2[:39]}g", "$(id)"],
)
def test_a_commit_that_is_not_a_full_sha_is_refused_before_any_request(
    upstream: Upstream, commit: str
) -> None:
    decision = decide(REPO, ref_name="master", commit=commit)

    assert not decision.publish
    assert decision.reason == (
        f"`{commit}` is not a full commit SHA: the commit input takes all 40 digits, in lower "
        "case, of a commit of the ref"
    )
    assert upstream.requests == []


def test_a_release_tag_is_published_only_at_the_commit_it_names(upstream: Upstream) -> None:
    assert decide(REPO, ref_name="v5.1.5", commit=S1).publish

    decision = decide(REPO, ref_name="v5.1.5", commit=S2)

    assert not decision.publish
    assert decision.reason == f"{S2} is not what tag v5.1.5 names ({S1}): not a tag build"


def test_a_manual_request_for_a_branch_without_a_finished_commit_publishes_nothing(
    upstream: Upstream,
) -> None:
    upstream.set("Stable_V5.1", S2, Android="cancelled")

    decision = decide(REPO, ref_name="Stable_V5.1")

    assert not decision.publish
    assert decision.reason == "no commit on Stable_V5.1 has finished building on every platform"


def test_a_manual_request_republishes_a_release_tag(upstream: Upstream) -> None:
    decision = decide(REPO, ref_name="v5.1.5")

    assert (decision.publish, decision.commit) == (True, S1)
    assert decision.selected == ("s3", "latest", "release")
    assert not decide(REPO, ref_name="v5.2.0-dev").publish
    assert decide(REPO, ref_name="v9.9.9").reason == "refs/tags/v9.9.9 does not exist"


def test_a_repository_without_targets_publishes_nothing(upstream: Upstream) -> None:
    # A fork: no S3, and no opt-in to its own daily release.
    decision = _completes(upstream, "Android", "master", M3, enabled=())

    assert not decision.publish
    assert decision.reason == "this repository publishes nothing for refs/heads/master"
    assert upstream.requests == []

    # A fork that opted in to the daily release has no S3 target.
    assert _completes(upstream, "Android", "master", M3, enabled=("daily",)).selected == ("daily",)
    assert not _completes(upstream, "MacOS", "v5.1.5", S1, enabled=("daily",)).publish


@pytest.mark.parametrize(
    ("ref", "published", "commit", "verdict"),
    [
        ("master", None, M3, (True, "nothing is published yet")),
        ("master", M3, M3, (False, "already holds this commit")),
        ("master", M2, M3, (True, f"moves forward from {M2}")),
        ("master", M3, M2, (False, f"holds the newer commit {M3}")),
        ("v5.1.5", M2, S1, (True, f"replaces {M2}")),
        ("v5.1.5", S1, S1, (False, "already holds this commit")),
    ],
)
def test_a_branch_target_only_moves_forward(
    upstream: Upstream, ref: str, published: str | None, commit: str, verdict: tuple[bool, str]
) -> None:
    kind = publishing_ref(ref)
    assert kind is not None
    assert target_verdict(kind, published, commit, upstream._relation) == verdict


@pytest.mark.parametrize("published", [S1, "f" * 40])
def test_a_branch_target_holding_an_unrelated_commit_is_refused(
    upstream: Upstream, published: str
) -> None:
    with pytest.raises(RewrittenHistoryError):
        target_verdict(Ref("branch", "master"), published, M3, upstream._relation)


@pytest.fixture
def actions(monkeypatch: pytest.MonkeyPatch) -> Callable[..., None]:
    def configure(**variables: str) -> None:
        monkeypatch.setenv("GITHUB_REPOSITORY", REPO)
        for name, value in variables.items():
            monkeypatch.setenv(name, value)

    return configure


def test_decide_outputs_what_the_later_jobs_read(
    upstream: Upstream,
    actions: Callable[..., None],
    workflow_run_event: Callable[..., Path],
    gh_output: Path,
    gh_summary: Path,
) -> None:
    actions(
        PUBLISH_DAILY_RELEASE="true", PUBLISH_S3="true", PUBLISH_REF="", PUBLISH_DRY_RUN="false"
    )
    workflow_run_event(**upstream.run("Android", "master", M3))

    assert main(["decide"]) == 0

    outputs = read_github_file(gh_output)
    # The attempt that was approved goes with each run: every platform passed M3 on its second.
    assert json.loads(outputs.pop("runs")) == [
        {
            "workflow": workflow,
            "path": REQUIRED_WORKFLOWS[workflow],
            "id": run_id,
            "attempt": 2,
            "conclusion": "success",
        }
        for workflow, run_id in zip(WORKFLOWS, RUN_IDS["master", M3], strict=True)
    ]
    assert outputs == {
        "publish": "true",
        "ref": "refs/heads/master",
        "commit": M3,
        "daily": "true",
        "s3": "true",
        "latest": "false",
        "release": "false",
        "dry-run": "false",
    }
    summary = gh_summary.read_text(encoding="utf-8")
    assert "| Trigger | Android run 42969993919 completed |" in summary
    assert "| Decision | ✅ publish: every platform built this commit |" in summary


def test_decide_exits_quietly_with_the_reason_when_nothing_is_published(
    upstream: Upstream,
    actions: Callable[..., None],
    workflow_run_event: Callable[..., Path],
    gh_output: Path,
    gh_summary: Path,
) -> None:
    actions(PUBLISH_DAILY_RELEASE="true", PUBLISH_S3="true")
    workflow_run_event(**{**upstream.run("Linux", "master", M3), "event": "pull_request"})

    assert main(["decide"]) == 0

    outputs = read_github_file(gh_output)
    assert (outputs["publish"], outputs["daily"], outputs["s3"]) == ("false", "false", "false")
    assert (outputs["ref"], outputs["commit"], outputs["runs"]) == ("", "", "[]")
    assert (
        "| Decision | ⏭️ nothing to publish: the Linux run was started by `pull_request`, "
        "not by a push |"
    ) in gh_summary.read_text(encoding="utf-8")


def test_a_dry_run_plans_every_target_and_says_so(
    upstream: Upstream, actions: Callable[..., None], gh_output: Path, gh_summary: Path
) -> None:
    # A fork publishes to nothing, but a dry run shows what upstream would publish.
    actions(PUBLISH_DAILY_RELEASE="false", PUBLISH_S3="false", PUBLISH_DRY_RUN="true")

    assert main(["decide", "--ref", "master"]) == 0

    outputs = read_github_file(gh_output)
    assert (outputs["publish"], outputs["dry-run"]) == ("true", "true")
    assert (outputs["daily"], outputs["s3"], outputs["commit"]) == ("true", "true", M3)
    summary = gh_summary.read_text(encoding="utf-8")
    assert "| Trigger | request for `master` |" in summary
    assert "dry run: nothing is written" in summary


def test_a_manual_request_ignores_the_event_and_can_name_a_commit(
    upstream: Upstream,
    actions: Callable[..., None],
    workflow_run_event: Callable[..., Path],
    gh_output: Path,
) -> None:
    actions(PUBLISH_DAILY_RELEASE="true", PUBLISH_S3="true", PUBLISH_REF="v5.1.5")
    workflow_run_event(**upstream.run("Android", "master", M3))

    assert main(["decide"]) == 0
    assert read_github_file(gh_output)["ref"] == "refs/tags/v5.1.5"

    gh_output.write_text("")
    assert main(["decide", "--ref", "master", "--commit", M1]) == 0
    assert read_github_file(gh_output)["publish"] == "false"


def test_the_commit_input_reaches_the_decision_from_the_environment(
    upstream: Upstream,
    actions: Callable[..., None],
    workflow_run_event: Callable[..., Path],
    gh_output: Path,
    gh_summary: Path,
) -> None:
    # Pasted into the form with a space after it.
    upstream.set("master", M3, Linux="failure")
    actions(
        PUBLISH_DAILY_RELEASE="true",
        PUBLISH_S3="true",
        PUBLISH_REF="master",
        PUBLISH_COMMIT=f"{M2} ",
    )

    assert main(["decide"]) == 0

    outputs = read_github_file(gh_output)
    assert (outputs["publish"], outputs["commit"]) == ("true", M2)
    assert f"| Trigger | request for `master` at `{M2}` |" in gh_summary.read_text(encoding="utf-8")

    # A completed build has no inputs: the workflow passes the variable empty.
    actions(PUBLISH_REF="", PUBLISH_COMMIT="")
    workflow_run_event(**upstream.run("Android", "master", M3))
    gh_output.write_text("")
    assert main(["decide"]) == 0
    assert read_github_file(gh_output)["commit"] == M3


def test_decide_needs_an_event_or_a_ref(actions: Callable[..., None], capsys) -> None:
    actions()

    assert main(["decide"]) == 2
    assert "no workflow_run event and no --ref" in capsys.readouterr().out


def test_unreadable_published_state_fails_the_decision(
    upstream: Upstream,
    actions: Callable[..., None],
    gh_output: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    actions(PUBLISH_DAILY_RELEASE="true", PUBLISH_S3="true")
    upstream.records["builds/master/published.json"] = {"commit": "not a commit"}

    assert main(["decide", "--ref", "master"]) == 1

    assert "does not record a commit" in capsys.readouterr().out
    assert gh_output.read_text() == ""


@pytest.mark.parametrize(
    ("target", "published", "republish", "proceed"),
    [
        ("daily", None, "false", "true"),
        ("s3", M2, "false", "true"),
        # Another run published this commit, or a newer one, while this job waited its turn.
        ("s3", M3, "false", "false"),
        ("daily", M4, "false", "false"),
        # A run started by hand publishes the same commit again, but never an older one.
        ("s3", M3, "true", "true"),
        ("daily", M4, "true", "false"),
    ],
)
def test_confirm_rechecks_one_target_inside_its_job(
    upstream: Upstream,
    actions: Callable[..., None],
    gh_output: Path,
    gh_summary: Path,
    target: str,
    published: str | None,
    republish: str,
    proceed: str,
) -> None:
    actions(PUBLISH_REF="refs/heads/master", PUBLISH_COMMIT=M3, PUBLISH_REPUBLISH=republish)
    if published:
        upstream.publish(published, target)

    assert main(["confirm", "--target", target]) == 0

    assert read_github_file(gh_output) == {"proceed": proceed}
    assert ("not published" in gh_summary.read_text(encoding="utf-8")) == (proceed == "false")


def test_a_tag_s_packages_go_to_its_github_release_and_a_published_one_is_only_checked(
    upstream: Upstream,
) -> None:
    # Upstream's v5.1.5 release is published: its files do not change under its users. Its
    # job starts all the same, and fails if the release lacks a file: a release that was
    # published before its packages arrived must not pass for complete.
    kept = "is already published: its job checks that it has these files, and changes none without force_release"
    decision = _completes(upstream, "MacOS", "v5.1.5", S1)
    assert decision.selected == ("s3", "latest", "release")
    assert decision.targets["release"] == kept
    # Not by hand either, without the input that says so, and never for a completed build.
    assert decide(REPO, ref_name="v5.1.5").targets["release"] == kept
    assert decide(REPO, ref_name="v5.1.5", force=("latest",)).targets["release"] == kept
    automatic = _completes(upstream, "MacOS", "v5.1.5", S1, force=("release",))
    assert automatic.targets["release"] == kept

    forced = decide(REPO, ref_name="v5.1.5", force=("release",))
    assert forced.selected == ("s3", "latest", "release")
    assert forced.targets["release"] == (
        "is already published, and its files are replaced as requested"
    )

    # Upstream's v5.1.5 as GitHub reports it: published while the repository's release
    # immutability setting was on. Nothing replaces its files, which the decision says, forced
    # or not.
    upstream.published["v5.1.5"]["immutable"] = True
    locked = (
        "is published and immutable: its job checks that it has these files, and nothing can "
        "change them, force_release included"
    )
    assert decide(REPO, ref_name="v5.1.5").targets["release"] == locked
    forced = decide(REPO, ref_name="v5.1.5", force=("release",))
    assert (forced.selected, forced.targets["release"]) == (("s3", "latest", "release"), locked)

    # A draft, or no release at all: a token that only reads sees neither, so the job starts
    # and looks for the draft itself.
    del upstream.published["v5.1.5"]
    decision = _completes(upstream, "MacOS", "v5.1.5", S1)
    assert decision.selected == ("s3", "latest", "release")
    assert decision.targets["release"] == (
        "has no published release: the packages go to the tag's draft"
    )
    assert f"repos/{REPO}/releases/tags/v5.1.5" in upstream.requests
    # S3 holds the tag by now: another build of it that completes reaches the release only,
    # where the job finds the files it already has.
    upstream.publish(S1, "s3", "latest", ref_name="v5.1.5")
    assert _completes(upstream, "Linux", "v5.1.5", S1).selected == ("release",)
    # A branch has no release.
    assert "release" not in decide(REPO, ref_name="master").targets


def test_the_force_release_input_reaches_the_decision_and_the_job_that_writes_the_release(
    upstream: Upstream,
    actions: Callable[..., None],
    gh_output: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    actions(PUBLISH_S3="true", PUBLISH_REF="refs/tags/v5.1.5", PUBLISH_COMMIT=S1)

    # A repository that publishes no GitHub releases plans none, published or not.
    del upstream.published["v5.1.5"]
    assert main(["decide", "--ref", "v5.1.5"]) == 0
    assert read_github_file(gh_output)["release"] == "false"

    # A published release: its job starts to check it, and replaces its files only when the
    # input says so.
    upstream.published["v5.1.5"] = {"id": 401155735, "draft": False}
    actions(PUBLISH_RELEASE_ASSETS="true")
    for forced in (False, True):
        gh_output.write_text("")
        actions(PUBLISH_FORCE_RELEASE=str(forced).lower())
        assert main(["decide", "--ref", "v5.1.5"]) == 0
        assert main(["confirm", "--target", "release", "--republish"]) == 0
        assert read_github_file(gh_output)["release"] == "true"
        assert read_github_file(gh_output)["proceed"] == "true"
        assert ("its files are replaced as requested" in capsys.readouterr().out) is forced


def test_the_force_latest_input_reaches_the_decision_and_the_job_that_writes_latest(
    upstream: Upstream, actions: Callable[..., None], gh_output: Path
) -> None:
    upstream.publish(S2, "latest", ref_name="v5.1.6")
    actions(PUBLISH_S3="true", PUBLISH_REF="refs/tags/v5.1.5", PUBLISH_COMMIT=S1)

    assert main(["decide", "--ref", "v5.1.5"]) == 0
    assert main(["confirm", "--target", "latest", "--republish"]) == 0
    assert read_github_file(gh_output)["latest"] == "false"
    assert read_github_file(gh_output)["proceed"] == "false"

    gh_output.write_text("")
    actions(PUBLISH_FORCE_LATEST="true")
    assert main(["decide", "--ref", "v5.1.5"]) == 0
    assert main(["confirm", "--target", "latest", "--republish"]) == 0
    assert read_github_file(gh_output)["latest"] == "true"
    assert read_github_file(gh_output)["proceed"] == "true"


@pytest.mark.parametrize("target", ["s3", "latest", "release"])
def test_confirm_stops_a_job_whose_tag_was_moved_or_deleted_while_it_waited(
    upstream: Upstream,
    actions: Callable[..., None],
    gh_output: Path,
    gh_summary: Path,
    target: str,
) -> None:
    actions(PUBLISH_REF="refs/tags/v5.1.5", PUBLISH_COMMIT=S1)
    del upstream.published["v5.1.5"]

    # The annotated tag still names the commit whose packages wait.
    assert main(["confirm", "--target", target]) == 0
    assert read_github_file(gh_output) == {"proceed": "true"}

    for now, problem in (
        (S2, f"the tag v5.1.5 now names {S2}, not {S1}, which these packages were built from"),
        (None, "the tag v5.1.5 no longer exists"),
    ):
        upstream.tags = {} if now is None else {"v5.1.5": now}
        gh_output.write_text("")
        assert main(["confirm", "--target", target]) == 0
        assert read_github_file(gh_output) == {"proceed": "false"}
        assert f"| Result | ⏭️ not published: {problem} |" in gh_summary.read_text(encoding="utf-8")

    # A tag that is not annotated names its commit directly.
    upstream.tags = {"v5.1.6": S1}
    actions(PUBLISH_REF="refs/tags/v5.1.6")
    gh_output.write_text("")
    assert main(["confirm", "--target", target]) == 0
    assert read_github_file(gh_output) == {"proceed": "true"}


def test_confirm_refuses_a_target_or_ref_the_decision_could_not_have_chosen(
    upstream: Upstream, actions: Callable[..., None], gh_output: Path
) -> None:
    actions()
    argv = ["confirm", "--commit", M3]

    assert main([*argv, "--target", "latest", "--ref", "refs/heads/master"]) == 1
    assert main([*argv, "--target", "release", "--ref", "refs/heads/master"]) == 1
    assert main([*argv, "--target", "daily", "--ref", "refs/heads/Stable_V5.1"]) == 1
    assert main([*argv, "--target", "s3", "--ref", "refs/tags/master"]) == 1
    assert main([*argv, "--target", "s3", "--ref", "refs/heads/feature"]) == 1
    assert main(["confirm", "--target", "s3", "--ref", "refs/heads/master", "--commit", "x"]) == 1
    upstream.publish(STRAY, "s3")
    assert main([*argv, "--target", "s3", "--ref", "refs/heads/master"]) == 1
    assert gh_output.read_text() == ""
