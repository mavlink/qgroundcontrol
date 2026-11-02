"""Copilot sessions restore only master's own apt downloads, and only their .deb files."""

from __future__ import annotations

import zipfile

import pytest
from ci.runner import apt_debs_artifact as artifact

REPOSITORY = {"id": 22, "default_branch": "master"}


def run(run_id: int = 1, **changes):
    return {
        "id": run_id,
        "path": artifact.PRODUCER_WORKFLOW,
        "event": "schedule",
        "head_branch": "master",
        "head_repository": {"id": 22},
        **changes,
    }


@pytest.mark.parametrize(
    "changes",
    [
        {"path": ".github/workflows/linux.yml"},
        {"event": "pull_request"},
        {"event": "dynamic"},
        {"head_branch": "feature"},
        {"head_repository": {"id": 33}},
        {"head_repository": None},
    ],
)
def test_only_the_setup_workflow_s_master_runs_are_trusted(changes):
    assert artifact.trusted_producer(run(), REPOSITORY)
    assert not artifact.trusted_producer(run(**changes), REPOSITORY)


def test_find_artifact_takes_the_newest_from_a_trusted_run(monkeypatch):
    listing = {
        "artifacts": [
            {"id": 4, "name": artifact.ARTIFACT_NAME, "expired": True, "workflow_run": {"id": 4}},
            {"id": 3, "name": artifact.ARTIFACT_NAME, "workflow_run": {"id": 3}},
            {"id": 2, "name": artifact.ARTIFACT_NAME, "workflow_run": None},
            {"id": 1, "name": artifact.ARTIFACT_NAME, "workflow_run": {"id": 1}},
            {"id": 5, "name": "other", "workflow_run": {"id": 5}},
        ]
    }
    runs = {3: run(3, event="pull_request"), 1: run(1), 4: run(4), 5: run(5)}

    def gh_api_json(path):
        if path == "repos/owner/repo":
            return REPOSITORY
        if "/actions/artifacts?" in path:
            return listing
        return runs[int(path.rsplit("/", 1)[1])]

    monkeypatch.setattr(artifact, "gh_api_json", gh_api_json)
    found = artifact.find_artifact("owner/repo")
    assert found is not None
    assert found["id"] == 1


def test_extract_copies_only_top_level_debs(tmp_path):
    archive = tmp_path / "debs.zip"
    with zipfile.ZipFile(archive, "w") as bundle:
        bundle.writestr("libfoo_1.2%3a3-1_amd64.deb", b"deb")
        bundle.writestr("../escape.deb", b"x")
        bundle.writestr("nested/inner.deb", b"x")
        bundle.writestr(".hidden.deb", b"x")
        bundle.writestr("lock", b"x")
    destination = tmp_path / "archives"
    assert artifact.extract_debs(archive, destination) == 1
    assert [path.name for path in destination.iterdir()] == ["libfoo_1.2%3a3-1_amd64.deb"]
    assert not (tmp_path / "escape.deb").exists()


def test_restore_extracts_the_found_artifact(monkeypatch, tmp_path):
    monkeypatch.setattr(
        artifact, "find_artifact", lambda _repo: {"id": 7, "workflow_run": {"id": 9}}
    )

    def download(_repo, artifact_id, destination, *, attempts, timeout):
        assert artifact_id == 7
        with zipfile.ZipFile(destination, "w") as bundle:
            bundle.writestr("libfoo_1.0_amd64.deb", b"deb")

    monkeypatch.setattr(artifact, "download_artifact_zip", download)
    archives = tmp_path / "archives"
    assert artifact.restore("owner/repo", archives) == 1
    assert (archives / "libfoo_1.0_amd64.deb").read_bytes() == b"deb"


def test_failed_download_only_warns(monkeypatch, tmp_path, capsys):
    monkeypatch.setenv("GITHUB_REPOSITORY", "owner/repo")
    monkeypatch.setattr(
        artifact, "find_artifact", lambda _repo: {"id": 7, "workflow_run": {"id": 9}}
    )

    def download(_repo, _artifact_id, _destination, *, attempts, timeout):
        assert (attempts, timeout) == (2, artifact.DOWNLOAD_TIMEOUT_SECONDS)
        raise RuntimeError("transfer timed out")

    monkeypatch.setattr(artifact, "download_artifact_zip", download)
    assert artifact.main(["--destination", str(tmp_path)]) == 0
    assert "::warning::apt download artifact unavailable" in capsys.readouterr().out


def test_malformed_listing_only_warns(monkeypatch, tmp_path, capsys):
    monkeypatch.setenv("GITHUB_REPOSITORY", "owner/repo")
    monkeypatch.setattr(artifact, "gh_api_json", lambda path: {"artifacts": None, **REPOSITORY})
    assert artifact.main(["--destination", str(tmp_path)]) == 0
    assert "::warning::apt download artifact unavailable" in capsys.readouterr().out
