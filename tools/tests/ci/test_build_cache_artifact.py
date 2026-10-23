"""Compiler snapshots retain branch isolation and reject unsafe archive contents."""

from __future__ import annotations

import io
import json
import subprocess
import tarfile

import pytest
from ci.cache import build_cache_artifact as cache

from .._helpers import completed, get_step, get_steps, load_action

pytestmark = pytest.mark.windows


def run(run_id=2, **changes):
    return {
        "id": run_id,
        "workflow_id": 11,
        "head_branch": "feature",
        "head_repository": {"id": 22},
        "event": "pull_request",
        **changes,
    }


def test_same_producer_can_restore_even_when_later_packaging_failed():
    assert cache.compatible_run(run(), run(1, conclusion="failure"))


@pytest.mark.parametrize(
    "changes",
    [
        {"id": 2},
        {"workflow_id": 12},
        {"head_branch": "master"},
        {"head_repository": {"id": 33}},
        {"event": "pull_request_target"},
        {"event": "push"},
    ],
)
def test_other_producers_are_rejected(changes):
    assert not cache.compatible_run(run(), run(1, **changes))


def test_default_branch_never_accepts_pr_artifacts():
    current = run(event="push", head_branch="master")
    assert not cache.compatible_run(current, run(1, head_branch="master"))
    assert not cache.compatible_run(run(event="pull_request_target"), run(1))


def test_baseline_must_be_default_branch_in_base_repository():
    repository = {"id": 55, "default_branch": "main"}
    candidate = run(1, head_branch="main", head_repository={"id": 55}, event="push")
    assert cache.trusted_baseline(run(), candidate, repository)
    for changes in [
        {"event": "pull_request"},
        {"head_repository": {"id": 22}},
        {"head_branch": "feature"},
        {"workflow_id": 99},
    ]:
        assert not cache.trusted_baseline(run(), {**candidate, **changes}, repository)


def test_key_separates_platform_config_compiler_and_scope():
    first = cache.artifact_name("windows-Release-pr-1-compiler-a", "moc-a")
    assert first != cache.artifact_name("windows-Release-pr-2-compiler-a", "moc-a")
    assert first != cache.artifact_name("windows-Release-pr-1-compiler-b", "moc-a")
    assert first != cache.artifact_name("windows-Release-pr-1-compiler-a", "moc-b")


def test_archive_roundtrip_preserves_both_caches_and_excludes_other_files(tmp_path):
    workspace = tmp_path / "source"
    for root in cache.CACHE_ROOTS:
        directory = workspace / root / "entry"
        directory.mkdir(parents=True)
        (directory / "data").write_bytes(b"cached data")
    (workspace / "secret").write_text("not a cache")
    archive = cache.pack_cache(workspace, tmp_path / "packed")
    assert archive
    destination = tmp_path / "restored"
    destination.mkdir()
    cache.unpack_cache(archive, destination)
    for root in cache.CACHE_ROOTS:
        assert (destination / root / "entry/data").read_bytes() == b"cached data"
    assert not (destination / "secret").exists()


@pytest.mark.parametrize(
    "name,kind",
    [
        ("../escape", tarfile.REGTYPE),
        ("/absolute", tarfile.REGTYPE),
        (".cache/ccache/../../escape", tarfile.REGTYPE),
        ("tools/evil.py", tarfile.REGTYPE),
        (".cache/CPM/evil.cmake", tarfile.REGTYPE),
        (".ccache/legacy", tarfile.REGTYPE),
        (".cache/ccache/link", tarfile.SYMTYPE),
        (".cache/ccache/hardlink", tarfile.LNKTYPE),
        (".cache/ccache/C:drive", tarfile.REGTYPE),
        (".cache/ccache\\escape", tarfile.REGTYPE),
    ],
)
def test_archive_is_validated_before_any_cache_file_is_written(tmp_path, name, kind):
    archive = tmp_path / "cache.tar"
    with tarfile.open(archive, "w") as tar:
        good = tarfile.TarInfo(".cache/ccache/good")
        good.size = 4
        tar.addfile(good, io.BytesIO(b"good"))
        bad = tarfile.TarInfo(name)
        bad.type = kind
        bad.linkname = "../escape" if kind != tarfile.REGTYPE else ""
        tar.addfile(bad)
    with pytest.raises(ValueError, match="Unexpected compiler cache member"):
        cache.unpack_cache(archive, tmp_path)
    assert not (tmp_path / ".cache/ccache/good").exists()


def test_empty_cache_is_not_published(tmp_path):
    assert cache.pack_cache(tmp_path, tmp_path / "packed") is None


def test_find_artifact_skips_expired_self_and_wrong_workflow(monkeypatch):
    def gh(*args, **kwargs):
        path = next(item for item in args if item.startswith("repos/"))
        if path.endswith("/runs/2"):
            payload = run()
        elif path.endswith("/artifacts"):
            payload = {
                "artifacts": [
                    {"id": 6, "name": "cache", "expired": True, "workflow_run": {"id": 1}},
                    {"id": 5, "name": "cache", "workflow_run": {"id": 2}},
                    {"id": 4, "name": "cache", "workflow_run": {"id": 3}},
                    {"id": 3, "name": "cache", "workflow_run": {"id": 1}},
                ]
            }
        elif path.endswith("/runs/3"):
            payload = run(3, workflow_id=99)
        else:
            payload = run(1)
        return completed(json.dumps(payload))

    monkeypatch.setattr(cache, "gh", gh)
    monkeypatch.setattr(cache, "gh_api_json", lambda path: json.loads(gh("api", path).stdout))
    found = cache.find_artifact("owner/repo", "2", "cache")
    assert found is not None
    assert found["id"] == 3


def test_failed_restore_is_informational_and_allows_legacy_fallback(monkeypatch, gh_output):
    monkeypatch.setenv("GITHUB_REPOSITORY", "owner/repo")
    monkeypatch.setenv("GITHUB_RUN_ID", "2")
    monkeypatch.setenv("GITHUB_WORKSPACE", ".")
    monkeypatch.setenv("QGC_BUILD_CACHE_ARTIFACT_NAME", "cache")
    monkeypatch.setattr(cache, "restore", lambda *_: (_ for _ in ()).throw(OSError("offline")))
    assert cache.main(["restore"]) == 0
    assert "restored=false" in gh_output.read_text()


def test_cache_actions_save_artifact_once_with_s3_and_upload_failure_fallback():
    plan, upload, *saves = get_steps(load_action("save-build-cache"))
    assert plan["if"] == "env.QGC_SAVE_BUILD_CACHE == 'true' && env.QGC_BUILD_CACHE_SAVED != 'true'"
    assert plan["run"] == "python3 tools/ci/cache/ccache_helper.py save-plan"
    assert upload["if"] == "steps.plan.outputs.artifact_path != ''"
    assert upload["continue-on-error"] is True
    assert upload["with"]["path"] == "${{ steps.plan.outputs.artifact_path }}"
    assert upload["with"]["retention-days"] == 1
    assert upload["with"]["compression-level"] == 0
    for save, name in zip(saves, ("ccache", "moccache"), strict=True):
        key = f"steps.plan.outputs.{name}_key"
        assert save["if"] == f"steps.artifact.outcome != 'success' && {key} != ''"
        assert save["with"] == {"path": f".cache/{name}", "key": f"${{{{ {key} }}}}"}


def test_pack_snapshot_failure_warns_without_an_archive(tmp_path, monkeypatch, capsys):
    monkeypatch.setattr(cache, "pack_cache", lambda *_: (_ for _ in ()).throw(OSError("full")))
    assert cache.pack_snapshot(tmp_path, str(tmp_path)) is None
    assert "::warning::Compiler cache artifact pack unavailable: full" in capsys.readouterr().out


def test_null_head_repository_is_rejected_without_error():
    repository = {"id": 22, "default_branch": "main"}
    baseline = run(1, event="push", head_branch="main", head_repository=None)
    assert not cache.compatible_run(run(head_repository=None), run(1))
    assert not cache.compatible_run(run(), run(1, head_repository=None))
    assert not cache.trusted_baseline(run(), baseline, repository)


def test_find_artifact_skips_producer_with_null_head_repository(monkeypatch):
    def gh(*args, **kwargs):
        path = next(item for item in args if item.startswith("repos/"))
        if path.endswith("/artifacts"):
            payload = {
                "artifacts": [
                    {"id": 9, "name": "cache", "workflow_run": None},
                    {"id": 8, "name": "cache", "workflow_run": {"id": 4}},
                    {"id": 7, "name": "cache", "workflow_run": {"id": 1}},
                ]
            }
        elif path.endswith("/runs/4"):
            payload = run(4, head_repository=None)
        else:
            payload = run(int(path.rsplit("/", 1)[1]))
        return completed(json.dumps(payload))

    monkeypatch.setattr(cache, "gh", gh)
    monkeypatch.setattr(cache, "gh_api_json", lambda path: json.loads(gh("api", path).stdout))
    found = cache.find_artifact("owner/repo", "2", "cache")
    assert found is not None
    assert found["id"] == 7


@pytest.fixture
def restore_env(monkeypatch, tmp_path, gh_output):
    monkeypatch.setenv("GITHUB_REPOSITORY", "owner/repo")
    monkeypatch.setenv("GITHUB_RUN_ID", "2")
    monkeypatch.setenv("GITHUB_WORKSPACE", str(tmp_path))
    monkeypatch.setenv("QGC_BUILD_CACHE_ARTIFACT_NAME", "cache")
    return gh_output


def test_api_timeouts_after_retries_fall_back(monkeypatch, capsys, restore_env):
    from common import proc

    calls = []

    def hang(cmd, **kwargs):
        calls.append(kwargs["timeout"])
        raise subprocess.TimeoutExpired(cmd, kwargs["timeout"])

    monkeypatch.setattr(proc.subprocess, "run", hang)
    monkeypatch.setattr(proc.time, "sleep", lambda _seconds: None)
    assert cache.main(["restore"]) == 0
    assert calls == [60, 60, 60]
    assert "restored=false" in restore_env.read_text()
    assert "::warning::Compiler cache artifact restore unavailable" in capsys.readouterr().out


def test_stalled_snapshot_download_times_out_and_falls_back(monkeypatch, restore_env):
    artifact = {"id": 3, "name": "cache", "workflow_run": {"id": 1}}
    monkeypatch.setattr(cache, "find_artifact", lambda *_: artifact)
    timeouts = []

    def gh(*args, **kwargs):
        assert args[:2] == ("run", "download")
        timeouts.append(kwargs.get("timeout"))
        raise subprocess.TimeoutExpired(["gh", *args], kwargs["timeout"])

    monkeypatch.setattr(cache, "gh", gh)
    assert cache.main(["restore"]) == 0
    assert timeouts == [cache.DOWNLOAD_TIMEOUT_SECONDS]
    assert "restored=false" in restore_env.read_text()


@pytest.mark.parametrize("payload", [{"artifacts": None}, {"artifacts": ["cache"]}])
def test_malformed_artifact_listing_falls_back(monkeypatch, restore_env, payload):
    def gh(*args, **kwargs):
        path = next(item for item in args if item.startswith("repos/"))
        body = payload if path.endswith("/artifacts") else run()
        return completed(json.dumps(body))

    monkeypatch.setattr(cache, "gh", gh)
    monkeypatch.setattr(cache, "gh_api_json", lambda path: json.loads(gh("api", path).stdout))
    assert cache.main(["restore"]) == 0
    assert "restored=false" in restore_env.read_text()


def test_artifact_restore_step_cannot_fail_the_job():
    steps = get_steps(load_action("cache"))
    restore = get_step(steps, "artifact-cache")
    assert restore["continue-on-error"] is True
    fallbacks = [
        step for step in steps if "steps.artifact-cache.outputs.restored" in step.get("if", "")
    ]
    fallback = "steps.artifact-cache.outputs.restored != 'true'"
    # The sccache pilot restores no ccache archive.
    assert {step["with"]["path"]: step["if"] for step in fallbacks} == {
        ".cache/ccache": f"{fallback} && env.QGC_COMPILER_CACHE_TOOL == 'ccache'",
        ".cache/moccache": fallback,
    }
