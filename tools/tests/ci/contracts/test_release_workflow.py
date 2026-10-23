"""Release workflow contracts: draft a GitHub Release for a hand-pushed tag, nothing more."""

from __future__ import annotations

from ..._helpers import get_step, load_workflow


def test_release_runs_for_release_tags_and_dispatches_with_a_tag() -> None:
    on = load_workflow("release.yml")["on"]
    assert set(on) == {"push", "workflow_dispatch"}
    # Same filter as the platform workflows that build the tag.
    assert on["push"] == {"tags": ["v[0-9]+.[0-9]+.[0-9]+"]}
    assert on["workflow_dispatch"]["inputs"]["tag"]["required"] is True
    assert load_workflow("linux.yml")["on"]["push"]["tags"] == on["push"]["tags"]


def test_release_only_drafts_a_release() -> None:
    workflow = load_workflow("release.yml")
    assert workflow["permissions"] == {"contents": "read"}
    assert list(workflow["jobs"]) == ["draft-release"]
    job = workflow["jobs"]["draft-release"]
    assert job["permissions"] == {"contents": "write"}
    assert job["env"]["RELEASE_TAG"] == "${{ inputs.tag || github.ref_name }}"

    steps = job["steps"]
    checkout = get_step(steps, "Checkout")
    check = get_step(steps, "Require a Stable branch tag")
    setup = get_step(steps, "Setup Python")
    notes = get_step(steps, "Draft release notes")
    assert steps == [steps[0], checkout, check, setup, notes]
    # Full history: every tag for git-cliff's range and origin/* branches for the Stable* check.
    assert checkout["with"] == {"fetch-depth": 0, "persist-credentials": False}
    assert check["run"] == (
        'python3 tools/ci/publish/release_branch_check.py --ref "refs/tags/${RELEASE_TAG}"'
    )
    assert setup["uses"] == "./.github/actions/setup-python"
    assert setup["with"] == {"groups": "release"}
    assert notes["run"].split() == [
        "python3",
        "tools/ci/publish/release_notes.py",
        '"${RELEASE_TAG}"',
        "--output",
        '"${RUNNER_TEMP}/release-notes.md"',
        "--draft",
    ]
    assert notes["env"] == {
        "GH_REPO": "${{ github.repository }}",
        "GH_TOKEN": "${{ github.token }}",
    }
    for step in steps:
        assert "if" not in step
        assert "continue-on-error" not in step
        # Inputs reach scripts through env, never through template expansion.
        assert "${{" not in step.get("run", "")
        assert "artifact" not in step.get("uses", "")
        assert "upload" not in step.get("run", "")


def test_ci_scripts_installs_git_cliff_for_the_release_notes_tests() -> None:
    legs = load_workflow("ci-scripts.yml")["jobs"]["test-ci-scripts"]["strategy"]["matrix"][
        "include"
    ]
    assert legs
    assert all("release" in leg["groups"].split(",") for leg in legs)
