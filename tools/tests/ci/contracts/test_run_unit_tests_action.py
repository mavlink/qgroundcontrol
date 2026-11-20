"""Every Codecov upload, in run-unit-tests and in the workflows, uses OIDC and names its reports."""

from __future__ import annotations

from .._workflows import step_groups, uses_action


def test_codecov_uploads_use_oidc_and_only_the_named_reports() -> None:
    # The CODECOV_TOKEN secret was rejected (404 "Repository not found"), so master coverage never
    # reached Codecov; the default search also uploaded Qt's and CPM packages' coverage files.
    uploads = [
        (label.split(":")[0], step)
        for label, _, steps in step_groups()
        for step in steps
        if uses_action(step, "codecov/codecov-action")
    ]
    assert {source for source, _ in uploads} >= {"linux.yml", "actions/run-unit-tests"}
    for source, step in uploads:
        options = step["with"]
        assert options["use_oidc"] is True, source
        assert options["disable_search"] is True, source
        assert "token" not in options, source
        assert options["files"], source
