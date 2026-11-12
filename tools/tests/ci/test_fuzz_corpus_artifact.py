"""The nightly fuzzers restore only their own default-branch corpus, and only its inputs."""

from __future__ import annotations

import zipfile

import pytest
from ci.cache import fuzz_corpus_artifact as artifact

REPOSITORY = {"id": 22, "default_branch": "master"}
INPUT = "0123456789abcdef0123456789abcdef01234567"


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
        {"event": "push"},
        {"head_branch": "feature"},
        {"head_repository": {"id": 33}},
        {"head_repository": None},
    ],
)
def test_only_the_workflow_s_nightly_default_branch_runs_are_trusted(changes):
    assert artifact.trusted_producer(run(), REPOSITORY)
    assert artifact.trusted_producer(run(event="workflow_dispatch"), REPOSITORY)
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
    runs = {3: run(3, head_branch="feature"), 1: run(1), 4: run(4), 5: run(5)}

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


def test_extract_copies_only_the_harnesses_inputs(tmp_path):
    archive = tmp_path / "corpus.zip"
    with zipfile.ZipFile(archive, "w") as bundle:
        bundle.writestr(f"source-table/{INPUT}", b"STR;MP")
        bundle.writestr(f"protocol/{INPUT}", b"\xb5\x62")
        bundle.writestr(f"../{INPUT}", b"x")
        bundle.writestr(f"protocol/nested/{INPUT}", b"x")
        bundle.writestr(INPUT, b"x")
        bundle.writestr("protocol/crash-input", b"x")
        bundle.writestr(f"reply/{INPUT[:-1]}0", b"x" * (artifact.MAX_INPUT_BYTES + 1))
    destination = tmp_path / "corpus"
    assert artifact.extract_inputs(archive, destination) == 2
    assert sorted(path.relative_to(destination).as_posix() for path in destination.rglob("*")) == [
        "protocol",
        f"protocol/{INPUT}",
        "source-table",
        f"source-table/{INPUT}",
    ]
    assert (destination / "source-table" / INPUT).read_bytes() == b"STR;MP"
    assert not (tmp_path / INPUT).exists()


def test_restore_extracts_the_found_artifact(monkeypatch, tmp_path):
    monkeypatch.setattr(
        artifact, "find_artifact", lambda _repo: {"id": 7, "workflow_run": {"id": 9}}
    )

    def download(_repo, artifact_id, destination, *, attempts, timeout):
        assert artifact_id == 7
        with zipfile.ZipFile(destination, "w") as bundle:
            bundle.writestr(f"reply/{INPUT}", b"\x00\x04")

    monkeypatch.setattr(artifact, "download_artifact_zip", download)
    corpus = tmp_path / "corpus"
    assert artifact.restore("owner/repo", corpus) == 1
    assert (corpus / "reply" / INPUT).read_bytes() == b"\x00\x04"


def test_no_artifact_leaves_the_seeds(monkeypatch, tmp_path, capsys):
    monkeypatch.setenv("GITHUB_REPOSITORY", "owner/repo")
    monkeypatch.setattr(artifact, "gh_api_json", lambda path: {"artifacts": [], **REPOSITORY})
    assert artifact.main(["--destination", str(tmp_path / "corpus")]) == 0
    assert not (tmp_path / "corpus").exists()
    assert "::warning::" not in capsys.readouterr().out


def test_failed_download_only_warns(monkeypatch, tmp_path, capsys):
    monkeypatch.setenv("GITHUB_REPOSITORY", "owner/repo")
    monkeypatch.setattr(
        artifact, "find_artifact", lambda _repo: {"id": 7, "workflow_run": {"id": 9}}
    )

    def download(_repo, _artifact_id, _destination, *, attempts, timeout):
        assert (attempts, timeout) == (
            artifact.DOWNLOAD_ATTEMPTS,
            artifact.DOWNLOAD_TIMEOUT_SECONDS,
        )
        raise RuntimeError("transfer timed out")

    monkeypatch.setattr(artifact, "download_artifact_zip", download)
    assert artifact.main(["--destination", str(tmp_path)]) == 0
    assert "::warning::fuzzing corpus unavailable" in capsys.readouterr().out
