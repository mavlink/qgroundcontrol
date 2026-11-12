"""Contracts between the cache actions, their planners, and the workflows that use them."""

from __future__ import annotations

import json
import re

import pytest
from ci.cache import gh_cache_cleanup
from ci.cache.build_cache_artifact import CACHE_ROOTS, is_snapshot_name
from ci.cache.cache_plan import CacheInputs, plan_caches
from ci.cache.ccache_helper import SavePlan
from ci.runner.build_setup import SetupInputs, plan_setup
from common.build_config import load_build_config

from ..._helpers import evaluate_expression, get_step, get_steps, load_action, load_workflow
from .._workflows import local_action, step_groups, uses_action

CACHE_ACTIONS = {"actions/cache", "actions/cache/restore", "actions/cache/save"}


@pytest.fixture
def plan(tmp_path, monkeypatch):
    """Plan the cache action for an iOS Debug job on the default branch, with overrides."""
    (tmp_path / ".github").mkdir()
    (tmp_path / ".github/build-config.json").write_text('{"gstreamer": {"version": "1"}}')
    (tmp_path / "tools/configs").mkdir(parents=True)
    (tmp_path / "tools/configs/ccache.conf").write_text("max_size = 5G\n")
    monkeypatch.setenv("GITHUB_WORKSPACE", str(tmp_path))

    def run(**changes):
        fields = {
            "host": "mac",
            "target": "ios",
            "build_type": "Debug",
            "event": "push",
            "ref_name": "master",
            "run_id": "7",
            "run_attempt": "1",
            "ccache_hash": "a" * 64,
            "moccache_hash": "b" * 64,
            **changes,
        }
        return plan_caches(CacheInputs(**fields), tmp_path)

    return run


def test_cache_action_reads_only_plan_outputs_the_planner_writes(plan) -> None:
    referenced = set(re.findall(r"steps\.plan\.outputs\.(\w+)", json.dumps(load_action("cache"))))
    assert referenced
    assert referenced <= plan().outputs.keys()


@pytest.mark.parametrize("suffix", ["", "device", "simulator", "custom-debug"])
def test_pr_keys_restore_matching_master_matrix_cache(plan, suffix) -> None:
    def keys(scope, build_type="Debug", variant="portable-tests", key_suffix=suffix, config="a"):
        event = {"event": "push", "ref_name": "master"}
        if scope != "shared":
            event = {"event": "pull_request", "ref_name": "42/merge", "pr_number": "42"}
        result = plan(
            build_type=build_type,
            variant=variant,
            key_suffix=key_suffix,
            ccache_hash=config * 64,
            moccache_hash=chr(ord(config) + 1) * 64,
            **event,
        )
        assert result.outputs["moccache_key"].startswith(f"moccache-mac-ios-{scope}-")
        restore = {
            family: result.outputs[f"{family}_restore_keys"].split("\n")
            for family in ("ccache", "moccache", "cpm")
        }
        return result.outputs, result.env, restore

    ordinary, _, _ = keys("pr-42", "Release", key_suffix="")
    _, _, custom_restore = keys("pr-42", key_suffix="custom-debug")
    assert any(ordinary["cpm_key"].startswith(prefix) for prefix in custom_restore["cpm"])

    master, master_env, master_restore = keys("shared")
    pr, pr_env, pr_restore = keys("pr-42")
    artifact, shared_artifact = "QGC_BUILD_CACHE_ARTIFACT_NAME", "QGC_BUILD_CACHE_SHARED_ARTIFACT"
    assert pr_env[shared_artifact] == master_env[artifact]
    # Exact-name artifact lookups must survive build-config, ccache.conf and moccache edits
    reconfigured, reconfigured_env, _ = keys("pr-42", config="d")
    assert reconfigured["ccache_key"] != pr["ccache_key"]
    for name in (artifact, shared_artifact):
        assert reconfigured_env[name] == pr_env[name]
    assert keys("shared", key_suffix="other")[1][artifact] != master_env[artifact]
    assert keys("shared", variant="sanitizers")[1][artifact] != master_env[artifact]
    for other in (keys("shared", "Release")[0], keys("shared", variant="sanitizers")[0]):
        assert other["moccache_key"] != master["moccache_key"]
        assert other["moccache_key"].startswith(master_restore["moccache"][0])
    for family in ("ccache", "moccache", "cpm"):
        assert any(master[f"{family}_key"].startswith(prefix) for prefix in pr_restore[family])
        assert not any(pr[f"{family}_key"].startswith(prefix) for prefix in master_restore[family])
    assert pr["cpm_shared_key"] == master["cpm_key"] == master["cpm_shared_key"]


def test_a_release_tag_plan_skips_every_compiler_and_moc_cache_step(plan) -> None:
    # cache_plan.py only plans: empty keys and two environment flags are all that keep the
    # actions from restoring into, or saving from, a release build.
    def transfers(planned) -> tuple[list[str], list[str]]:
        restoring = {
            "env": planned.env,
            "steps": {"plan": {"outputs": planned.outputs}, "artifact-cache": {"outputs": {}}},
        }
        restores = [
            step["name"]
            for step in get_steps(load_action("cache"))
            if (
                step.get("with", {}).get("path") in CACHE_ROOTS
                or "build_cache_artifact.py" in step.get("run", "")
            )
            and evaluate_expression(step["if"], restoring)
        ]
        # What the save plan's outputs hold when its step is skipped.
        saving = {
            "env": planned.env,
            "steps": {"plan": {"outputs": dict.fromkeys(SavePlan._fields, "")}},
        }
        saves = [
            step["name"]
            for step in get_steps(load_action("save-build-cache"))
            if evaluate_expression(step["if"], saving)
        ]
        return restores, saves

    restores, saves = transfers(plan())
    # The snapshot, then a dependency-cache fallback for each directory it holds.
    assert len(restores) == 1 + len(CACHE_ROOTS)
    assert saves
    assert transfers(plan(ref_type="tag", ref_name="v5.1.5")) == ([], [])


def test_cache_steps_for_one_directory_share_one_path_list() -> None:
    # actions/cache versions an archive by its path list, so a step listing other paths for the
    # same directory could never restore what its counterpart saved.
    by_directory: dict[str, dict[str, str]] = {}
    saved, restored = set(), set()
    for label, _, steps in step_groups():
        for step in steps:
            action = str(step.get("uses", "")).partition("@")[0]
            if action not in CACHE_ACTIONS:
                continue
            path = step["with"]["path"]
            directory = path.splitlines()[0]
            by_directory.setdefault(directory, {})[f"{label}:{step.get('name')}"] = path
            (saved if action.endswith("/save") else restored).add(directory)
    assert saved
    for paths in by_directory.values():
        assert len(set(paths.values())) == 1, paths
    assert saved <= restored


def test_post_job_saves_read_no_inputs_or_step_outputs_of_their_composite_action() -> None:
    # A composite action's nested post steps evaluate their inputs again once its `inputs` and step
    # outputs are gone: actions/cache reads `path` again, setup-gradle every input.
    checked = 0
    for label, _, steps in step_groups(workflows=False):
        for step in steps:
            if uses_action(step, "actions/cache"):
                reread = [step["with"]["path"]]
            elif uses_action(step, "gradle/actions/setup-gradle"):
                reread = list(step["with"].values())
            else:
                continue
            assert not re.search(r"\b(inputs|steps)\.", json.dumps(reread)), (label, step["name"])
            checked += 1
    assert checked


def test_default_branch_cpm_snapshot_is_restored_by_its_exact_key_only() -> None:
    # GitHub searches every key in the PR scope before the default branch, so a scoped
    # restore-key could select an older PR snapshot instead of the exact default-branch one.
    shared = get_step(get_steps(load_action("cache")), "cpm-shared")
    assert "restore-keys" not in shared["with"]


def test_ios_legs_share_the_device_cpm_snapshot(plan) -> None:
    # Only the Release device leg saves CPM sources; a per-leg key suffix would leave the Debug
    # simulator nothing to restore. Build types already separate the compiler and moc caches.
    job = load_workflow("ios.yml")["jobs"]["build"]
    legs = job["strategy"]["matrix"]["include"]
    (setup,) = (step for step in job["steps"] if local_action(step) == "build-setup")
    assert "cache-key-suffix" not in setup["with"]
    for leg in legs:
        outputs = plan_setup(
            SetupInputs("ios", qt_arch=leg["qt_arch"]), load_build_config()
        ).outputs
        assert (outputs["qt_host"], outputs["cache_target"]) == ("mac", "ios")

    planned = [plan(build_type=leg["build_type"]).outputs for leg in legs]
    assert sorted(outputs["cpm_mode"] for outputs in planned) == ["restore", "save"]
    assert len({outputs["cpm_key"] for outputs in planned}) == 1
    for family in ("ccache", "moccache"):
        assert len({outputs[f"{family}_key"] for outputs in planned}) == len(legs)


def test_runs_on_cache_setup_runs_for_every_event_and_fork() -> None:
    # runs-on/action skips itself off RunsOn, so a condition could only keep a fork pull
    # request or another event from the cache setup.
    conditions = [
        step.get("if")
        for _, _, steps in step_groups()
        for step in steps
        if uses_action(step, "runs-on/action")
    ]
    assert conditions
    assert not any(conditions)


def test_only_the_snapshot_upload_takes_a_name_the_cleanup_deletes(plan) -> None:
    # gh_cache_cleanup.py deletes superseded artifacts by their name alone, so a package, SBOM
    # or report uploaded under a snapshot's name would be deleted with them.
    env = plan().env
    for variable in ("QGC_BUILD_CACHE_ARTIFACT_NAME", "QGC_BUILD_CACHE_SHARED_ARTIFACT"):
        assert is_snapshot_name(env[variable])
    uploads = [
        (label, str(step["with"]["name"]))
        for label, _, steps in step_groups()
        for step in steps
        if uses_action(step, "actions/upload-artifact")
    ]
    snapshot = "${{ env.QGC_BUILD_CACHE_ARTIFACT_NAME }}"
    assert [label for label, name in uploads if name == snapshot] == ["actions/save-build-cache"]
    # No other name becomes a snapshot's when its expressions yield a digest.
    for label, name in uploads:
        if name != snapshot:
            assert not is_snapshot_name(re.sub(r"\$\{\{.*?\}\}", "0" * 64, name)), label


def test_cache_cleanup_protects_the_default_branch_caches_the_plan_saves(plan) -> None:
    # Above its storage threshold the cleanup evicts every cache but the newest of each family
    # it protects, so a key prefix that changes in the plan must change in the cleanup too.
    outputs = plan().outputs
    protected = re.compile(gh_cache_cleanup.DEFAULT_PROTECT)
    for name in ("ccache_key", "moccache_key", "cpm_key", "gst_key"):
        assert protected.search(outputs[name]), outputs[name]


def test_cache_cleanup_dispatch_defaults_match_the_script() -> None:
    inputs = load_workflow("cache-cleanup.yml")["on"]["workflow_dispatch"]["inputs"]
    assert {name: spec.get("default", "") for name, spec in inputs.items()} == (
        gh_cache_cleanup.DISPATCH_INPUT_DEFAULTS
    )
