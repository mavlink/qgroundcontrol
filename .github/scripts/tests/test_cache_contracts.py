"""Static contracts for CI cache configuration."""

from __future__ import annotations

import os
import subprocess
from pathlib import Path

import pytest
import yaml

REPO_ROOT = Path(__file__).resolve().parents[3]


def _read(path: str) -> str:
    return (REPO_ROOT / path).read_text(encoding="utf-8")


def test_build_caches_use_rolling_platform_keys() -> None:
    action = _read(".github/actions/cache/action.yml")
    generation = "${{ github.run_id }}-${{ github.run_attempt }}"

    for output in ("ccache_key", "moccache_key"):
        assert action.count(f"key: ${{{{ steps.cache-keys.outputs.{output} }}}}-{generation}") == 1
    assert "ccache_key=${ccache_base}${key_suffix}" in action
    assert 'cpm_base="cpm-sources-v2-${HOST}-${TARGET}${key_suffix}"' in action
    assert "inputs.key-suffix" in action


def test_compiler_archives_use_workspace_relative_paths() -> None:
    steps = yaml.safe_load(_read(".github/actions/cache/action.yml"))["runs"]["steps"]
    paths = [step["with"]["path"] for step in steps if "uses" in step]
    saves = yaml.safe_load(_read(".github/actions/save-build-cache/action.yml"))["runs"]["steps"]
    paths += [step["with"]["path"] for step in saves if "uses" in step]
    assert paths.count(".ccache") == 2
    assert paths.count(".cache/moccache") == 2


@pytest.mark.parametrize("suffix", ["", "device", "simulator", "custom-debug"])
def test_pr_keys_restore_matching_master_matrix_cache(tmp_path, suffix) -> None:
    steps = yaml.safe_load(_read(".github/actions/cache/action.yml"))["runs"]["steps"]
    script = next(step["run"] for step in steps if step.get("id") == "cache-keys")

    def keys(scope, build_type="Debug", variant="portable-tests", key_suffix=suffix):
        output = tmp_path / f"{scope}-{build_type}-{variant}"
        env = dict(
            os.environ,
            HOST="mac",
            TARGET="ios",
            BUILD_TYPE=build_type,
            VARIANT=variant,
            KEY_SUFFIX=key_suffix,
            SCOPE=scope,
            CCACHE_HASH="a" * 64,
            MOCCACHE_HASH="b" * 64,
            CPM_FINGERPRINT="c" * 64,
            GITHUB_OUTPUT=str(output),
        )
        output.unlink(missing_ok=True)
        subprocess.run(["bash", "-e", "-c", script], env=env, check=True)
        return dict(line.split("=", 1) for line in output.read_text().splitlines())

    ordinary = keys("pr-42", "Release", key_suffix="")
    custom = keys("pr-42", key_suffix="custom-debug")
    assert ordinary["cpm_key"].startswith(custom["cpm_unvaried_scope"])

    master = keys("shared")
    pr = keys("pr-42")
    assert pr["artifact_ccache_shared"] == master["ccache_key"]
    assert pr["artifact_moccache_shared"] == master["moccache_key"]
    release = keys("shared", "Release")
    sanitizer = keys("shared", variant="sanitizers")
    assert sanitizer["moccache_key"] != master["moccache_key"]
    assert sanitizer["moccache_key"].startswith(master["moccache_hash"])
    assert release["moccache_key"] != master["moccache_key"]
    assert release["moccache_key"].startswith(master["moccache_hash"])
    for family in ("ccache", "moccache", "cpm"):
        assert master[f"{family}_key"].startswith(pr[f"{family}_shared"])
        assert not pr[f"{family}_key"].startswith(master[f"{family}_shared"])


def test_moccache_is_cross_platform_and_content_versioned() -> None:
    action = _read(".github/actions/cache/action.yml")
    build = _read(".github/actions/cmake-build/action.yml")

    assert "runner.os != 'Windows'" not in action
    assert "runner.os != 'Windows'" not in build
    assert (
        "hashFiles('.github/build-config.json', 'cmake/Helpers.cmake', 'tools/moccache.py')"
        in action
    )
    assert 'moccache_base="moccache-${HOST}-${TARGET}-${SCOPE}"' in action
    assert "moccache_shared=moccache-${HOST}-${TARGET}-shared-" in action


def test_qt_cache_stores_only_architecture_tree() -> None:
    action = _read(".github/actions/qt-install/action.yml")
    arch_path = "${{ steps.qt-meta.outputs.cache_dir }}/Qt/${{ inputs.version }}/${{ steps.qt-meta.outputs.arch_dir }}"

    assert action.count(arch_path) == 3
    assert "dir: ${{ runner.temp }}" not in _read(".github/actions/qt-android/action.yml")


def test_qt_install_prefers_compatible_preinstalled_sdk() -> None:
    action = _read(".github/actions/qt-install/action.yml")

    assert "resolve-preinstalled" in action
    assert "steps.qt-preinstalled.outputs.qt_root_dir ||" in action
    assert action.count("steps.qt-preinstalled.outputs.available != 'true'") == 5


def test_dependency_caches_use_shared_backend() -> None:
    prerequisites = _read(".github/actions/build-prerequisites/action.yml")
    setup_python = _read(".github/actions/setup-python/action.yml")

    assert "useCloudCache: true" in prerequisites
    assert "useLocalCache: false" in prerequisites
    assert "RUNS_ON_S3_BUCKET_CACHE" in setup_python
    assert "cache-python: ${{ env.QGC_ACTIONS_CACHE_BACKEND == 's3' }}" in setup_python
    assert "cache-dependency-glob: tools/uv.lock" in setup_python


def test_nested_cache_save_inputs_survive_post_job_cleanup() -> None:
    action = _read(".github/actions/cache/action.yml")
    python = _read(".github/actions/setup-python/action.yml")

    assert action.count("          ${{ env.CPM_CACHE_PATH }}/*") == 2
    assert "path: ${{ steps.cpm-cache.outputs.path }}" not in action
    assert "enable-cache: ${{ env.QGC_ACTIONS_CACHE_BACKEND == 's3' }}" in python
    assert "steps.cache-policy.outputs" not in python


def test_ios_matrix_forwards_distinct_cache_write_suffixes() -> None:
    workflow = _read(".github/workflows/ios.yml")
    setup = _read(".github/actions/build-setup/action.yml").split("- name: Install Qt for iOS")[1]
    ios = _read(".github/actions/qt-ios/action.yml")

    assert "cache-key-suffix: ${{ matrix.target }}" in workflow
    assert "cache-key-suffix: ${{ inputs.cache-key-suffix }}" in setup
    assert "key-suffix: ${{ inputs.cache-key-suffix }}" in ios


def test_macos_installed_cache_binary_is_selected_explicitly() -> None:
    action = _read(".github/actions/cache/action.yml")
    assert 'echo QGC_CACHE_PROGRAM=/usr/local/bin/ccache >> "$GITHUB_ENV"' in action
    assert 'echo /usr/local/bin >> "$GITHUB_PATH"' in action


def test_link_cache_has_restore_and_save_with_fork_read_only_policy() -> None:
    workflow = _read(".github/workflows/check-links.yml")
    assert "uses: actions/cache/restore@" in workflow
    assert "uses: actions/cache/save@" in workflow
    assert workflow.count("path: .lycheecache") == 2
    assert "key: ${{ steps.link-cache.outputs.cache-primary-key }}" in workflow
    assert "github.event.pull_request.head.repo.full_name == github.repository" in workflow


def test_runs_on_jobs_reject_fork_pull_requests() -> None:
    guard = "github.event.pull_request.head.repo.full_name == github.repository"

    assert _read(".github/workflows/linux.yml").count(guard) >= 5
    custom = yaml.safe_load(_read(".github/workflows/custom-build.yml"))["jobs"]["build"]
    assert guard in custom["runs-on"]
    magic_cache = next(
        step for step in custom["steps"] if step.get("uses", "").startswith("runs-on/action@")
    )
    assert guard in magic_cache["if"]


def test_manual_linux_reuses_coverage_build_for_excluded_tests() -> None:
    workflow = yaml.safe_load(_read(".github/workflows/linux.yml"))
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
    workflow = yaml.load(_read(f".github/workflows/{platform}.yml"), Loader=yaml.BaseLoader)
    assert "schedule" not in workflow["on"]
    assert {"push", "pull_request", "workflow_dispatch"} <= workflow["on"].keys()
    if platform == "linux":
        assert "merge_group" in workflow["on"]


def test_cache_cleanup_runs_every_six_hours() -> None:
    assert "cron: '17 */6 * * *'" in _read(".github/workflows/cache-cleanup.yml")


def test_disk_cleanup_avoids_bulk_package_removal() -> None:
    action = _read(".github/actions/free-disk-space/action.yml")

    assert "remove_packages:" not in action
    assert "remove_packages_one_command:" not in action


def test_docker_cache_uses_magic_cache_compatible_backend() -> None:
    action = yaml.safe_load(_read(".github/actions/docker/action.yml"))
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


def test_docker_build_enables_magic_cache_on_trusted_aws_runners() -> None:
    workflow = yaml.safe_load(_read(".github/workflows/docker.yml"))
    job = workflow["jobs"]["build"]
    assert "runs-on={0}/runner={1}" in job["runs-on"]
    assert "github.event.pull_request.head.repo.full_name == github.repository" in job["runs-on"]
    assert "'ubuntu-latest'" in job["runs-on"]
    steps = job["steps"]
    magic = next(
        index for index, step in enumerate(steps) if step.get("uses") == "runs-on/action@v2"
    )
    build = next(
        index for index, step in enumerate(steps) if step.get("uses") == "./.github/actions/docker"
    )
    assert magic < build
    assert (
        "github.event.pull_request.head.repo.full_name == github.repository" in steps[magic]["if"]
    )
    config = yaml.safe_load(_read(".github/runs-on.yml"))
    for pool in ("linux-x64-builder", "linux-x64-builder-prebaked"):
        assert config["runners"][pool]["extras"] == "s3-cache"


def test_docker_cache_setup_uses_image_identity_and_shared_policy():
    docker = yaml.safe_load(_read(".github/actions/docker/action.yml"))["runs"]["steps"]
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
    assert cache["cpm-modules"] == ".cache/CPM"
    assert cache["install-tools"] == "false"
    assert "save-cache" not in cache
    shared = yaml.safe_load(_read(".github/actions/cache/action.yml"))
    assert shared["inputs"]["install-tools"]["default"] == "true"
    for step in shared["runs"]["steps"]:
        if step.get("name", "").startswith(("Install ccache", "Install mold", "Initialize ccache")):
            assert "inputs.install-tools == 'true'" in step["if"]
    report = next(
        step for step in docker if step.get("name") == "Report application build performance"
    )
    assert "!cancelled()" in report["if"]
    assert "outcome == 'success'" not in report["if"]


def test_preinstalled_cpm_seed_runs_after_archive_restore():
    steps = yaml.safe_load(_read(".github/actions/cache/action.yml"))["runs"]["steps"]
    seed_index = next(i for i, step in enumerate(steps) if "seed-cache" in step.get("run", ""))
    restores = [
        i for i, step in enumerate(steps) if step.get("name", "").startswith("Cache CPM Modules")
    ]
    assert restores and all(i < seed_index for i in restores)
    assert "QGC_PREINSTALLED_CPM_DIR" in steps[seed_index]["if"]


def test_dependencies_use_immutable_separate_sdk_archives():
    steps = yaml.safe_load(_read(".github/actions/cache/action.yml"))["runs"]["steps"]
    cpm = [step for step in steps if step.get("name", "").startswith("Cache CPM Modules")]
    sdk = [step for step in steps if step.get("name", "").startswith("Cache GStreamer SDK")]
    assert len(cpm) == len(sdk) == 2
    for step in cpm + sdk:
        assert "github.run_id" not in step["with"]["key"]
    for step in cpm:
        assert "!${{ env.CPM_CACHE_PATH }}/gstreamer*" in step["with"]["path"]
        assert "cpm_unvaried_scope" in step["with"]["restore-keys"]
        assert "cpm_unvaried_shared" in step["with"]["restore-keys"]
    assert all(step["with"]["path"] == "${{ env.CPM_CACHE_PATH }}/gstreamer*" for step in sdk)


def test_compiler_caches_save_before_downstream_failures():
    restore = yaml.safe_load(_read(".github/actions/cache/action.yml"))["runs"]["steps"]
    for step in restore:
        if step.get("with", {}).get("path") in {".ccache", ".cache/moccache"}:
            assert step["uses"] == "actions/cache/restore@v5"
    build = yaml.safe_load(_read(".github/actions/cmake-build/action.yml"))["runs"]["steps"]
    save = next(step for step in build if step.get("uses") == "./.github/actions/save-build-cache")
    assert "steps.compile.outputs.build_success == 'true'" in save["if"]
    assert "inputs.target == 'autogen'" in save["if"]
    docker = yaml.safe_load(_read(".github/actions/docker/action.yml"))["runs"]["steps"]
    save = next(step for step in docker if step.get("uses") == "./.github/actions/save-build-cache")
    assert "!cancelled()" in save["if"]
    assert "steps.performance.outputs.build_success == 'true'" in save["if"]
