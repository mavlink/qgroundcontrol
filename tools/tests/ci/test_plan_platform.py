"""Tests for plan_platform.py."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING, Any

import pytest
from ci.planning import detect_changes
from ci.planning.android_matrix import build_matrix
from ci.planning.plan_docker_builds import load_variants
from ci.planning.plan_platform import PLATFORMS, main
from qgc_tools import release_tag_check

from .._helpers import commit_files, git, load_workflow, read_github_file

if TYPE_CHECKING:
    from pathlib import Path

UPSTREAM = "mavlink/qgroundcontrol"
GHCR = "ghcr.io/mavlink/qgroundcontrol"
DOCKER_HUB = "dronecode/qgroundcontrol"
# Parametrize values for every Docker variant (or all but the last, Android), resolved when a test runs:
# collection must not read deploy/docker/variants.json, which the Windows job's sparse checkout lacks.
ALL = "all"
ALL_BUT_ANDROID = "all-but-android"


def _all_docker_platforms() -> list[str]:
    return [variant["platform"] for variant in load_variants()]


@pytest.fixture
def event(monkeypatch: pytest.MonkeyPatch, gh_output: Path) -> pytest.MonkeyPatch:
    monkeypatch.setenv("GITHUB_REF_TYPE", "branch")
    monkeypatch.setattr(detect_changes, "get_changed_files", lambda: ["src/Vehicle.cc"])
    return monkeypatch


def _plan(gh_output: Path, platform: str) -> tuple[str, list[dict[str, Any]]]:
    assert main(["--platform", platform]) == 0
    outputs = read_github_file(gh_output)
    assert set(outputs) == {"should_build", "matrix"}
    # Compact JSON keeps log lines short and the workflow's contains() checks exact.
    assert ", " not in outputs["matrix"]
    assert ": " not in outputs["matrix"]
    return outputs["should_build"], json.loads(outputs["matrix"])


def test_workflow_runs_the_planner_for_the_callers_platform() -> None:
    job = load_workflow("_detect-changes.yml")["jobs"]["detect"]
    (run,) = [step for step in job["steps"] if "run" in step]
    assert run["env"]["PLATFORM"] == "${{ inputs.platform }}"
    assert run["run"] == (
        'python3 "${GITHUB_WORKSPACE}/tools/ci/planning/plan_platform.py" --platform "${PLATFORM}"'
    )
    for platform in PLATFORMS:
        workflow = load_workflow(f"{platform}.yml")
        assert workflow["jobs"]["changes"]["with"] == {"platform": platform}


@pytest.mark.parametrize("platform", ["linux", "windows", "macos", "ios"])
@pytest.mark.parametrize(
    ("changed", "should_build"), [("src/Vehicle.cc", "true"), ("docs/a.md", "false")]
)
def test_platforms_with_their_own_matrix_plan_only_whether_to_build(
    event: pytest.MonkeyPatch, gh_output: Path, platform: str, changed: str, should_build: str
) -> None:
    event.setenv("EVENT_NAME", "pull_request")
    event.setattr(detect_changes, "get_changed_files", lambda: [changed])
    assert _plan(gh_output, platform) == (should_build, [])


def test_unknown_platform_is_rejected() -> None:
    with pytest.raises(SystemExit):
        main(["--platform", "docker-linux"])


@pytest.mark.parametrize("platform", PLATFORMS)
def test_tag_fails_when_git_cannot_answer(
    event: pytest.MonkeyPatch, gh_output: Path, capsys: pytest.CaptureFixture[str], platform: str
) -> None:
    def fail(*_: str) -> bool:
        raise release_tag_check.GitQueryError("no history")

    event.setenv("EVENT_NAME", "push")
    event.setenv("GITHUB_REF_TYPE", "tag")
    event.setattr(detect_changes, "check_release_tag", fail)
    assert main(["--platform", platform]) == 1
    assert gh_output.read_text() == ""
    assert "::error::Cannot determine whether tag is on a Stable* branch" in capsys.readouterr().out


@pytest.mark.parametrize("platform", PLATFORMS)
@pytest.mark.parametrize(("release_tag", "should_build"), [(True, "true"), (False, "false")])
def test_tags_build_only_stable_releases(
    event: pytest.MonkeyPatch, gh_output: Path, platform: str, release_tag: bool, should_build: str
) -> None:
    event.setenv("EVENT_NAME", "push")
    event.setenv("GITHUB_REF_TYPE", "tag")
    event.setattr(detect_changes, "check_release_tag", lambda *_: release_tag)
    built, matrix = _plan(gh_output, platform)
    assert built == should_build
    if not release_tag:
        assert matrix == []


# --- Android -------------------------------------------------------------------------------------


def test_android_pull_request_drops_the_windows_leg(
    event: pytest.MonkeyPatch, gh_output: Path
) -> None:
    event.setenv("EVENT_NAME", "pull_request")
    assert _plan(gh_output, "android") == ("true", build_matrix(is_pr=True))


@pytest.mark.parametrize("event_name", ["push", "merge_group", "workflow_dispatch"])
def test_android_other_events_include_windows(
    event: pytest.MonkeyPatch, gh_output: Path, event_name: str
) -> None:
    event.setenv("EVENT_NAME", event_name)
    assert _plan(gh_output, "android") == ("true", build_matrix(is_pr=False))


def test_android_skips_pull_requests_without_android_changes(
    event: pytest.MonkeyPatch, gh_output: Path
) -> None:
    event.setenv("EVENT_NAME", "pull_request")
    event.setattr(detect_changes, "get_changed_files", lambda: ["docs/index.md"])
    assert _plan(gh_output, "android") == ("false", [])


# --- Docker --------------------------------------------------------------------------------------


def _docker_platforms(gh_output: Path) -> tuple[str, list[str]]:
    should_build, matrix = _plan(gh_output, "docker")
    return should_build, [leg["platform"] for leg in matrix]


@pytest.mark.parametrize(
    ("changed", "expected"),
    [
        (["docs/index.md"], []),
        (["src/Vehicle/Vehicle.cc"], ["Linux-Ubuntu-24.04", "Android"]),
        (["android/AndroidManifest.xml"], ["Android"]),
        (["deploy/linux/AppImage.sh"], ALL_BUT_ANDROID),
        (None, ALL),
    ],
)
def test_docker_plans_pull_requests_from_their_diff(
    event: pytest.MonkeyPatch, gh_output: Path, changed: list[str] | None, expected: list[str] | str
) -> None:
    if expected == ALL:
        expected = _all_docker_platforms()
    elif expected == ALL_BUT_ANDROID:
        expected = _all_docker_platforms()[:-1]
    event.setenv("EVENT_NAME", "pull_request")
    event.setattr(detect_changes, "get_changed_files", lambda: changed)
    assert _docker_platforms(gh_output) == ("true" if expected else "false", expected)


@pytest.mark.parametrize("event_name", ["push", "workflow_dispatch"])
def test_docker_builds_every_variant_outside_pull_requests(
    event: pytest.MonkeyPatch, gh_output: Path, event_name: str
) -> None:
    def unexpected() -> None:
        raise AssertionError("non-PR events must not diff")

    event.setenv("EVENT_NAME", event_name)
    event.setattr(detect_changes, "get_changed_files", unexpected)
    assert _docker_platforms(gh_output) == ("true", _all_docker_platforms())


def test_docker_rename_out_of_docker_inputs_uses_full_matrix(
    monkeypatch: pytest.MonkeyPatch, git_repo: Path, gh_output: Path
) -> None:
    base = commit_files(git_repo, {"deploy/docker/Dockerfile": "FROM scratch\n" * 20}, "base")
    (git_repo / "docs").mkdir()
    git(git_repo, "mv", "deploy/docker/Dockerfile", "docs/Dockerfile")
    git(git_repo, "commit", "-qm", "move")

    monkeypatch.chdir(git_repo)
    monkeypatch.setenv("EVENT_NAME", "pull_request")
    monkeypatch.setenv("PR_BASE_SHA", base)
    monkeypatch.setenv("PR_HEAD_SHA", git(git_repo, "rev-parse", "HEAD"))
    assert _docker_platforms(gh_output) == ("true", _all_docker_platforms())


@pytest.mark.parametrize(
    ("event_name", "repository", "ref", "target"),
    [
        ("push", UPSTREAM, "refs/heads/master", GHCR),
        ("push", UPSTREAM, "refs/heads/Stable_V5.0", GHCR),
        ("push", UPSTREAM, "refs/tags/v5.0.0", DOCKER_HUB),
        ("push", "someone/qgroundcontrol", "refs/heads/master", ""),
        ("push", "someone/qgroundcontrol", "refs/tags/v5.0.0", ""),
        ("pull_request", UPSTREAM, "refs/pull/1/merge", ""),
        ("workflow_dispatch", UPSTREAM, "refs/heads/master", ""),
        ("workflow_dispatch", UPSTREAM, "refs/tags/v5.0.0", ""),
    ],
)
def test_docker_legs_carry_the_events_push_target(
    event: pytest.MonkeyPatch,
    gh_output: Path,
    event_name: str,
    repository: str,
    ref: str,
    target: str,
) -> None:
    event.setenv("EVENT_NAME", event_name)
    event.setenv("GITHUB_REPOSITORY", repository)
    event.setenv("GITHUB_REF", ref)
    if ref.startswith("refs/tags/"):
        event.setenv("GITHUB_REF_TYPE", "tag")
        event.setattr(detect_changes, "check_release_tag", lambda *_: True)
    _, matrix = _plan(gh_output, "docker")
    assert matrix
    assert {leg["push_target"] for leg in matrix} == {target}

    # docker.yml attested pushed images when `repository == upstream && event == push`; its
    # attest-images job now runs when no leg has an empty push target.
    attests = '"push_target":""' not in read_github_file(gh_output)["matrix"]
    assert attests == (repository == UPSTREAM and event_name == "push")
