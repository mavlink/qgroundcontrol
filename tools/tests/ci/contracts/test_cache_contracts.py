"""Static contracts for CI cache configuration."""

from __future__ import annotations

import pytest
from ci.cache import gh_cache_cleanup
from ci.cache.cache_plan import CacheInputs, plan_caches
from ci.runner.build_setup import SetupInputs, plan_setup
from common.build_config import load_build_config

from ..._helpers import (
    REPO_ROOT,
    get_step,
    get_steps,
    load_action,
    load_workflow,
    load_yaml,
    workflow_names,
)


def _read(path: str) -> str:
    return (REPO_ROOT / path).read_text(encoding="utf-8")


def _setup_uv() -> dict:
    (step,) = (
        step
        for step in get_steps(load_action("setup-python"))
        if step.get("uses", "").startswith("astral-sh/setup-uv@")
    )
    return step


def _cache_steps() -> list[dict]:
    return get_steps(load_action("cache"))


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


def test_build_caches_use_rolling_platform_keys(plan) -> None:
    steps = _cache_steps()
    named = {step["name"]: step for step in steps}
    for name, family in (
        ("Restore Build Cache (ccache)", "ccache"),
        ("Restore moc output (moccache)", "moccache"),
    ):
        assert named[name]["with"]["key"] == f"${{{{ steps.plan.outputs.{family}_key }}}}"
        restore_keys = f"${{{{ steps.plan.outputs.{family}_restore_keys }}}}"
        assert named[name]["with"]["restore-keys"] == restore_keys
    planner = next(step for step in steps if step.get("id") == "plan")
    assert planner["env"]["INPUT_KEY_SUFFIX"] == "${{ inputs.key-suffix }}"

    device, simulator = plan(key_suffix="device"), plan(key_suffix="simulator")
    retried = plan(key_suffix="device", run_attempt="2")
    for family in ("ccache", "moccache"):
        key = device.outputs[f"{family}_key"]
        assert key.endswith("-7-1")
        assert retried.outputs[f"{family}_key"] == f"{key.removesuffix('-1')}-2"
        assert key != simulator.outputs[f"{family}_key"]
        assert device.env[f"QGC_{family.upper()}_KEY"] == key
    assert device.outputs["cpm_key"] != simulator.outputs["cpm_key"]


def test_plan_step_receives_the_action_inputs() -> None:
    action = load_action("cache")
    planner = action["runs"]["steps"][0]
    assert planner["id"] == "plan"
    forwarded = {
        f"INPUT_{name.upper().replace('-', '_')}": f"${{{{ inputs.{name} }}}}"
        for name in action["inputs"]
        if name != "install-tools"
    }
    assert forwarded.items() <= planner["env"].items()
    assert planner["env"]["PR_NUMBER"] == "${{ github.event.pull_request.number }}"
    assert not {"EVENT_NAME", "PR_REPO", "THIS_REPO"} & planner["env"].keys()


def test_compiler_archives_use_workspace_relative_paths() -> None:
    steps = get_steps(load_action("cache"))
    paths = [step["with"]["path"] for step in steps if "uses" in step]
    saves = get_steps(load_action("save-build-cache"))
    paths += [step["with"]["path"] for step in saves if "uses" in step]
    assert paths.count(".cache/ccache") == 2
    assert paths.count(".cache/moccache") == 2


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


def test_moccache_is_cross_platform_and_content_versioned(plan) -> None:
    action = _read(".github/actions/cache/action.yml")
    build = _read(".github/actions/cmake-build/action.yml")
    planner = next(step for step in _cache_steps() if step.get("id") == "plan")

    assert "runner.os != 'Windows'" not in action
    assert "runner.os != 'Windows'" not in build
    assert planner["env"]["MOC_CACHE_HASH"] == (
        "${{ hashFiles('.github/build-config.json', 'cmake/Helpers.cmake', 'tools/moccache/*.py', "
        "'tools/moccache_launcher.py') }}"
    )
    outputs = plan(event="pull_request", ref_name="1/merge", pr_number="1").outputs
    assert outputs["moccache_key"].startswith(f"moccache-mac-ios-pr-1-{'b' * 64}-")
    assert outputs["moccache_restore_keys"].split("\n")[-1] == "moccache-mac-ios-shared-"


def test_qt_cache_stores_only_architecture_tree() -> None:
    steps = get_steps(load_action("qt-install"))
    plan = next(step for step in steps if step.get("id") == "plan")
    cached = [step for step in steps if step.get("uses", "").startswith("actions/cache/")]

    # install_qt.py plan reports `.qt/Qt/<version>/<arch dir>` relative to the workspace.
    assert '--outdir "${GITHUB_WORKSPACE}/.qt/Qt"' in plan["run"]
    assert [step["with"]["path"] for step in cached] == ["${{ steps.plan.outputs.cache_path }}"] * 3
    # install_qt.py plan computes the key (test_install_qt.py pins it to the former expression).
    assert [step["with"]["key"] for step in cached] == ["${{ steps.plan.outputs.key }}"] * 3


def test_qt_install_prefers_compatible_preinstalled_sdk() -> None:
    action = load_action("qt-install")
    steps = action["runs"]["steps"]
    plan = next(step for step in steps if step.get("id") == "plan")
    ensure = next(step for step in steps if step.get("id") == "qt")

    # Plan, cache restore, reuse-or-install, and the pre-save lookup and save.
    assert len(steps) == 5
    assert "--preinstalled-prefix" in plan["run"]
    # The plan carries the preinstalled root to ensure.
    assert '--plan "${PLAN}"' in ensure["run"]
    assert "if" not in ensure
    assert action["outputs"]["qt_root_dir"]["value"] == "${{ steps.qt.outputs.qt_root_dir }}"
    restore, *saves = (step for step in steps if step.get("uses", "").startswith("actions/cache/"))
    assert restore["if"] == "steps.plan.outputs.preinstalled_root == ''"
    # `save` is false when a preinstalled SDK is reused (test_install_qt.py).
    for step in saves:
        assert step["if"].startswith("steps.plan.outputs.save == 'true' && ")


def test_dependency_caches_use_shared_backend() -> None:
    (cmake,) = (
        step
        for step in get_steps(load_action("build-setup"))
        if step.get("uses", "").startswith("lukka/get-cmake@")
    )
    # Copilot sessions skip it: they cannot save (test_build_tools_contract.py).
    assert cmake["with"]["useCloudCache"] == "${{ github.event_name != 'dynamic' }}"
    assert cmake["with"]["useLocalCache"] is False
    detect = get_step(get_steps(load_action("setup-python")), "Detect Actions cache backend")
    assert "tools/ci/cache/cache_backend.py" in detect["with"]["script"]
    uv = _setup_uv()
    assert uv["with"]["cache-python"] == "${{ env.QGC_ACTIONS_CACHE_BACKEND == 's3' }}"
    assert uv["with"]["cache-dependency-glob"] == "tools/uv.lock"


def test_nested_cache_save_inputs_survive_post_job_cleanup() -> None:
    # Post-job saves evaluate their inputs again, after nested composite step outputs are gone.
    cpm = "${{ env.CPM_CACHE_PATH }}/*"
    steps = get_steps(load_action("cache"))
    caches = [step for step in steps if step.get("uses", "").startswith("actions/cache")]
    assert [step["name"] for step in caches if cpm in step["with"]["path"].splitlines()] == [
        "Restore default-branch CPM snapshot",
        "Cache CPM Modules (restore and save)",
        "Cache CPM Modules (restore only)",
    ]
    assert not [step["name"] for step in caches if "steps." in step["with"]["path"]]
    assert _setup_uv()["with"]["enable-cache"] == "${{ env.QGC_ACTIONS_CACHE_BACKEND == 's3' }}"


def test_ios_legs_share_the_device_cpm_snapshot(plan) -> None:
    # Only the Release device leg saves CPM sources; a per-leg key suffix would leave the Debug
    # simulator nothing to restore. Build types already separate the compiler and moc caches.
    job = load_workflow("ios.yml")["jobs"]["build"]
    legs = job["strategy"]["matrix"]["include"]
    assert sorted(leg["build_type"] for leg in legs) == ["Debug", "Release"]
    (setup,) = (
        step for step in job["steps"] if step.get("uses") == "./.github/actions/build-setup"
    )
    assert "cache-key-suffix" not in setup["with"]
    steps = get_steps(load_action("build-setup"))
    (cache,) = (step for step in steps if step.get("uses") == "./.github/actions/cache")

    # One cache setup serves every mode; iOS keeps its former mac/ios cache identity.
    assert cache["if"] == "inputs.build-cache == 'true'"
    assert cache["with"]["key-suffix"] == "${{ inputs.cache-key-suffix }}"
    assert cache["with"]["host"] == "${{ steps.plan.outputs.qt_host }}"
    assert cache["with"]["target"] == "${{ steps.plan.outputs.cache_target }}"
    for arch in (leg["qt_arch"] for leg in legs):
        outputs = plan_setup(SetupInputs("ios", qt_arch=arch), load_build_config()).outputs
        assert (outputs["qt_host"], outputs["cache_target"]) == ("mac", "ios")

    device, simulator = plan(build_type="Release"), plan(build_type="Debug")
    assert (device.outputs["cpm_mode"], simulator.outputs["cpm_mode"]) == ("save", "restore")
    assert simulator.outputs["cpm_key"] == device.outputs["cpm_key"]
    for family in ("ccache", "moccache"):
        assert simulator.outputs[f"{family}_key"] != device.outputs[f"{family}_key"]


def test_only_jobs_without_qgc_sources_skip_the_build_caches() -> None:
    # build-gstreamer checks out only .github and tools: its CPM fingerprint would be wrong.
    assert load_action("build-setup")["inputs"]["build-cache"]["default"] == "true"
    skipping = {
        workflow
        for workflow in workflow_names()
        for job in (load_workflow(workflow).get("jobs") or {}).values()
        for step in job.get("steps") or []
        if step.get("uses") == "./.github/actions/build-setup"
        and (step.get("with") or {}).get("build-cache", "true") != "true"
    }
    assert skipping == {"build-gstreamer.yml"}


def test_compiler_tools_install_in_one_step_for_every_runner() -> None:
    # ccache_helper.py install picks the OS and runner arch, adds mold on Linux, and writes
    # PATH/QGC_CACHE_PROGRAM itself (covered by test_ccache_helper.py).
    action = _read(".github/actions/cache/action.yml")
    steps = get_steps(load_action("cache"))
    (install,) = (step for step in steps if "ccache_helper.py" in step.get("run", ""))
    assert install["name"] == "Install compiler tools"
    assert install["if"] == "inputs.install-tools == 'true'"
    assert install["run"].endswith('/tools/ci/cache/ccache_helper.py" install')
    assert "env" not in install
    assert "GITHUB_PATH" not in action
    assert "GITHUB_ENV" not in action


def test_link_cache_has_restore_and_save_with_fork_read_only_policy() -> None:
    steps = get_steps(load_workflow("check-links.yml"), "check-links")
    restore, save = (step for step in steps if step.get("uses", "").startswith("actions/cache/"))
    assert restore["uses"].startswith("actions/cache/restore@")
    assert save["uses"].startswith("actions/cache/save@")
    assert restore["with"]["path"] == save["with"]["path"] == ".lycheecache"
    assert save["with"]["key"] == f"${{{{ steps.{restore['id']}.outputs.cache-primary-key }}}}"
    assert "github.event.pull_request.head.repo.full_name == github.repository" in save["if"]


@pytest.mark.parametrize(
    "workflow",
    ["linux", "windows", "android", "docker", "vm-builds"],
)
def test_runs_on_cache_setup_includes_fork_pull_requests(workflow: str) -> None:
    jobs = load_workflow(f"{workflow}.yml")["jobs"]
    cache_steps = [
        step
        for job in jobs.values()
        for step in job.get("steps", [])
        if step.get("uses") == "runs-on/action@v2"
    ]
    assert cache_steps
    # The action skips itself off RunsOn, so it needs no runner or fork condition.
    for step in cache_steps:
        assert "if" not in step


def test_windows_fork_prs_install_runson_build_tools() -> None:
    workflow = load_workflow("windows.yml")
    install = next(
        step
        for step in workflow["jobs"]["build"]["steps"]
        if step.get("name") == "Install Visual Studio Build Tools"
    )
    # Every RunsOn leg needs the tools; GitHub-hosted legs, including forks and native arm64, have them.
    assert install["if"] == "runner.environment != 'github-hosted'"
    assert install["with"] == {"msvc-arm64": "${{ matrix.variant == 'arm64-cross' }}"}
    job = workflow["jobs"]["build"]
    route = " ".join(job["runs-on"].split())
    assert route.startswith(
        "${{ github.repository_owner == 'mavlink' && matrix.variant != 'arm64' && "
    )
    assert route.endswith("|| matrix.hosted_runner }}")
    hosted = {leg["variant"]: leg["hosted_runner"] for leg in job["strategy"]["matrix"]["include"]}
    assert hosted == {
        "amd64": "windows-2022",
        "arm64": "windows-11-arm",
        "arm64-cross": "windows-2022",
    }


def test_manual_linux_reuses_coverage_build_for_excluded_tests() -> None:
    workflow = load_workflow("linux.yml")
    job = workflow["jobs"]["debug-validation"]
    assert job["strategy"]["matrix"]["mode"] == ["coverage", "sanitizers"]
    steps = job["steps"]
    excluded = next(step for step in steps if step.get("id") == "excluded")
    assert excluded["with"]["include-labels"] == "Network|Flaky"
    assert excluded["with"]["exclude-labels"] == ""
    assert "steps.build.outcome == 'success'" in excluded["if"]
    assert "github.event_name == 'workflow_dispatch'" in excluded["if"]
    assert sum(step.get("uses") == "./.github/actions/cmake-build" for step in steps) == 1


@pytest.mark.parametrize("platform", ["docker", "linux"])
def test_application_builds_have_no_scheduled_trigger(platform: str) -> None:
    workflow = load_workflow(f"{platform}.yml")
    assert "schedule" not in workflow["on"]
    assert {"push", "pull_request", "workflow_dispatch"} <= workflow["on"].keys()
    if platform == "linux":
        assert "merge_group" in workflow["on"]


def test_cache_cleanup_runs_every_six_hours() -> None:
    assert load_workflow("cache-cleanup.yml")["on"]["schedule"] == [{"cron": "17 */6 * * *"}]


def test_cache_cleanup_handles_every_trigger_in_one_job() -> None:
    workflow = load_workflow("cache-cleanup.yml")
    assert set(workflow["on"]) == {"schedule", "pull_request_target", "workflow_dispatch"}
    assert workflow["on"]["pull_request_target"] == {"types": ["closed"]}
    (job,) = workflow["jobs"].values()
    assert job["permissions"] == {"contents": "read", "actions": "write"}
    assert job["timeout-minutes"] == "${{ github.event_name == 'pull_request_target' && 5 || 30 }}"
    checkout = next(step for step in job["steps"] if "actions/checkout@" in step.get("uses", ""))
    # pull_request_target must never check out PR code.
    assert "ref" not in checkout["with"]
    (cleanup,) = (step for step in job["steps"] if "gh_cache_cleanup.py" in step.get("run", ""))
    assert cleanup["run"] == "python3 tools/ci/cache/gh_cache_cleanup.py"
    # gh_cache_cleanup.event_defaults() reads every trigger's settings from the event itself
    # (behavior tests in test_gh_cache_cleanup.py); the job passes only the repository and token.
    assert cleanup["env"] == {
        "GH_TOKEN": "${{ github.token }}",
        "GH_REPO": "${{ github.repository }}",
    }


def test_cache_cleanup_dispatch_defaults_match_the_script() -> None:
    inputs = load_workflow("cache-cleanup.yml")["on"]["workflow_dispatch"]["inputs"]
    assert {name: spec.get("default", "") for name, spec in inputs.items()} == (
        gh_cache_cleanup.DISPATCH_INPUT_DEFAULTS
    )
    assert inputs["mode"]["options"] == ["prune", "wipe"]


def test_disk_cleanup_avoids_bulk_package_removal() -> None:
    action = _read(".github/actions/free-disk-space/action.yml")

    assert "remove_packages:" not in action
    assert "remove_packages_one_command:" not in action


def test_docker_cache_uses_magic_cache_compatible_backend() -> None:
    action = load_action("docker")
    steps = action["runs"]["steps"]
    build = next(step for step in steps if step.get("id") == "build")["with"]

    assert build["cache-from"] == (
        "type=gha,version=2,scope=qgc-docker-${{ inputs.variant }}-${{ inputs.target }}"
    )
    assert build["cache-to"] == (
        "${{ github.event_name != 'pull_request' && "
        "format('type=gha,version=2,scope=qgc-docker-{0}-{1},mode=max', "
        "inputs.variant, inputs.target) || '' }}"
    )
    assert "type=registry" not in _read(".github/actions/docker/action.yml")
    login = next(step for step in steps if step.get("name") == "Login to GHCR")
    assert login["if"] == "startsWith(inputs.push-image, 'ghcr.io/')"
    assert build["push"] == "${{ inputs.push-image != '' }}"
    assert build["load"] is True


def test_docker_build_enables_magic_cache_on_upstream_aws_runners() -> None:
    workflow = load_workflow("docker.yml")
    job = workflow["jobs"]["build"]
    assert "runs-on={0}-build-{1}-{2}/runner={3}" in job["runs-on"]
    assert "github.repository_owner == 'mavlink'" in job["runs-on"]
    assert "github.event." not in job["runs-on"]
    assert "'ubuntu-latest'" in job["runs-on"]
    steps = job["steps"]
    magic = next(
        index for index, step in enumerate(steps) if step.get("uses") == "runs-on/action@v2"
    )
    build = next(
        index for index, step in enumerate(steps) if step.get("uses") == "./.github/actions/docker"
    )
    assert magic < build
    config = load_yaml(".github/runs-on.yml")
    for pool in ("linux-x64-builder", "linux-x64-builder-prebaked"):
        assert config["runners"][pool]["extras"] == "s3-cache"


def test_docker_cache_setup_uses_image_identity_and_shared_policy():
    docker = get_steps(load_action("docker"))
    setup_index = next(
        i for i, step in enumerate(docker) if step.get("uses") == "./.github/actions/setup-python"
    )
    cache_index = next(
        i for i, step in enumerate(docker) if step.get("uses") == "./.github/actions/cache"
    )
    build_index = next(i for i, step in enumerate(docker) if step.get("id") == "application-build")
    assert setup_index < cache_index < build_index
    cache = docker[cache_index]["with"]
    assert cache["target"] == "docker-${{ inputs.variant }}"
    assert cache["key-suffix"] == "${{ steps.compilers.outputs.fingerprint }}"
    assert cache["install-tools"] == "false"
    assert "save-cache" not in cache
    shared = load_action("cache")
    assert shared["inputs"]["install-tools"]["default"] == "true"
    installs = [
        step for step in shared["runs"]["steps"] if step.get("name") == "Install compiler tools"
    ]
    assert installs
    for step in installs:
        assert "inputs.install-tools == 'true'" in step["if"]
    reset = next(step for step in shared["runs"]["steps"] if "ccache -z" in step.get("run", ""))
    assert "inputs.install-tools == 'true' && " in reset["env"]["RESET_CCACHE"]
    assert 'if [[ "${RESET_CCACHE}" == true ]]; then\n  ccache -z\nfi\n' in reset["run"]
    report = next(
        step for step in docker if step.get("name") == "Report application build performance"
    )
    assert "!cancelled()" in report["if"]
    assert "outcome == 'success'" not in report["if"]


def test_scoped_runs_do_not_resave_the_default_branch_cpm_snapshot(plan):
    steps = _cache_steps()
    shared = next(step for step in steps if step.get("id") == "cpm-shared")
    save, restore = (
        next(step for step in steps if step.get("name") == f"Cache CPM Modules ({kind})")
        for kind in ("restore and save", "restore only")
    )
    # Matching paths keep the cache version equal to the default branch's saved archive.
    assert shared["uses"] == "actions/cache/restore@v6"
    assert shared["with"]["path"] == save["with"]["path"] == restore["with"]["path"]
    # GitHub searches every key in the PR scope before the default branch, so a scoped
    # restore-key could select an older PR snapshot instead of the exact default-branch one.
    assert shared["with"]["key"] == "${{ steps.plan.outputs.cpm_shared_key }}"
    assert "restore-keys" not in shared["with"]
    assert "lookup-only" not in shared["with"]
    assert shared["if"] == "steps.plan.outputs.cpm_shared_probe == 'true'"
    # Saving and restoring are exclusive modes, both skipped after an exact shared hit.
    missed = "steps.cpm-shared.outputs.cache-hit != 'true'"
    assert save["if"] == f"steps.plan.outputs.cpm_mode == 'save' && {missed}"
    assert restore["if"] == f"steps.plan.outputs.cpm_mode == 'restore' && {missed}"
    assert steps.index(shared) < steps.index(save)

    pr = {"event": "pull_request", "ref_name": "1/merge", "pr_number": "1", "build_type": "Release"}
    probes = {
        "github pr": plan(**pr),
        "s3 pr": plan(**pr, backend="s3"),
        "branch": plan(build_type="Release", ref_name="feature"),
        "default branch": plan(build_type="Release"),
        "debug pr": plan(**{**pr, "build_type": "Debug"}),
        "read-only pr": plan(**pr, save_cache="false"),
    }
    modes = {
        name: (result.outputs["cpm_shared_probe"], result.outputs["cpm_mode"])
        for name, result in probes.items()
    }
    assert modes == {
        "github pr": ("true", "restore"),
        "s3 pr": ("true", "save"),
        "branch": ("true", "save"),
        "default branch": ("false", "save"),
        "debug pr": ("false", "restore"),
        "read-only pr": ("false", "restore"),
    }
    assert (
        probes["github pr"].outputs["cpm_shared_key"] == probes["default branch"].outputs["cpm_key"]
    )


def test_preinstalled_cpm_seed_runs_after_archive_restore():
    steps = get_steps(load_action("cache"))
    seed_index = next(i for i, step in enumerate(steps) if "seed-cache" in step.get("run", ""))
    restores = [
        i for i, step in enumerate(steps) if step.get("name", "").startswith("Cache CPM Modules")
    ]
    assert restores and all(i < seed_index for i in restores)
    assert "QGC_PREINSTALLED_CPM_DIR" in steps[seed_index]["if"]


def test_dependencies_use_immutable_separate_sdk_archives(plan, tmp_path):
    steps = _cache_steps()
    cpm = [step for step in steps if step.get("name", "").startswith("Cache CPM Modules")]
    sdk = [step for step in steps if step.get("name", "").startswith("Cache GStreamer SDK")]
    assert len(cpm) == len(sdk) == 2
    for step in cpm:
        assert "!${{ env.CPM_CACHE_PATH }}/gstreamer*" in step["with"]["path"]
        assert step["with"]["key"] == "${{ steps.plan.outputs.cpm_key }}"
    # Only the extracted SDK trees are cached; configure reuses a tree stamped with the pins without its archive.
    # actions/cache does not expand a matched directory, so the include names the entries the exclusions remove.
    for step in sdk:
        assert step["with"]["path"].splitlines() == [
            "${{ env.CPM_CACHE_PATH }}/gstreamer*/*",
            "!${{ env.CPM_CACHE_PATH }}/gstreamer*/*.tar.xz",
            "!${{ env.CPM_CACHE_PATH }}/gstreamer*/*.exe",
            "!${{ env.CPM_CACHE_PATH }}/gstreamer*/*.pkg",
        ]
        assert step["with"]["key"] == "${{ steps.plan.outputs.gst_key }}"
    # Unrelated GStreamer CMake edits and other build-config.json sections must not invalidate the SDK caches.
    planner = next(step for step in steps if step.get("id") == "plan")
    assert planner["env"]["GST_SDK_FILES_HASH"] == (
        "${{ hashFiles('cmake/GStreamer/Download.cmake', 'cmake/GStreamer/platform/*.cmake') }}"
    )

    release = {"target": "android", "build_type": "Release"}
    first = plan(**release).outputs
    rerun = plan(**release, run_id="8", run_attempt="2").outputs
    assert (first["cpm_key"], first["gst_key"]) == (rerun["cpm_key"], rerun["gst_key"])
    config = tmp_path / ".github/build-config.json"
    config.write_text('{"qt": {"version": "6.11"}, "gstreamer": {"version": "1"}}')
    assert plan(**release).outputs["gst_key"] == first["gst_key"]
    config.write_text('{"gstreamer": {"version": "2"}}')
    assert plan(**release).outputs["gst_key"] != first["gst_key"]
    # Legs with a key suffix fall back to the platform's unsuffixed sources.
    custom = plan(
        target="android", key_suffix="custom-debug", event="pull_request", pr_number="1"
    ).outputs["cpm_restore_keys"]
    assert custom.split("\n")[-2:] == [
        "cpm-sources-v2-mac-android-pr-1-",
        "cpm-sources-v2-mac-android-shared-",
    ]


def test_compiler_caches_save_before_downstream_failures():
    restore = get_steps(load_action("cache"))
    for step in restore:
        if step.get("with", {}).get("path") in {".cache/ccache", ".cache/moccache"}:
            assert step["uses"] == "actions/cache/restore@v6"
    build = get_steps(load_action("cmake-build"))
    save = next(step for step in build if step.get("uses") == "./.github/actions/save-build-cache")
    assert "steps.compile.outputs.build_success == 'true'" in save["if"]
    assert "inputs.target == 'autogen'" in save["if"]
    docker = get_steps(load_action("docker"))
    save = next(step for step in docker if step.get("uses") == "./.github/actions/save-build-cache")
    assert "!cancelled()" in save["if"]
    assert "steps.performance.outputs.build_success == 'true'" in save["if"]


def test_sccache_pilot_is_gated_to_the_linux_release_legs() -> None:
    jobs = load_workflow("linux.yml")["jobs"]
    build = jobs["build"]
    assert build["env"]["QGC_COMPILER_CACHE"] == "${{ vars.QGC_COMPILER_CACHE }}"
    (runs_on,) = (step for step in build["steps"] if step.get("uses") == "runs-on/action@v2")
    sccache = "${{ vars.QGC_COMPILER_CACHE == 'sccache' && 's3' || '' }}"
    assert runs_on["with"]["sccache"] == sccache
    # Coverage and sanitizer builds, and every other workflow, stay on ccache.
    assert "QGC_COMPILER_CACHE" not in str(jobs["debug-validation"])
    for workflow in workflow_names():
        if workflow != "linux.yml":
            text = (REPO_ROOT / ".github/workflows" / workflow).read_text(encoding="utf-8")
            assert "QGC_COMPILER_CACHE" not in text, workflow


def test_sccache_pilot_restores_and_saves_no_ccache_archive(plan) -> None:
    named = {step["name"]: step for step in _cache_steps()}
    selected = "env.QGC_COMPILER_CACHE_TOOL == 'ccache'"
    assert selected in named["Restore Build Cache (ccache)"]["if"]
    assert selected in named["Reset cache statistics"]["env"]["RESET_CCACHE"]
    # ccache_helper.py save-plan reads QGC_CCACHE_KEY and QGC_BUILD_CACHE_ARTIFACT; an empty key
    # skips the save (test_ccache_helper.py).
    saves = {step["name"]: step for step in get_steps(load_action("save-build-cache"))}
    assert "steps.plan.outputs.ccache_key != ''" in saves["Save compiler cache"]["if"]

    leg = {"host": "linux", "target": "linux_gcc_64", "build_type": "Release"}
    result = plan(**leg, compiler_cache="sccache", sccache_bucket="bucket")
    assert result.env["QGC_COMPILER_CACHE_TOOL"] == "sccache"
    assert result.env["QGC_CCACHE_KEY"] == ""
    assert result.env["QGC_BUILD_CACHE_ARTIFACT"] == "false"
    assert result.env["QGC_MOCCACHE_KEY"] == plan(**leg).env["QGC_MOCCACHE_KEY"]


def test_gradle_cache_is_s3_only_and_survives_post_job_cleanup() -> None:
    steps = get_steps(load_action("build-setup"))
    gradle = next(
        step for step in steps if step.get("uses", "").startswith("gradle/actions/setup-gradle@")
    )
    options = gradle["with"]
    assert options["cache-disabled"] == "${{ env.QGC_ACTIONS_CACHE_BACKEND != 's3' }}"
    assert options["cache-provider"] == "basic"
    # The saving post step re-evaluates these without the composite's inputs context.
    assert not any("inputs." in str(value) for value in options.values())


def test_build_setup_python_groups_default_to_scripts_and_linux_coverage_adds_coverage() -> None:
    action = load_action("build-setup")
    (python,) = (
        step
        for step in action["runs"]["steps"]
        if step.get("uses") == "./.github/actions/setup-python"
    )
    assert action["inputs"]["python-groups"]["default"] == "scripts"
    assert python["with"]["groups"] == "${{ inputs.python-groups }}"

    job = load_workflow("linux.yml")["jobs"]["debug-validation"]
    (setup,) = (
        step for step in job["steps"] if step.get("uses") == "./.github/actions/build-setup"
    )
    assert setup["with"]["python-groups"] == (
        "${{ matrix.mode == 'coverage' && 'scripts,coverage' || 'scripts' }}"
    )
    assert "install_python.py" not in str(job["steps"])
