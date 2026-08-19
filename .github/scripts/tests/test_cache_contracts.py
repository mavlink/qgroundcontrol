"""Static contracts for CI cache configuration."""

from __future__ import annotations

from pathlib import Path

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

    assert "max_size=2G" in action
    assert "RUNS_ON_S3_BUCKET_CACHE" in action
    assert 'max_size="${CONFIGURED_MAX_SIZE}"' in action


def test_qt_cache_stores_only_architecture_tree() -> None:
    action = _read(".github/actions/qt-install/action.yml")
    arch_path = "${{ inputs.dir }}/Qt/${{ inputs.version }}/${{ steps.qt-meta.outputs.arch_dir }}"

    assert action.count(arch_path) == 3
    assert "dir: ${{ runner.temp }}" not in _read(".github/actions/qt-android/action.yml")


def test_dependency_caches_use_shared_backend() -> None:
    prerequisites = _read(".github/actions/build-prerequisites/action.yml")
    setup_python = _read(".github/actions/setup-python/action.yml")

    assert "useCloudCache: true" in prerequisites
    assert "useLocalCache: false" in prerequisites
    assert "RUNS_ON_S3_BUCKET_CACHE" in setup_python
    assert "cache-python: ${{ steps.cache-policy.outputs.enabled }}" in setup_python
    assert "cache-dependency-glob: tools/uv.lock" in setup_python


def test_runs_on_jobs_reject_fork_pull_requests() -> None:
    guard = "github.event.pull_request.head.repo.full_name == github.repository"

    assert _read(".github/workflows/linux.yml").count(guard) >= 5
    assert _read(".github/workflows/custom-build.yml").count(guard) >= 4


def test_cache_cleanup_runs_hourly() -> None:
    assert "cron: '17 * * * *'" in _read(".github/workflows/cache-cleanup.yml")
