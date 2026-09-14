"""Only the current open PR head or current master may publish build reports."""

from unittest.mock import patch

import pytest
from report_context import resolve_context


@pytest.mark.parametrize(
    "state,sha,current",
    [("open", "new", "true"), ("closed", "new", "false"), ("open", "old", "false")],
)
def test_pr_reports_ignore_closed_or_superseded_heads(state, sha, current):
    run = {"head_sha": "new", "event": "pull_request", "pull_requests": [{"number": 42}]}
    with patch(
        "report_context.api", return_value={"number": 42, "state": state, "head": {"sha": sha}}
    ):
        result = resolve_context("o/r", run)
    assert result["current"] == current
    assert result["pr"] == ("42" if current == "true" else "")


def test_fork_run_resolves_pr_via_commit():
    with patch(
        "report_context.api",
        side_effect=[[{"number": 42}], {"number": 42, "state": "open", "head": {"sha": "new"}}],
    ) as api:
        assert resolve_context("o/r", {"head_sha": "new", "event": "pull_request"})["pr"] == "42"
    assert api.call_args_list[0].args == ("repos/o/r/commits/new/pulls",)


@pytest.mark.parametrize("head,current", [("new", "true"), ("old", "false")])
def test_old_master_completion_cannot_replace_baseline(head, current):
    with patch("report_context.api", return_value={"object": {"sha": head}}):
        assert (
            resolve_context("o/r", {"head_sha": "new", "event": "push", "head_branch": "master"})[
                "current"
            ]
            == current
        )
