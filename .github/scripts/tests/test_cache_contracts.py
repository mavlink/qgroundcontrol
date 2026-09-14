"""Static contracts for CI cache configuration."""

from __future__ import annotations

from pathlib import Path

import yaml

REPO_ROOT = Path(__file__).resolve().parents[3]


def _read(path: str) -> str:
    return (REPO_ROOT / path).read_text(encoding="utf-8")


def test_build_caches_use_rolling_platform_keys() -> None:
    action = _read(".github/actions/cache/action.yml")
    generation = "${{ github.run_id }}-${{ github.run_attempt }}"

    for output in ("ccache_key", "moccache_key", "cpm_key"):
        assert action.count(f"key: ${{{{ steps.cache-keys.outputs.{output} }}}}-{generation}") == 2
    assert "ccache_key=${ccache_base}${key_suffix}" in action
    assert 'cpm_base="cpm-modules-${HOST}-${TARGET}${key_suffix}"' in action
    assert "inputs.key-suffix" in action


def test_ccache_size_uses_runs_on_capacity() -> None:
    action = _read(".github/actions/cache/action.yml")

    assert "max_size=1G" in action
    assert "RUNS_ON_S3_BUCKET_CACHE" in action
    assert 'max_size="${CONFIGURED_MAX_SIZE}"' in action


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
    arch_path = "${{ inputs.dir }}/Qt/${{ inputs.version }}/${{ steps.qt-meta.outputs.arch_dir }}"

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
    assert "cache-python: ${{ env.RUNS_ON_S3_BUCKET_CACHE != '' }}" in setup_python
    assert "cache-dependency-glob: tools/uv.lock" in setup_python


def test_nested_cache_save_inputs_survive_post_job_cleanup() -> None:
    action = _read(".github/actions/cache/action.yml")
    python = _read(".github/actions/setup-python/action.yml")

    assert action.count("path: ${{ env.CPM_SOURCE_CACHE }}") == 2
    assert "path: ${{ steps.cpm-cache.outputs.path }}" not in action
    assert "enable-cache: ${{ env.RUNS_ON_S3_BUCKET_CACHE != '' }}" in python
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
    assert _read(".github/workflows/custom-build.yml").count(guard) >= 4


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
