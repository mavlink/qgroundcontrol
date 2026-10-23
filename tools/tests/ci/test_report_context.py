"""Only the current open PR head or current master may publish build reports."""

from unittest.mock import patch

import pytest
from ci.reporting.report_context import main, resolve_context


def _pr(state="open", head_sha="new"):
    return {"number": 42, "state": state, "head": {"sha": head_sha}, "base": {"sha": "base"}}


@pytest.mark.parametrize(
    "state,sha,current",
    [("open", "new", "true"), ("closed", "new", "false"), ("open", "old", "false")],
)
def test_pr_reports_ignore_closed_or_superseded_heads(state, sha, current):
    run = {"head_sha": "new", "event": "pull_request", "pull_requests": [{"number": 42}]}
    with patch("ci.reporting.report_context.api", return_value=_pr(state, sha)):
        result = resolve_context("o/r", run)
    assert result["current"] == current
    assert result["pr"] == ("42" if current == "true" else "")
    assert result["base_sha"] == ("base" if current == "true" else "")


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


@pytest.mark.parametrize("head,current", [("new", "true"), ("old", "false")])
def test_old_master_completion_cannot_replace_baseline(head, current):
    with patch("ci.reporting.report_context.api", return_value={"object": {"sha": head}}):
        assert (
            resolve_context("o/r", {"head_sha": "new", "event": "push", "head_branch": "master"})[
                "current"
            ]
            == current
        )


@pytest.mark.parametrize(
    "run,baseline",
    [
        ({"event": "push", "head_branch": "master", "conclusion": "success"}, "true"),
        ({"event": "push", "head_branch": "master", "conclusion": "failure"}, "false"),
        ({"event": "push", "head_branch": "Stable_V5.0", "conclusion": "success"}, "false"),
        ({"event": "workflow_dispatch", "head_branch": "master", "conclusion": "success"}, "false"),
        ({"event": "merge_group", "head_branch": "master", "conclusion": "success"}, "false"),
    ],
)
def test_only_successful_master_pushes_save_baselines(run, baseline):
    with patch("ci.reporting.report_context.api", return_value={"object": {"sha": "new"}}):
        result = resolve_context("o/r", {"head_sha": "new", **run})
    assert result["baseline"] == baseline
    assert result["pr"] == ""


@pytest.mark.parametrize("event", ["push", "workflow_dispatch", "merge_group", "schedule"])
def test_only_pull_request_builds_resolve_a_pr(event):
    run = {"head_sha": "new", "event": event, "pull_requests": [{"number": 42}]}
    with patch("ci.reporting.report_context.api", return_value={"object": {"sha": "old"}}):
        assert resolve_context("o/r", run)["pr"] == ""


def test_main_reads_the_workflow_run_payload(gh_output, monkeypatch, workflow_run_event):
    monkeypatch.setenv("GITHUB_REPOSITORY", "o/r")
    workflow_run_event(head_sha="new", event="pull_request", pull_requests=[{"number": 42}])
    with patch("ci.reporting.report_context.api", return_value=_pr()):
        assert main([]) == 0
    assert gh_output.read_text() == (
        "pr=42\ncurrent=true\nsha=new\nbase_sha=base\nbaseline=false\n"
    )


def test_main_requires_a_workflow_run_payload(gh_output, monkeypatch, workflow_run_event, capsys):
    monkeypatch.setenv("GITHUB_REPOSITORY", "o/r")
    workflow_run_event().write_text('{"pull_request": {"number": 42}}')
    assert main([]) == 1
    assert "no workflow_run" in capsys.readouterr().out
    assert gh_output.read_text() == ""
