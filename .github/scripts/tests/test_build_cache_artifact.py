"""Compiler snapshots retain branch isolation and reject unsafe archive contents."""

from __future__ import annotations

import io
import json
import subprocess
import tarfile
from pathlib import Path

import build_cache_artifact as cache
import pytest


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
        (".ccache/../../escape", tarfile.REGTYPE),
        ("tools/evil.py", tarfile.REGTYPE),
        (".ccache/link", tarfile.SYMTYPE),
        (".ccache/hardlink", tarfile.LNKTYPE),
        (".ccache/C:drive", tarfile.REGTYPE),
        (".ccache\\escape", tarfile.REGTYPE),
    ],
)
def test_archive_is_validated_before_any_cache_file_is_written(tmp_path, name, kind):
    archive = tmp_path / "cache.tar"
    with tarfile.open(archive, "w") as tar:
        good = tarfile.TarInfo(".ccache/good")
        good.size = 4
        tar.addfile(good, io.BytesIO(b"good"))
        bad = tarfile.TarInfo(name)
        bad.type = kind
        bad.linkname = "../escape" if kind != tarfile.REGTYPE else ""
        tar.addfile(bad)
    with pytest.raises(ValueError, match="Unexpected compiler cache member"):
        cache.unpack_cache(archive, tmp_path)
    assert not (tmp_path / ".ccache/good").exists()


def test_empty_cache_is_not_published(tmp_path):
    assert cache.pack_cache(tmp_path, tmp_path / "packed") is None


def test_find_artifact_skips_expired_self_and_wrong_workflow(monkeypatch):
    def gh(*args):
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
        return subprocess.CompletedProcess(args, 0, json.dumps(payload))

    monkeypatch.setattr(cache, "gh", gh)
    found = cache.find_artifact("owner/repo", "2", "cache")
    assert found is not None
    assert found["id"] == 3


@pytest.mark.parametrize(
    "backend,event,enabled",
    [
        ("github", "pull_request", "true"),
        ("s3", "pull_request", "false"),
        ("github", "pull_request_target", "false"),
        ("github", "push", "true"),
    ],
)
def test_configure_preserves_backend_and_event_boundaries(
    monkeypatch, tmp_path, backend, event, enabled
):
    env = tmp_path / "env"
    for key, value in {
        "GITHUB_ENV": str(env),
        "GITHUB_EVENT_NAME": event,
        "QGC_ACTIONS_CACHE_BACKEND": backend,
        "COMPILER_KEY": "compiler",
        "MOC_KEY": "moc",
        "COMPILER_SHARED_KEY": "shared",
        "MOC_SHARED_KEY": "shared-moc",
    }.items():
        monkeypatch.setenv(key, value)
    assert cache.main(["configure"]) == 0
    assert f"QGC_BUILD_CACHE_ARTIFACT={enabled}\n" in env.read_text()


def test_failed_restore_is_informational_and_allows_legacy_fallback(monkeypatch, gh_output):
    monkeypatch.setenv("GITHUB_REPOSITORY", "owner/repo")
    monkeypatch.setenv("GITHUB_RUN_ID", "2")
    monkeypatch.setenv("GITHUB_WORKSPACE", ".")
    monkeypatch.setenv("QGC_BUILD_CACHE_ARTIFACT_NAME", "cache")
    monkeypatch.setattr(cache, "restore", lambda *_: (_ for _ in ()).throw(OSError("offline")))
    assert cache.main(["restore"]) == 0
    assert "restored=false" in gh_output.read_text()


def test_cache_actions_save_artifact_once_with_s3_and_upload_failure_fallback():
    import yaml

    root = Path(__file__).resolve().parents[2]
    steps = yaml.safe_load((root / "actions/save-build-cache/action.yml").read_text())["runs"][
        "steps"
    ]
    indexed = {step["name"]: step for step in steps}
    upload = indexed["Retain compiler cache snapshot"]
    assert upload["with"]["retention-days"] == 1
    assert upload["with"]["compression-level"] == 0
    assert "QGC_BUILD_CACHE_SAVED != 'true'" in indexed["Pack compiler cache snapshot"]["if"]
    assert "steps.artifact.outcome != 'success'" in indexed["Save compiler cache"]["if"]
