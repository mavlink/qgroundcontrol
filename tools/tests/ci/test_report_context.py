"""Only the current open PR head or current master may publish build reports."""

import json
import subprocess
from pathlib import Path
from unittest.mock import patch

import pytest
from ci.reporting import report_context as mod
from ci.reporting.report_context import main, resolve_context

from .._helpers import completed

CREATED = "2026-10-22T13:31:50Z"


def _pr(state="open", head_sha="new", base_ref="master"):
    return {
        "number": 42,
        "state": state,
        "head": {"sha": head_sha},
        "base": {"sha": "base", "ref": base_ref},
    }


def _api(pr, commits=({"sha": "tip"},)):
    """Answer the PR lookup with *pr* and the base-branch commit lookup with *commits*."""

    def answer(path):
        if not path.startswith("repos/o/r/commits?"):
            return pr
        if isinstance(commits, Exception):
            raise commits
        return list(commits) if isinstance(commits, tuple) else commits

    return answer


def _pr_run(**fields):
    return {
        "head_sha": "new",
        "event": "pull_request",
        "pull_requests": [{"number": 42}],
        "created_at": CREATED,
        **fields,
    }


@pytest.mark.parametrize(
    "state,sha,current",
    [("open", "new", "true"), ("closed", "new", "false"), ("open", "old", "false")],
)
def test_pr_reports_ignore_closed_or_superseded_heads(state, sha, current):
    with patch("ci.reporting.report_context.api", side_effect=_api(_pr(state, sha))):
        result = resolve_context("o/r", _pr_run())
    assert result["current"] == current
    assert result["pr"] == ("42" if current == "true" else "")
    assert result["base_sha"] == ("tip" if current == "true" else "")


@pytest.mark.parametrize(
    "branch,query",
    [
        ("master", "sha=master&until=2026-10-22T13%3A31%3A50Z&per_page=1"),
        ("release/v5.0", "sha=release%2Fv5.0&until=2026-10-22T13%3A31%3A50Z&per_page=1"),
    ],
)
def test_base_is_the_base_branch_tip_when_the_run_was_created(branch, query, capsys):
    # A re-run starts later but builds the merge its event created.
    run = _pr_run(run_started_at="2026-10-23T09:00:00Z")
    with patch("ci.reporting.report_context.api", side_effect=_api(_pr(base_ref=branch))) as api:
        assert resolve_context("o/r", run)["base_sha"] == "tip"
    assert api.call_args_list[-1].args == (f"repos/o/r/commits?{query}",)
    assert "base.sha" not in capsys.readouterr().out


@pytest.mark.parametrize(
    "commits,reason",
    [
        ((), "master has no commit before 2026-10-22T13:31:50Z"),
        (({},), "master has no commit before"),
        ({"message": "Not Found"}, "master has no commit before"),
        (
            subprocess.CalledProcessError(1, ["gh"], stderr="HTTP 502: Bad Gateway\n"),
            "looking up master at 2026-10-22T13:31:50Z failed: HTTP 502: Bad Gateway",
        ),
        (subprocess.TimeoutExpired(["gh"], 60), "looking up master at"),
        (OSError("gh: not found"), "failed: gh: not found"),
        (json.JSONDecodeError("Expecting value", "", 0), "failed: Expecting value"),
    ],
)
def test_base_falls_back_to_the_prs_base_sha_when_the_lookup_fails(commits, reason, capsys):
    with patch("ci.reporting.report_context.api", side_effect=_api(_pr(), commits)):
        assert resolve_context("o/r", _pr_run())["base_sha"] == "base"
    out = capsys.readouterr().out
    assert "Comparing with the PR's base.sha base: " in out
    assert reason in out


@pytest.mark.parametrize(
    "run,pr",
    [(_pr_run(created_at=None), _pr()), (_pr_run(), _pr(base_ref=""))],
)
def test_base_without_a_branch_or_creation_time_is_the_prs_base_sha(run, pr, capsys):
    with patch("ci.reporting.report_context.api", side_effect=_api(pr)) as api:
        assert resolve_context("o/r", run)["base_sha"] == "base"
    assert not any("/commits?" in call.args[0] for call in api.call_args_list)
    assert "Comparing with the PR's base.sha base: the PR has no base branch" in (
        capsys.readouterr().out
    )


def test_fork_run_resolves_pr_via_head_branch():
    run = {
        "head_sha": "new",
        "event": "pull_request",
        "head_branch": "feature/a b",
        "head_repository": {"owner": {"login": "fork"}},
    }
    with patch(
        "ci.reporting.report_context.api",
        side_effect=[[{"number": 42}], _pr()],
    ) as api:
        assert resolve_context("o/r", run)["pr"] == "42"
    assert api.call_args_list[0].args == ("repos/o/r/pulls?state=open&head=fork:feature%2Fa%20b",)


def test_fork_run_without_head_match_falls_back_to_commit():
    run = {
        "head_sha": "new",
        "event": "pull_request",
        "head_branch": "feature",
        "head_repository": {"owner": {"login": "fork"}},
    }
    with patch(
        "ci.reporting.report_context.api",
        side_effect=[[], [{"number": 42}], _pr()],
    ) as api:
        assert resolve_context("o/r", run)["pr"] == "42"
    assert api.call_args_list[1].args == ("repos/o/r/commits/new/pulls",)


def test_fork_run_resolves_pr_via_commit():
    with patch(
        "ci.reporting.report_context.api",
        side_effect=[[{"number": 42}], _pr()],
    ) as api:
        assert resolve_context("o/r", {"head_sha": "new", "event": "pull_request"})["pr"] == "42"
    assert api.call_args_list[0].args == ("repos/o/r/commits/new/pulls",)


@pytest.mark.parametrize("event", ["push", "workflow_dispatch", "merge_group", "schedule"])
def test_only_pull_request_builds_resolve_a_pr(event):
    run = {
        "head_sha": "new",
        "event": event,
        "head_branch": "master",
        "pull_requests": [{"number": 42}],
    }
    with patch("ci.reporting.report_context.api") as api:
        assert resolve_context("o/r", run) == {
            "pr": "",
            "current": "false",
            "sha": "new",
            "base_sha": "",
        }
    api.assert_not_called()


def test_main_reads_the_workflow_run_payload(gh_output, monkeypatch, workflow_run_event):
    monkeypatch.setenv("GITHUB_REPOSITORY", "o/r")
    workflow_run_event(**_pr_run())
    with patch("ci.reporting.report_context.api", side_effect=_api(_pr())):
        assert main([]) == 0
    assert gh_output.read_text() == "pr=42\ncurrent=true\nsha=new\nbase_sha=tip\n"


def test_main_requires_a_workflow_run_payload(gh_output, monkeypatch, workflow_run_event, capsys):
    monkeypatch.setenv("GITHUB_REPOSITORY", "o/r")
    workflow_run_event().write_text('{"pull_request": {"number": 42}}')
    assert main([]) == 1
    assert "no workflow_run" in capsys.readouterr().out
    assert gh_output.read_text() == ""


LEGACY_TAG = '<!-- thollander/actions-comment-pull-request "build-results" -->'


class FakeGh:
    """Record gh calls; list the given issue comments and capture each written payload."""

    def __init__(self, comments, fail_writes=False):
        self.comments = comments
        self.fail_writes = fail_writes
        self.calls = []
        self.payloads = []

    def __call__(self, *args, **kwargs):
        self.calls.append(args)
        if "--input" in args:
            self.payloads.append(json.loads(Path(args[args.index("--input") + 1]).read_text()))
            if self.fail_writes:
                raise subprocess.CalledProcessError(1, ["gh"], stderr="HTTP 403: Forbidden")
            return completed()
        return completed("\n".join(json.dumps(comment) for comment in self.comments))


def _publish(tmp_path, monkeypatch, workflow_run_event, gh, head="new"):
    monkeypatch.setenv("GITHUB_REPOSITORY", "o/r")
    workflow_run_event(head_sha="new", event="pull_request", pull_requests=[{"number": 42}])
    comment = tmp_path / "comment.md"
    comment.write_text("## Build Results\n\nbody\n", encoding="utf-8")
    monkeypatch.setattr(mod, "gh", gh)
    with patch("ci.reporting.report_context.api", return_value=_pr(head_sha=head)):
        return main(["--comment", str(comment)])


def test_publish_creates_the_comment_with_its_marker(
    tmp_path, monkeypatch, workflow_run_event, gh_output
):
    gh = FakeGh([{"id": 7, "type": "User", "body": f"quoting {mod.COMMENT_MARKER}"}])
    assert _publish(tmp_path, monkeypatch, workflow_run_event, gh) == 0
    assert gh.calls[0][:4] == ("api", "--paginate", "--jq", ".[] | {id, type: .user.type, body}")
    assert gh.calls[0][4] == "repos/o/r/issues/42/comments?per_page=100"
    assert gh.calls[1][:4] == ("api", "--method", "POST", "repos/o/r/issues/42/comments")
    assert gh.payloads == [{"body": f"## Build Results\n\nbody\n\n{mod.COMMENT_MARKER}\n"}]


@pytest.mark.parametrize("marker", [mod.COMMENT_MARKER, LEGACY_TAG])
def test_publish_updates_the_bots_existing_comment(
    tmp_path, monkeypatch, workflow_run_event, gh_output, marker
):
    gh = FakeGh(
        [
            {"id": 5, "type": "Bot", "body": "## Codecov Report"},
            {"id": 9, "type": "Bot", "body": f"## Build Results\n\nold\n\n{marker}"},
        ]
    )
    assert _publish(tmp_path, monkeypatch, workflow_run_event, gh) == 0
    assert gh.calls[1][:4] == ("api", "--method", "PATCH", "repos/o/r/issues/comments/9")
    assert gh.payloads[0]["body"].endswith(f"{mod.COMMENT_MARKER}\n")


def test_publish_skips_a_superseded_head(tmp_path, monkeypatch, workflow_run_event, gh_output):
    gh = FakeGh([])
    assert _publish(tmp_path, monkeypatch, workflow_run_event, gh, head="newer") == 0
    assert gh.calls == []
    assert "current=false" in gh_output.read_text()


def test_publish_failure_fails_the_step(
    tmp_path, monkeypatch, workflow_run_event, gh_output, capsys
):
    gh = FakeGh([], fail_writes=True)
    assert _publish(tmp_path, monkeypatch, workflow_run_event, gh) == 1
    assert (
        "::error::Cannot publish the build-results comment on #42: HTTP 403: Forbidden"
        in capsys.readouterr().out
    )
