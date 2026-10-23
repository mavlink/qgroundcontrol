"""Tests for detect_changes.py."""

from __future__ import annotations

import re
import string
from typing import TYPE_CHECKING

import pytest
from ci.planning import detect_changes
from ci.planning.detect_changes import (
    build_patterns,
    detect_builds,
    get_changed_files,
    has_relevant_changes,
    workflow_name_for_platform,
)
from hypothesis import given
from hypothesis import strategies as st
from qgc_tools.release_tag_check import GitQueryError

from .._helpers import (
    REPO_ROOT,
    commit_files,
    git,
    load_workflow,
    pr_merge_checkout,
    workflow_names,
)

if TYPE_CHECKING:
    from pathlib import Path

WORKFLOWS_DIR = REPO_ROOT / ".github" / "workflows"
REUSABLE_DETECTION = "./.github/workflows/_detect-changes.yml"
_EVENT_VARS = (
    "EVENT_NAME",
    "PR_BASE_SHA",
    "PR_HEAD_SHA",
)


def test_rename_out_of_watched_dir_is_relevant(
    monkeypatch: pytest.MonkeyPatch, git_repo: Path
) -> None:
    base = commit_files(git_repo, {"src/Vehicle.cc": "int vehicle = 1;\n" * 20}, "base")
    (git_repo / "docs").mkdir()
    git(git_repo, "mv", "src/Vehicle.cc", "docs/Vehicle.cc")
    git(git_repo, "commit", "-qm", "move")
    head = git(git_repo, "rev-parse", "HEAD")

    monkeypatch.chdir(git_repo)
    monkeypatch.setenv("PR_BASE_SHA", base)
    monkeypatch.setenv("PR_HEAD_SHA", head)

    changed = get_changed_files()
    assert changed is not None
    assert sorted(changed) == ["docs/Vehicle.cc", "src/Vehicle.cc"]
    assert has_relevant_changes(changed, "linux")


@pytest.fixture
def event(monkeypatch: pytest.MonkeyPatch, git_repo: Path) -> pytest.MonkeyPatch:
    """Clear the workflow's event inputs and run inside a one-commit repository."""
    for name in _EVENT_VARS:
        monkeypatch.delenv(name, raising=False)
    commit_files(git_repo, {"README.md": "base\n"}, "base")
    monkeypatch.chdir(git_repo)
    return monkeypatch


def _should_build() -> bool:
    return detect_builds(["linux"]).builds["linux"]


@pytest.mark.parametrize(("path", "expected"), [("src/Vehicle.cc", True), ("docs/a.md", False)])
def test_pull_request_builds_only_relevant_changes(
    event: pytest.MonkeyPatch, tmp_path: Path, path: str, expected: bool
) -> None:
    event.setenv("EVENT_NAME", "pull_request")
    event.setenv("GITHUB_REF_TYPE", "branch")
    event.setenv("PR_BASE_SHA", git(tmp_path, "rev-parse", "HEAD"))
    event.setenv("PR_HEAD_SHA", commit_files(tmp_path, {path: path}, path))
    assert _should_build() is expected


@pytest.mark.parametrize("clone", ["full", "shallow"])
def test_pull_request_behind_its_base_detects_only_its_own_changes(
    event: pytest.MonkeyPatch, tmp_path: Path, clone: str
) -> None:
    pr = pr_merge_checkout(
        tmp_path / "pr",
        base={"docs/index.md": "# Index\n"},
        newer_base={"src/Vehicle.cc": "int vehicle;\n"},
        pr={"docs/index.md": "# Changed\n"},
    )
    # The depth-2 clone is what the detection jobs check out for a pull request.
    event.chdir(getattr(pr, clone))
    event.setenv("EVENT_NAME", "pull_request")
    # A two-dot diff from the event's base SHA would also list the base branch's src/ change.
    event.setenv("PR_BASE_SHA", pr.base_tip)
    event.setenv("PR_HEAD_SHA", pr.pr_head)
    assert get_changed_files() == ["docs/index.md"]
    assert _should_build() is False


def test_pull_request_with_unknown_diff_builds(
    event: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    event.setenv("EVENT_NAME", "pull_request")
    event.setenv("PR_BASE_SHA", "f" * 40)
    event.setenv("PR_HEAD_SHA", "e" * 40)
    assert _should_build() is True
    assert "::warning::Unable to compute changed files" in capsys.readouterr().out


@pytest.mark.parametrize("event_name", ["push", "merge_group", "workflow_dispatch"])
def test_other_branch_events_build_without_diffing(
    event: pytest.MonkeyPatch, event_name: str
) -> None:
    event.setenv("EVENT_NAME", event_name)
    event.setenv("GITHUB_REF_TYPE", "branch")

    def unexpected() -> None:
        raise AssertionError("non-PR events must not diff")

    event.setattr(detect_changes, "get_changed_files", unexpected)
    assert _should_build() is True


@pytest.mark.parametrize("event_name", ["push", "workflow_dispatch"])
@pytest.mark.parametrize(
    ("ref_name", "stable_branch", "expected"),
    [
        ("v5.1.4", True, True),
        ("v5.1.4", False, False),
        ("v5.2.0-dev", True, False),
    ],
)
def test_tags_build_only_release_tags_on_stable_branches(
    event: pytest.MonkeyPatch,
    tmp_path: Path,
    event_name: str,
    ref_name: str,
    stable_branch: bool,
    expected: bool,
) -> None:
    head = commit_files(tmp_path, {"src/Vehicle.cc": "change"}, "change")
    git(tmp_path, "update-ref", "refs/remotes/origin/master", head)
    if stable_branch:
        git(tmp_path, "update-ref", "refs/remotes/origin/Stable_V5.1", head)
    event.setenv("EVENT_NAME", event_name)
    event.setenv("GITHUB_REF_TYPE", "tag")
    event.setenv("GITHUB_REF_NAME", ref_name)
    assert _should_build() is expected


def test_tag_fails_when_git_cannot_answer(event: pytest.MonkeyPatch, tmp_path: Path) -> None:
    outside = tmp_path / "outside"
    outside.mkdir()
    event.chdir(outside)
    event.setenv("GIT_CEILING_DIRECTORIES", str(tmp_path))
    event.setenv("EVENT_NAME", "push")
    event.setenv("GITHUB_REF_TYPE", "tag")
    event.setenv("GITHUB_REF_NAME", "v5.1.4")
    with pytest.raises(GitQueryError):
        _should_build()


def test_detect_builds_returns_pull_request_diff_per_platform(
    event: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    event.setenv("EVENT_NAME", "pull_request")
    event.setenv("PR_BASE_SHA", git(tmp_path, "rev-parse", "HEAD"))
    event.setenv(
        "PR_HEAD_SHA", commit_files(tmp_path, {"android/AndroidManifest.xml": "change"}, "change")
    )
    detection = detect_builds(["docker-linux", "docker-android"])
    assert detection.builds == {"docker-linux": False, "docker-android": True}
    assert detection.changed_files == ["android/AndroidManifest.xml"]


def _single(steps: list[dict], predicate) -> dict:
    (step,) = [s for s in steps if predicate(s)]
    return step


def _checkout(steps: list[dict]) -> dict:
    return _single(steps, lambda s: str(s.get("uses", "")).startswith("actions/checkout@"))


def test_reusable_workflow_runs_one_unconditional_detection() -> None:
    workflow = load_workflow("_detect-changes.yml")
    assert set(workflow["on"]["workflow_call"]["outputs"]) == {"should_build", "matrix"}
    job = workflow["jobs"]["detect"]
    assert job["outputs"] == {
        "should_build": "${{ steps.detect.outputs.should_build }}",
        "matrix": "${{ steps.detect.outputs.matrix }}",
    }
    steps = job["steps"]
    run = _single(steps, lambda s: "run" in s)
    assert run["id"] == "detect"
    assert 'plan_platform.py" --platform' in run["run"]
    assert "if" not in _checkout(steps)
    assert "if" not in run
    assert not any(str(s.get("uses", "")).startswith("./") for s in steps)


def test_detection_job_checks_out_and_passes_what_detect_builds_reads() -> None:
    steps = load_workflow("_detect-changes.yml")["jobs"]["detect"]["steps"]
    checkout = _checkout(steps)["with"]
    entries = set(checkout["sparse-checkout"].split())
    assert {
        "deploy/docker/variants.json",
        "tools/ci/planning/detect_changes.py",
        "tools/ci/planning/plan_platform.py",
        "tools/qgc_tools",
    } <= entries
    # Pull requests diff the merge commit against its first parent; tags need full history.
    assert checkout["fetch-depth"] == (
        "${{ github.event_name == 'pull_request' && 2 || github.ref_type != 'tag' && 1 || 0 }}"
    )
    env = _single(steps, lambda s: "run" in s)["env"]
    assert env["EVENT_NAME"] == "${{ github.event_name }}"
    assert env["PR_BASE_SHA"] == "${{ github.event.pull_request.base.sha }}"
    assert env["PR_HEAD_SHA"] == "${{ github.event.pull_request.head.sha }}"


# Workflows whose build legs come from the planner's matrix output.
MATRIX_WORKFLOWS = {"android.yml", "docker.yml"}


def test_reusable_detection_callers_consume_should_build_and_planned_matrices() -> None:
    callers = set()
    for workflow in workflow_names():
        jobs = load_workflow(workflow).get("jobs") or {}
        text = (WORKFLOWS_DIR / workflow).read_text(encoding="utf-8")
        for name, job in jobs.items():
            if job.get("uses") == REUSABLE_DETECTION:
                callers.add(workflow)
                consumed = set(re.findall(rf"needs\.{re.escape(name)}\.outputs\.(\w+)", text))
                expected = (
                    {"should_build", "matrix"} if workflow in MATRIX_WORKFLOWS else {"should_build"}
                )
                assert consumed == expected, workflow
    assert {"linux.yml", "windows.yml", "macos.yml", "ios.yml", *MATRIX_WORKFLOWS} <= callers


@pytest.mark.parametrize(
    ("workflow", "platform"), [("android.yml", "android"), ("docker.yml", "docker")]
)
def test_matrix_workflows_build_the_planned_legs(workflow: str, platform: str) -> None:
    jobs = load_workflow(workflow)["jobs"]
    assert jobs["changes"] == {"uses": REUSABLE_DETECTION, "with": {"platform": platform}}
    assert jobs["build"]["needs"] == "changes"
    assert jobs["build"]["if"] == "needs.changes.outputs.should_build == 'true'"
    assert jobs["build"]["strategy"]["matrix"] == {
        "include": "${{ fromJSON(needs.changes.outputs.matrix) }}"
    }


@pytest.mark.parametrize(
    "platform",
    [
        "linux",
        "windows",
        "macos",
        "ios",
        "android",
        "docker-linux",
        "docker-android",
    ],
)
@pytest.mark.parametrize(
    "filename",
    [
        "CMakePresets.json",
        ".github/workflows/_detect-changes.yml",
        "tools/moccache/core.py",
        "tools/common/config.py",
        "tools/configs/ccache.conf",
        "tools/setup/install_qt.py",
        "tools/setup/qt_installer/install.py",
        "tools/setup/install_dependencies/_common.py",
    ],
)
def test_shared_build_inputs_trigger_all_platforms(platform: str, filename: str) -> None:
    assert has_relevant_changes([filename], platform)


@pytest.mark.parametrize("platform", detect_changes.PLATFORMS)
@pytest.mark.parametrize(
    "filename",
    [
        "Doxyfile",
        "cliff.toml",
        "editorconfig-checker.json",
        "gersemirc.yaml",
        "lychee.toml",
        "yamlfmt.yaml",
    ],
)
def test_non_build_configs_do_not_trigger_builds(platform: str, filename: str) -> None:
    assert not has_relevant_changes([f"tools/configs/{filename}"], platform)


@pytest.mark.parametrize(
    "filename", ["variants.json", "_variants.py", "run_docker.py", "docker_helper.py"]
)
def test_variant_inputs_trigger_docker_builds(filename: str) -> None:
    for platform in ("docker-linux", "docker-android"):
        assert has_relevant_changes([f"deploy/docker/{filename}"], platform)
    assert not has_relevant_changes([f"deploy/docker/{filename}"], "linux")


class TestWorkflowNameForPlatform:
    def test_regular_platform(self) -> None:
        assert workflow_name_for_platform("linux") == "linux"
        assert workflow_name_for_platform("windows") == "windows"
        assert workflow_name_for_platform("macos") == "macos"

    def test_docker_prefix(self) -> None:
        assert workflow_name_for_platform("docker-linux") == "docker"
        assert workflow_name_for_platform("docker-android") == "docker"


class TestBuildPatterns:
    def test_common_patterns_present(self) -> None:
        patterns = build_patterns("linux")
        pattern_strs = [p.pattern for p in patterns]
        assert any("^src/" in p for p in pattern_strs)
        assert any("CMakeLists" in p for p in pattern_strs)

    def test_platform_workflow_pattern(self) -> None:
        patterns = build_patterns("linux")
        pattern_strs = [p.pattern for p in patterns]
        assert any("linux\\.yml" in p for p in pattern_strs)

    def test_docker_workflow_pattern(self) -> None:
        patterns = build_patterns("docker-linux")
        pattern_strs = [p.pattern for p in patterns]
        assert any("docker\\.yml" in p for p in pattern_strs)

    def test_android_extra_pattern(self) -> None:
        patterns = build_patterns("android")
        pattern_strs = [p.pattern for p in patterns]
        assert any("^android/" in p for p in pattern_strs)

    def test_deploy_pattern_uses_platform(self) -> None:
        patterns = build_patterns("linux")
        pattern_strs = [p.pattern for p in patterns]
        assert any("deploy/linux/" in p for p in pattern_strs)


class TestHasRelevantChanges:
    def test_src_change_triggers_any_platform(self) -> None:
        assert has_relevant_changes(["src/Vehicle.cc"], "linux")
        assert has_relevant_changes(["src/Vehicle.cc"], "windows")
        assert has_relevant_changes(["src/Vehicle.cc"], "android")

    def test_cmakelists_triggers(self) -> None:
        assert has_relevant_changes(["CMakeLists.txt"], "linux")

    def test_cmake_dir_triggers(self) -> None:
        assert has_relevant_changes(["cmake/FindFoo.cmake"], "macos")

    def test_qrc_triggers(self) -> None:
        assert has_relevant_changes(["resources/qgc.qrc"], "linux")

    def test_unrelated_file_does_not_trigger(self) -> None:
        assert not has_relevant_changes(["README.md"], "linux")
        assert not has_relevant_changes(["docs/guide.md"], "windows")

    def test_workflow_file_triggers_own_platform(self) -> None:
        assert has_relevant_changes([".github/workflows/linux.yml"], "linux")
        assert not has_relevant_changes([".github/workflows/linux.yml"], "windows")

    def test_actions_dir_triggers_all(self) -> None:
        assert has_relevant_changes([".github/actions/qt-install/action.yml"], "linux")
        assert has_relevant_changes([".github/actions/qt-install/action.yml"], "android")

    def test_build_config_triggers_all(self) -> None:
        assert has_relevant_changes([".github/build-config.json"], "ios")

    def test_android_dir_only_triggers_android(self) -> None:
        assert has_relevant_changes(["android/AndroidManifest.xml"], "android")
        assert not has_relevant_changes(["android/AndroidManifest.xml"], "linux")

    def test_docker_linux_patterns(self) -> None:
        assert has_relevant_changes(["deploy/docker/Dockerfile"], "docker-linux")
        assert has_relevant_changes(["deploy/docker/install_sysroot_aarch64.py"], "docker-linux")
        assert has_relevant_changes(["deploy/docker/entrypoint.sh"], "docker-linux")
        assert has_relevant_changes(["deploy/docker/run_docker.py"], "docker-linux")
        assert has_relevant_changes(["deploy/docker/lib/setup-base.sh"], "docker-linux")
        assert has_relevant_changes(["deploy/linux/AppImage.sh"], "docker-linux")
        assert has_relevant_changes(
            ["tools/ci/packaging/validate_native_package.py"], "docker-linux"
        )
        assert not has_relevant_changes(["deploy/docker/Dockerfile"], "linux")

    def test_docker_android_patterns(self) -> None:
        assert has_relevant_changes(["deploy/docker/Dockerfile"], "docker-android")
        assert has_relevant_changes(["deploy/docker/entrypoint.py"], "docker-android")
        assert has_relevant_changes(["android/build.gradle"], "docker-android")

    def test_setup_patterns_linux(self) -> None:
        assert has_relevant_changes(["tools/setup/install_dependencies/_debian.py"], "linux")
        assert has_relevant_changes(["tools/setup/foo-debian.sh"], "linux")
        assert not has_relevant_changes(["tools/setup/foo-windows.ps1"], "linux")

    def test_setup_patterns_windows(self) -> None:
        assert has_relevant_changes(["tools/setup/foo-windows.ps1"], "windows")
        assert not has_relevant_changes(["tools/setup/foo-debian.sh"], "windows")

    def test_setup_patterns_android_matches_all(self) -> None:
        assert has_relevant_changes(["tools/setup/anything.py"], "android")

    def test_setup_patterns_ios(self) -> None:
        assert has_relevant_changes(["tools/setup/foo-ios.sh"], "ios")
        assert has_relevant_changes(["tools/setup/foo-macos.sh"], "ios")
        assert not has_relevant_changes(["tools/setup/foo-debian.sh"], "ios")

    def test_empty_files_list(self) -> None:
        assert not has_relevant_changes([], "linux")

    def test_empty_strings_ignored(self) -> None:
        assert not has_relevant_changes(["", ""], "linux")

    def test_test_dir_triggers(self) -> None:
        assert has_relevant_changes(["test/UnitTest.cc"], "linux")

    def test_deploy_platform_triggers(self) -> None:
        assert has_relevant_changes(["deploy/macos/Info.plist"], "macos")
        assert not has_relevant_changes(["deploy/macos/Info.plist"], "linux")

    def test_scripts_dir_triggers_all(self) -> None:
        assert has_relevant_changes(["tools/ci/compile/foo.py"], "windows")

    def test_docker_workflow_triggers_docker_platforms(self) -> None:
        assert has_relevant_changes([".github/workflows/docker.yml"], "docker-linux")
        assert has_relevant_changes([".github/workflows/docker.yml"], "docker-android")
        assert not has_relevant_changes([".github/workflows/docker.yml"], "linux")


_PLATFORM = st.sampled_from(detect_changes.PLATFORMS)
_RELATIVE_PATH = st.lists(
    st.text(alphabet=string.ascii_letters + string.digits + "._-", min_size=1, max_size=12),
    min_size=1,
    max_size=4,
).map("/".join)
_CHANGED_FILE = st.one_of(
    st.just(""),
    _RELATIVE_PATH,
    st.builds(
        "{}/{}".format, st.sampled_from(["src", "docs", "tools/tests", "android"]), _RELATIVE_PATH
    ),
)


class TestRelevanceProperties:
    @given(files=st.lists(_CHANGED_FILE, max_size=8), platform=_PLATFORM)
    def test_a_diff_is_relevant_when_any_one_file_is(self, files: list[str], platform: str) -> None:
        relevant = has_relevant_changes(files, platform)
        assert relevant == any(has_relevant_changes([file], platform) for file in files)
        assert has_relevant_changes(files[::-1], platform) == relevant

    @given(path=_RELATIVE_PATH, platform=_PLATFORM)
    def test_shared_inputs_build_every_platform(self, path: str, platform: str) -> None:
        for root in ("src", "cmake", "test", "resources", ".github/actions", "tools/ci"):
            assert has_relevant_changes([f"{root}/{path}"], platform)

    @given(path=_RELATIVE_PATH, platform=_PLATFORM)
    def test_platform_inputs_build_their_platform(self, path: str, platform: str) -> None:
        workflow = workflow_name_for_platform(platform)
        assert has_relevant_changes([f"deploy/{platform}/{path}"], platform)
        assert has_relevant_changes([f".github/workflows/{workflow}.yml"], platform)

    @given(path=_RELATIVE_PATH.filter(lambda path: not path.endswith(".qrc")), platform=_PLATFORM)
    def test_docs_and_tool_tests_never_build(self, path: str, platform: str) -> None:
        assert not has_relevant_changes([f"docs/{path}", f"tools/tests/{path}", ""], platform)

    @pytest.mark.parametrize("platform", detect_changes.PLATFORMS)
    def test_every_pattern_is_anchored(self, platform: str) -> None:
        assert all(pattern.pattern.startswith("^") for pattern in build_patterns(platform))
