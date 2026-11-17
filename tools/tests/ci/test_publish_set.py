"""Tests for publish_set.py: only verified, marked packages reach the publishing jobs."""

from __future__ import annotations

import hashlib
import json
import shutil
import zipfile
from typing import TYPE_CHECKING, Any

import pytest
from ci.publish import publish_set
from ci.publish.publish_set import (
    NotBuiltError,
    PublishSetError,
    RerunError,
    SourceRun,
    fetch_set,
    load_set,
    main,
    marked_package,
    newest_artifacts,
    zsync_problem,
)

from .._helpers import completed, read_github_file

if TYPE_CHECKING:
    from collections.abc import Callable, Mapping, Sequence
    from pathlib import Path

REPO = "mavlink/qgroundcontrol"
REF = "refs/heads/master"
COMMIT = "36436db319c731b70ab25dd3178896c4c22c75f6"
# Upstream's master commit 36436db3 and its four publishing runs, each approved at attempt 1.
LINUX = SourceRun("Linux", ".github/workflows/linux.yml", 41380314322, 1)
WINDOWS = SourceRun("Windows", ".github/workflows/windows.yml", 41380314306, 1)
MACOS = SourceRun("MacOS", ".github/workflows/macos.yml", 41380314284, 1)
ANDROID = SourceRun("Android", ".github/workflows/android.yml", 41380314279, 1)
RUNS = [LINUX, WINDOWS, MACOS, ANDROID]


def _runs_json(runs: Sequence[SourceRun]) -> str:
    """The decision's `runs` output."""
    return json.dumps(
        [
            {
                "workflow": run.workflow,
                "path": run.path,
                "id": run.run_id,
                "attempt": run.attempt,
                "conclusion": run.conclusion,
            }
            for run in runs
        ]
    )


RUNS_JSON = _runs_json(RUNS)
# Artifact name and package file of every attest-and-upload artifact of those runs; True marks
# the ones the workflows publish.
ARTIFACTS: dict[int, list[tuple[str, str, bool]]] = {
    LINUX.run_id: [
        ("QGroundControl-x86_64", "QGroundControl-x86_64.AppImage", True),
        ("QGroundControl-aarch64", "QGroundControl-aarch64.AppImage", True),
    ],
    WINDOWS.run_id: [
        ("QGroundControl-installer-AMD64", "QGroundControl-installer-AMD64.exe", True),
        ("QGroundControl-installer-ARM64", "QGroundControl-installer-ARM64.exe", True),
        ("QGroundControl-installer-AMD64-ARM64", "QGroundControl-installer-AMD64-ARM64.exe", False),
    ],
    MACOS.run_id: [("QGroundControl", "QGroundControl.dmg", True)],
    ANDROID.run_id: [
        ("QGroundControl-linux-arm64-v8a-armeabi-v7a", "QGroundControl.apk", True),
        ("QGroundControl-linux-arm64-v8a-armeabi-v7a-aab", "QGroundControl.aab", False),
        ("QGroundControl-mac-arm64-v8a", "QGroundControl.apk", False),
    ],
}
PUBLISHED = [
    "QGroundControl-aarch64.AppImage",
    "QGroundControl-installer-AMD64.exe",
    "QGroundControl-installer-ARM64.exe",
    "QGroundControl-x86_64.AppImage",
    "QGroundControl.apk",
    "QGroundControl.dmg",
]


def _marker(
    run: SourceRun,
    file: str,
    packages: list[str] | None = None,
    companions: Mapping[str, bytes] | None = None,
) -> dict[str, Any]:
    """The publish marker of *file*: its name, its workflow and the packages that workflow marks.

    With *companions*, the files that go beside it on a release, as the build recorded them.
    """
    if packages is None:
        packages = sorted(name for _, name, publish in ARTIFACTS[run.run_id] if publish)
    marker: dict[str, Any] = {"name": file, "workflow": run.workflow, "packages": packages}
    if companions is not None:
        marker["companions"] = [
            {"name": name, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()}
            for name, data in companions.items()
        ]
    return marker


def _zsync(file: str, data: bytes, **changes: str | None) -> bytes:
    """The update file that appimagetool writes beside the AppImage *file* holding *data*.

    The header is that of upstream's master run 42969994287, with the package's own values.
    *changes* replaces a field's value, or drops the field with None.
    """
    fields = {
        "zsync": "0.6.2",
        "Filename": file,
        "MTime": "Wed, 11 Nov 2026 16:05:51 +0000",
        "Blocksize": "4096",
        "Length": str(len(data)),
        "Hash-Lengths": "2,2,5",
        "URL": file,
        "SHA-1": hashlib.sha1(data, usedforsecurity=False).hexdigest(),
        **changes,
    }
    header = "".join(f"{key}: {value}\n" for key, value in fields.items() if value is not None)
    return header.encode() + b"\n" + bytes(range(256))


def _metadata(
    run: SourceRun,
    file: str,
    data: bytes,
    *,
    publish: bool,
    job: Mapping[str, Any] | None = None,
) -> dict[str, Any]:
    """Build metadata as attest_helper.py writes it in *job* for a push of COMMIT to master."""
    job = job or {"id": 1, "run_attempt": 1}
    metadata = {
        "schema_version": 2,
        "repository": REPO,
        "commit": COMMIT,
        "ref": REF,
        "event": "push",
        "workflow": run.workflow,
        "job": "build",
        "job_id": str(job["id"]),
        "run_id": str(run.run_id),
        "run_attempt": str(job["run_attempt"]),
        "artifact": {"name": file, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()},
    }
    if publish:
        metadata["publish"] = _marker(run, file)
    return metadata


def _approved(run: SourceRun, attempt: int, conclusion: str = "success") -> list[SourceRun]:
    """The four runs, with *run* approved at *attempt*, as it concluded."""
    approved = SourceRun(run.workflow, run.path, run.run_id, attempt, conclusion)
    return [approved if other is run else other for other in RUNS]


class Runs:
    """Stand-in for the runs, jobs and artifacts APIs and ``gh attestation verify``.

    Every package artifact of the four runs is built by a job of its own, ``Build <artifact>``,
    which passed in attempt 1 and attested the package and its build metadata.
    """

    def __init__(self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
        self.store = tmp_path / "store"
        self.store.mkdir()
        self.listing: dict[int, list[dict[str, Any]]] = {run.run_id: [] for run in RUNS}
        # Each run's latest attempt, as the runs API reports it.
        self.state: dict[int, dict[str, Any]] = {
            run.run_id: {"run_attempt": 1, "status": "completed", "conclusion": "success"}
            for run in RUNS
        }
        self.downloads: list[int] = []
        # Called with the artifact id before each download: what happens meanwhile.
        self.meanwhile: Callable[[int], None] = lambda _artifact_id: None
        self.verified: list[list[str]] = []
        # SHA-256 of every file that has a build provenance attestation.
        self.attested: set[str] = set()
        # Every job by id, as the listing of its attempt reports it.
        self.jobs: dict[int, dict[str, Any]] = {}
        self._next_id = 13113000000
        for run in RUNS:
            # What a run uploads besides its packages.
            self.listing[run.run_id].append(self._entry(run, f"sbom-{run.workflow}"))
            for artifact, file, publish in ARTIFACTS[run.run_id]:
                data = f"{run.workflow} {artifact}".encode()
                job = self.job(run, f"Build {artifact}")
                metadata = _metadata(run, file, data, publish=publish, job=job)
                self.add(run, artifact, file, data, metadata)
        monkeypatch.setattr(publish_set, "gh_api_json", self._run)
        monkeypatch.setattr(publish_set, "list_run_jobs", self._jobs)
        monkeypatch.setattr(publish_set, "list_run_artifacts", self._list)
        monkeypatch.setattr(publish_set, "download_artifact_zip", self._download)
        monkeypatch.setattr(publish_set, "gh", self._gh)

    def job(self, run: SourceRun, name: str, *, attempt: int = 1, hour: int = 3) -> dict[str, Any]:
        """Run the job *name* in *attempt* of *run*: ten minutes from *hour*, and it passes."""
        self._next_id += 1
        self.jobs[self._next_id] = {
            "id": self._next_id,
            "run_id": run.run_id,
            "run_attempt": attempt,
            "name": name,
            "status": "completed",
            "conclusion": "success",
            "started_at": f"2026-11-02T{hour:02}:00:00Z",
            "completed_at": f"2026-11-02T{hour:02}:10:00Z",
        }
        return self.jobs[self._next_id]

    def job_of(self, run: SourceRun, artifact: str, attempt: int = 1) -> dict[str, Any]:
        """The job of *attempt* that builds *artifact*."""
        return next(
            job
            for job in self._jobs(REPO, run.run_id, attempt=attempt)
            if job["name"] == f"Build {artifact}"
        )

    def rerun(self, run: SourceRun, state: str, *, again: Sequence[int] = ()) -> None:
        """Start another attempt of *run*, which is ``in_progress`` or has a conclusion.

        It runs the jobs whose ids are in *again* once more. Like GitHub, it lists the others
        as copies that have new ids and the names and times of the jobs they stand for.
        """
        running = state == "in_progress"
        previous = self.state[run.run_id]["run_attempt"]
        self.state[run.run_id] = {
            "run_attempt": previous + 1,
            "status": state if running else "completed",
            "conclusion": None if running else state,
        }
        for job in self._jobs(REPO, run.run_id, attempt=previous):
            if job["id"] in again:
                self.job(run, job["name"], attempt=previous + 1, hour=5)
            else:
                copy = self.job(run, job["name"], attempt=previous + 1)
                copy.update({key: job[key] for key in ("conclusion", "started_at", "completed_at")})

    def _run(self, path: str) -> dict[str, Any]:
        prefix = f"repos/{REPO}/actions/runs/"
        assert path.startswith(prefix)
        return {"id": int(path.removeprefix(prefix)), **self.state[int(path.removeprefix(prefix))]}

    def _jobs(self, repo: str, run_id: int, *, attempt: int) -> list[dict[str, Any]]:
        assert repo == REPO
        return [
            job
            for job in self.jobs.values()
            if (job["run_id"], job["run_attempt"]) == (run_id, attempt)
        ]

    def _entry(self, run: SourceRun, name: str, **fields: Any) -> dict[str, Any]:
        self._next_id += 1
        return {
            "id": self._next_id,
            "name": name,
            "expired": False,
            "created_at": "2026-11-02T03:10:00Z",
            "workflow_run": {"id": run.run_id, "head_branch": "master", "head_sha": COMMIT},
            **fields,
        }

    def add(
        self,
        run: SourceRun,
        artifact: str,
        file: str,
        data: bytes,
        metadata: dict[str, Any] | None,
        *,
        sidecar: str | None = None,
        attested: bool = True,
        companions: Mapping[str, bytes] | None = None,
        **fields: Any,
    ) -> dict[str, Any]:
        """Upload a package artifact as attest-and-upload does: every file at the archive root.

        A build attests the package, its build metadata and its *companions*; whoever uploads
        in its place cannot.
        """
        entry = self._entry(run, artifact, **fields)
        digest = hashlib.sha256(data).hexdigest()
        record = b"" if metadata is None else json.dumps(metadata).encode()
        with zipfile.ZipFile(self.store / f"{entry['id']}.zip", "w") as archive:
            archive.writestr(file, data)
            archive.writestr(f"{file}.sha256", sidecar or f"{digest}  {file}\n")
            archive.writestr(f"{artifact}.dependencies.cdx.json", "{}")
            if metadata is not None:
                archive.writestr(f"{file}.build.json", record)
            for name, content in (companions or {}).items():
                archive.writestr(name, content)
        if attested:
            self.attested |= {digest, hashlib.sha256(record).hexdigest()}
            self.attested |= {hashlib.sha256(c).hexdigest() for c in (companions or {}).values()}
        self.listing[run.run_id].append(entry)
        return entry

    def replace(
        self,
        run: SourceRun,
        artifact: str,
        changes: dict[str, Any] | None = None,
        *,
        data: bytes | None = None,
        sidecar: str | None = None,
        attested: bool = True,
        companions: Mapping[str, bytes] | None = None,
    ) -> None:
        """Upload *artifact* again with build metadata fields, content or checksum changed.

        And with *companions* beside the package.
        """
        index = next(i for i, e in enumerate(self.listing[run.run_id]) if e["name"] == artifact)
        old = self.listing[run.run_id].pop(index)
        file = next(f for a, f, _ in ARTIFACTS[run.run_id] if a == artifact)
        with zipfile.ZipFile(self.store / f"{old['id']}.zip") as archive:
            data = archive.read(file) if data is None else data
            metadata = json.loads(archive.read(f"{file}.build.json"))
        for key, value in (changes or {}).items():
            if value is None:
                metadata.pop(key, None)
            else:
                metadata[key] = value
        self.add(
            run,
            artifact,
            file,
            data,
            metadata,
            sidecar=sidecar,
            attested=attested,
            companions=companions,
        )

    def unattest(self, run: SourceRun, artifact: str, member: str) -> None:
        """Take away the attestation of the file *member* of *artifact*."""
        entry = next(e for e in self.listing[run.run_id] if e["name"] == artifact)
        with zipfile.ZipFile(self.store / f"{entry['id']}.zip") as archive:
            self.attested.discard(hashlib.sha256(archive.read(member)).hexdigest())

    def _list(self, repo: str, run_id: int) -> list[dict[str, Any]]:
        assert repo == REPO
        return self.listing[run_id]

    def _download(self, repo: str, artifact_id: int, destination: Path) -> None:
        assert repo == REPO
        self.meanwhile(artifact_id)
        self.downloads.append(artifact_id)
        shutil.copyfile(self.store / f"{artifact_id}.zip", destination)

    def _gh(self, *args: str, **_: object):
        self.verified.append(list(args))
        with open(args[2], "rb") as subject:
            missing = hashlib.sha256(subject.read()).hexdigest() not in self.attested
        return completed(returncode=1 if missing else 0, stderr="no attestations" * missing)


@pytest.fixture
def runs(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Runs:
    return Runs(tmp_path, monkeypatch)


def _fetch(tmp_path: Path) -> publish_set.PublishSet | None:
    return fetch_set(REPO, REF, COMMIT, RUNS, tmp_path / "set")


def test_fetch_keeps_exactly_the_marked_packages_with_their_checksums(
    runs: Runs, tmp_path: Path
) -> None:
    fetched = _fetch(tmp_path)

    assert fetched is not None
    assert [package.name for package in fetched.packages] == PUBLISHED
    files = sorted(path.name for path in (tmp_path / "set").iterdir())
    assert files == sorted(
        [*PUBLISHED, *(f"{name}.sha256" for name in PUBLISHED), "publish-set.json"]
    )
    assert not (tmp_path / "set.download").exists()
    apk = next(package for package in fetched.packages if package.name == "QGroundControl.apk")
    # The Android workflow's other APK artifacts are not marked.
    assert (tmp_path / "set/QGroundControl.apk").read_bytes() == (
        b"Android QGroundControl-linux-arm64-v8a-armeabi-v7a"
    )
    assert (apk.workflow, apk.run_id) == ("Android", ANDROID.run_id)


def test_every_package_needs_an_attestation_from_its_workflow_ref_and_commit(
    runs: Runs, tmp_path: Path
) -> None:
    _fetch(tmp_path)

    # The package and the build metadata that marks it.
    assert len(runs.verified) == 2 * len(PUBLISHED)
    dmg = next(args for args in runs.verified if args[2].endswith("/QGroundControl.dmg"))
    assert dmg[:2] == ["attestation", "verify"]
    assert dmg[3:] == [
        "--repo",
        REPO,
        "--signer-workflow",
        f"{REPO}/.github/workflows/macos.yml",
        "--source-ref",
        REF,
        "--source-digest",
        COMMIT,
        "--signer-digest",
        COMMIT,
        "--predicate-type",
        "https://slsa.dev/provenance/v1",
    ]
    macos = [args for args in runs.verified if f"{REPO}/{MACOS.path}" in args]
    assert [args[3:] for args in macos] == [dmg[3:]] * 2
    assert sum(args[2].endswith(".build.json") for args in macos) == 1

    runs.unattest(MACOS, "QGroundControl", "QGroundControl.dmg")
    with pytest.raises(PublishSetError, match=r": QGroundControl\.dmg has no build provenance"):
        _fetch(tmp_path)


def test_the_build_metadata_that_marks_a_package_needs_the_same_attestation(
    runs: Runs, tmp_path: Path
) -> None:
    runs.unattest(MACOS, "QGroundControl", "QGroundControl.dmg.build.json")

    with pytest.raises(PublishSetError) as excinfo:
        _fetch(tmp_path)

    assert str(excinfo.value) == (
        f"MacOS run {MACOS.run_id}, artifact QGroundControl: the build metadata "
        "QGroundControl.dmg.build.json has no build provenance attestation from "
        f".github/workflows/macos.yml for {REF} at {COMMIT}: no attestations"
    )
    assert not (tmp_path / "set/QGroundControl.dmg").exists()


def test_a_rewritten_marker_cannot_shrink_the_set(runs: Runs, tmp_path: Path) -> None:
    # Someone who can replace a run's artifacts deletes the aarch64 AppImage and uploads the
    # x86_64 one again: the same bytes, whose attestation holds, and a marker that lists it alone.
    x86_64 = "QGroundControl-x86_64.AppImage"
    listing = runs.listing[LINUX.run_id]
    runs.listing[LINUX.run_id] = [e for e in listing if e["name"] != "QGroundControl-aarch64"]
    shrunk = {"publish": _marker(LINUX, x86_64, [x86_64])}
    runs.replace(LINUX, "QGroundControl-x86_64", shrunk, attested=False)

    with pytest.raises(
        PublishSetError,
        match=rf"the build metadata {x86_64}\.build\.json has no build provenance attestation",
    ):
        _fetch(tmp_path)
    assert not (tmp_path / "set/publish-set.json").exists()


def test_runs_of_workflows_that_publish_for_themselves_carry_no_marker(
    runs: Runs, tmp_path: Path, gh_output: Path, gh_summary: Path
) -> None:
    # Upstream master before this change wrote schema 1 metadata; older branches wrote none.
    for run in RUNS:
        for entry in list(runs.listing[run.run_id]):
            runs.listing[run.run_id].remove(entry)
        for artifact, file, _ in ARTIFACTS[run.run_id]:
            data = artifact.encode()
            old = {**_metadata(run, file, data, publish=False), "schema_version": 1}
            runs.add(run, artifact, file, data, old if run is not MACOS else None)

    argv = ["fetch", "--repo", REPO, "--ref", REF, "--commit", COMMIT]
    assert main([*argv, "--runs", RUNS_JSON, "--out", str(tmp_path / "set")]) == 0

    assert read_github_file(gh_output) == {"publish": "false"}
    assert "Nothing is published" in gh_summary.read_text(encoding="utf-8")
    assert not (tmp_path / "set/publish-set.json").exists()
    assert runs.verified == []


def test_fetch_reports_the_verified_set(
    runs: Runs, tmp_path: Path, gh_output: Path, gh_summary: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setenv("GITHUB_REPOSITORY", REPO)
    monkeypatch.setenv("PUBLISH_REF", REF)
    monkeypatch.setenv("PUBLISH_COMMIT", COMMIT)
    monkeypatch.setenv("PUBLISH_RUNS", RUNS_JSON)

    assert main(["fetch", "--out", str(tmp_path / "set")]) == 0

    assert read_github_file(gh_output) == {"publish": "true"}
    summary = gh_summary.read_text(encoding="utf-8")
    assert "6 packages verified" in summary
    assert all(f"`{name}`" in summary for name in PUBLISHED)


def _fail(runs: Runs, run: SourceRun, job: str = "Boot test") -> list[SourceRun]:
    """Have *run* conclude ``failure`` because *job*, which builds no package, failed.

    Returns the four runs as the decision approves them for a branch.
    """
    runs.job(run, job)["conclusion"] = "failure"
    runs.state[run.run_id]["conclusion"] = "failure"
    return _approved(run, 1, "failure")


def _fetch_command(tmp_path: Path, approved: Sequence[SourceRun], ref: str = REF) -> list[str]:
    argv = ["fetch", "--repo", REPO, "--ref", ref, "--commit", COMMIT]
    return [*argv, "--runs", _runs_json(approved), "--out", str(tmp_path / "set")]


def test_a_branch_is_published_although_a_job_that_builds_no_package_failed(
    runs: Runs, tmp_path: Path, gh_output: Path, gh_summary: Path
) -> None:
    # Upstream's Stable_V5.1 run 40772256141: both APKs were uploaded, then another job failed.
    approved = _fail(runs, ANDROID, "Android (linux-emulator)")

    assert main(_fetch_command(tmp_path, approved)) == 0

    assert read_github_file(gh_output) == {"publish": "true"}
    loaded = load_set(tmp_path / "set", ref=REF, commit=COMMIT)
    assert [package.name for package in loaded.packages] == PUBLISHED
    # The publishing jobs are told how each workflow concluded, and so is whoever reads this.
    assert {run.workflow: run.conclusion for run in loaded.runs} == {
        "Linux": "success",
        "Windows": "success",
        "MacOS": "success",
        "Android": "failure",
    }
    summary = gh_summary.read_text(encoding="utf-8")
    runs_page = f"https://github.com/{REPO}/actions/runs"
    assert (
        "| Android build | packages built; workflow concluded `failure` "
        f"([run {ANDROID.run_id}, attempt 1]({runs_page}/{ANDROID.run_id}/attempts/1)) |"
    ) in summary
    assert (
        "| Linux build | packages built; workflow passed "
        f"([run {LINUX.run_id}, attempt 1]({runs_page}/{LINUX.run_id}/attempts/1)) |"
    ) in summary


def _assert_nothing_to_publish_yet(
    gh_output: Path, gh_summary: Path, capsys: pytest.CaptureFixture[str], reason: str
) -> None:
    """A build that failed leaves nothing to publish, which is no error of this workflow."""
    assert read_github_file(gh_output) == {"publish": "false"}
    out = capsys.readouterr().out
    assert "::error" not in out
    assert f"::notice title=Nothing to publish yet::{reason}" in out
    summary = gh_summary.read_text(encoding="utf-8")
    assert "| Result | ⏭️ nothing to publish yet |" in summary
    assert f"| Reason | {reason} |" in summary


def test_a_failed_run_that_lacks_a_package_ends_quietly_with_nothing_to_publish(
    runs: Runs,
    tmp_path: Path,
    gh_output: Path,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    # The aarch64 leg failed before it uploaded. The x86_64 AppImage is there and lists both.
    listing = runs.listing[LINUX.run_id]
    runs.listing[LINUX.run_id] = [e for e in listing if e["name"] != "QGroundControl-aarch64"]
    runs.job_of(LINUX, "QGroundControl-aarch64")["conclusion"] = "failure"
    runs.state[LINUX.run_id]["conclusion"] = "failure"

    assert main(_fetch_command(tmp_path, _approved(LINUX, 1, "failure"))) == 0

    _assert_nothing_to_publish_yet(
        gh_output,
        gh_summary,
        capsys,
        f"Linux run {LINUX.run_id} lacks QGroundControl-aarch64.AppImage, which its workflow "
        "marks: a commit is published with every package or not at all. Linux run "
        f"{LINUX.run_id} concluded `failure`.",
    )
    assert not (tmp_path / "set/publish-set.json").exists()
    # The failed run was read first, so the other platforms' packages were not downloaded.
    linux = {entry["id"] for entry in runs.listing[LINUX.run_id]}
    assert runs.downloads and set(runs.downloads) <= linux


def test_a_failed_run_whose_package_was_built_by_the_job_that_failed_ends_quietly(
    runs: Runs,
    tmp_path: Path,
    gh_output: Path,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    # The dmg was uploaded, and a later step of the same job failed.
    job = runs.job_of(MACOS, "QGroundControl")
    job["conclusion"] = "failure"
    runs.state[MACOS.run_id]["conclusion"] = "failure"

    assert main(_fetch_command(tmp_path, _approved(MACOS, 1, "failure"))) == 0

    _assert_nothing_to_publish_yet(
        gh_output,
        gh_summary,
        capsys,
        f"MacOS run {MACOS.run_id}, artifact QGroundControl: QGroundControl.dmg was built by "
        f"the job 'Build QGroundControl' ({job['id']}) of attempt 1, which failed. MacOS run "
        f"{MACOS.run_id} concluded `failure`.",
    )


@pytest.mark.parametrize("left", [[], ["QGroundControl-mac-arm64-v8a"]])
def test_a_failed_run_without_any_of_its_packages_ends_quietly(
    runs: Runs,
    tmp_path: Path,
    gh_output: Path,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
    left: list[str],
) -> None:
    # The job that builds the published APK failed: no marker says what the workflow marks.
    # Another leg may have uploaded an APK that is not published.
    listing = runs.listing[ANDROID.run_id]
    runs.listing[ANDROID.run_id] = [entry for entry in listing if entry["name"] in left]
    runs.state[ANDROID.run_id]["conclusion"] = "failure"

    assert main(_fetch_command(tmp_path, _approved(ANDROID, 1, "failure"))) == 0

    _assert_nothing_to_publish_yet(
        gh_output,
        gh_summary,
        capsys,
        f"no package of Android run {ANDROID.run_id} is marked for publishing, but other "
        f"workflows of the commit marked theirs. Android run {ANDROID.run_id} concluded "
        "`failure`.",
    )
    # Known as soon as another workflow's first package is: the rest is not downloaded.
    assert len(runs.downloads) == len(left) + 2


def test_runs_that_all_failed_without_a_package_end_quietly(
    runs: Runs, tmp_path: Path, gh_output: Path
) -> None:
    approved = []
    for run in RUNS:
        runs.listing[run.run_id] = []
        runs.state[run.run_id]["conclusion"] = "failure"
        approved.append(SourceRun(run.workflow, run.path, run.run_id, 1, "failure"))

    with pytest.raises(NotBuiltError, match=r"^none of the commit's runs built a package\. Linux"):
        fetch_set(REPO, REF, COMMIT, approved, tmp_path / "set")
    assert main(_fetch_command(tmp_path, approved)) == 0
    assert read_github_file(gh_output) == {"publish": "false"}


def test_a_failed_build_is_no_excuse_for_a_package_that_does_not_verify(
    runs: Runs, tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    # Only a missing package, or one from a job that did not pass, is the build's failure.
    approved = _fail(runs, MACOS)
    runs.unattest(MACOS, "QGroundControl", "QGroundControl.dmg")

    assert main(_fetch_command(tmp_path, approved)) == 1

    assert "QGroundControl.dmg has no build provenance attestation" in capsys.readouterr().out


@pytest.mark.parametrize(
    ("ref", "conclusion"),
    [
        (REF, "cancelled"),
        (REF, "timed_out"),
        (REF, "skipped"),
        (REF, "neutral"),
        (REF, "action_required"),
        (REF, "stale"),
        (REF, "startup_failure"),
        # A release needs every job to pass.
        ("refs/tags/v5.1.5", "failure"),
    ],
)
def test_no_ref_is_published_from_a_run_that_was_stopped_and_no_tag_from_one_that_failed(
    runs: Runs, tmp_path: Path, ref: str, conclusion: str
) -> None:
    runs.state[ANDROID.run_id]["conclusion"] = conclusion

    with pytest.raises(
        PublishSetError,
        match=rf"^Android run {ANDROID.run_id} concluded `{conclusion}`: {ref} is not published",
    ) as excinfo:
        fetch_set(REPO, ref, COMMIT, _approved(ANDROID, 1, conclusion), tmp_path / "set")

    assert not isinstance(excinfo.value, NotBuiltError)
    assert runs.downloads == []


def test_a_failed_run_that_was_re_run_since_it_was_approved_publishes_nothing(
    runs: Runs, tmp_path: Path, gh_output: Path, gh_summary: Path
) -> None:
    # The failed job is re-run while the commit's packages are verified. The completion of
    # the re-run starts a decision of its own, which finds the commit published or not.
    approved = _fail(runs, ANDROID)
    runs.rerun(ANDROID, "success")

    assert main(_fetch_command(tmp_path, approved)) == 0

    assert read_github_file(gh_output) == {"publish": "false"}
    assert (
        f"Android run {ANDROID.run_id} was approved at attempt 1 (failure), but its latest "
        "attempt is 2 (success)"
    ) in gh_summary.read_text(encoding="utf-8")


def test_a_run_is_approved_with_the_conclusion_it_had(runs: Runs, tmp_path: Path) -> None:
    # The decision saw a run that passed; the run now reports that attempt as failed.
    runs.state[ANDROID.run_id]["conclusion"] = "failure"

    with pytest.raises(RerunError, match=r"attempt 1 \(success\), but its latest attempt is 1"):
        _fetch(tmp_path)


def test_a_checksum_mismatch_fails_the_set(
    runs: Runs, tmp_path: Path, gh_output: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    wrong = f"{'0' * 64}  QGroundControl.dmg\n"
    runs.replace(MACOS, "QGroundControl", sidecar=wrong)
    argv = ["fetch", "--repo", REPO, "--ref", REF, "--commit", COMMIT]

    assert main([*argv, "--runs", RUNS_JSON, "--out", str(tmp_path / "set")]) == 1

    assert "SHA-256 checksum mismatch" in capsys.readouterr().out
    assert gh_output.read_text() == ""
    assert not (tmp_path / "set/publish-set.json").exists()


def test_a_package_changed_after_its_metadata_was_written_fails(runs: Runs, tmp_path: Path) -> None:
    # The sidecar matches the new bytes, the build metadata still describes the old ones.
    data = b"swapped"
    sidecar = f"{hashlib.sha256(data).hexdigest()}  QGroundControl.dmg\n"
    runs.replace(MACOS, "QGroundControl", data=data, sidecar=sidecar)

    with pytest.raises(PublishSetError, match="not the file its build metadata describes"):
        _fetch(tmp_path)


@pytest.mark.parametrize(
    ("field", "value"),
    [
        ("repository", "someone/qgroundcontrol"),
        ("commit", "13faea0245c22a1d763b96721fe5c1711edc52e2"),
        ("ref", "refs/tags/master"),
        ("ref", "refs/heads/Stable_V5.1"),
        ("event", "pull_request"),
        ("event", "workflow_dispatch"),
        ("workflow", "Linux"),
        ("run_id", "1"),
        ("schema_version", 1),
        ("schema_version", 3),
        ("job_id", None),
        ("job_id", "build"),
        ("job_id", 13113000001),
    ],
)
def test_a_marker_that_disagrees_with_its_run_fails_the_set(
    runs: Runs, tmp_path: Path, field: str, value: object
) -> None:
    runs.replace(MACOS, "QGroundControl", {field: value})

    with pytest.raises(PublishSetError, match=rf"MacOS run .*build metadata {field} is "):
        _fetch(tmp_path)


@pytest.mark.parametrize(
    "name",
    [
        "../QGroundControl.dmg",
        "latest/QGroundControl.dmg",
        "QGroundControl.dmg.sha256",
        "published.json",
        "QGroundControl.sh",
        "",
        None,
        ["QGroundControl.dmg"],
    ],
)
def test_only_a_public_package_name_can_be_published(
    runs: Runs, tmp_path: Path, name: object
) -> None:
    runs.replace(MACOS, "QGroundControl", {"publish": {"name": name}})

    with pytest.raises(PublishSetError, match="is not a package name that the default branch"):
        _fetch(tmp_path)
    assert not list((tmp_path / "set").glob("*.dmg"))


def test_a_package_is_published_under_its_own_file_name_only(runs: Runs, tmp_path: Path) -> None:
    runs.replace(MACOS, "QGroundControl", {"publish": {"name": "QGroundControl-x86_64.AppImage"}})

    with pytest.raises(PublishSetError, match=r"is not the package file 'QGroundControl\.dmg'"):
        _fetch(tmp_path)


def test_two_packages_cannot_take_one_name(runs: Runs, tmp_path: Path) -> None:
    # The Android workflow's second APK artifact claims the published name too.
    runs.replace(
        ANDROID, "QGroundControl-mac-arm64-v8a", {"publish": _marker(ANDROID, "QGroundControl.apk")}
    )

    with pytest.raises(PublishSetError, match=r"two packages ask for the name QGroundControl\.apk"):
        _fetch(tmp_path)


def test_a_stable_branch_publishes_the_packages_its_own_workflows_mark(
    runs: Runs, tmp_path: Path
) -> None:
    # A branch cut before the aarch64 AppImage existed: its Linux workflow marks one package and
    # says so. The default branch's longer list does not make its release incomplete.
    x86_64 = "QGroundControl-x86_64.AppImage"
    listing = runs.listing[LINUX.run_id]
    runs.listing[LINUX.run_id] = [e for e in listing if e["name"] != "QGroundControl-aarch64"]
    runs.replace(LINUX, "QGroundControl-x86_64", {"publish": _marker(LINUX, x86_64, [x86_64])})

    fetched = _fetch(tmp_path)

    assert fetched is not None
    older = [name for name in PUBLISHED if name != "QGroundControl-aarch64.AppImage"]
    assert [package.name for package in fetched.packages] == older
    # The publishing jobs get exactly these five.
    loaded = load_set(tmp_path / "set", ref=REF, commit=COMMIT)
    assert [name for _, name in loaded.files()] == [
        name for package in older for name in (package, f"{package}.sha256")
    ]


def test_a_partial_set_is_never_published(runs: Runs, tmp_path: Path) -> None:
    # Every workflow must contribute, whatever the others list.
    runs.replace(MACOS, "QGroundControl", {"publish": None})

    with pytest.raises(PublishSetError, match="no package of MacOS run 41380314284 is marked"):
        _fetch(tmp_path)
    assert not (tmp_path / "set/publish-set.json").exists()


@pytest.mark.parametrize(
    ("run", "artifact", "file"),
    [
        (LINUX, "QGroundControl-aarch64", "QGroundControl-aarch64.AppImage"),
        (WINDOWS, "QGroundControl-installer-ARM64", "QGroundControl-installer-ARM64.exe"),
    ],
)
def test_a_workflow_must_provide_every_package_its_markers_list(
    runs: Runs,
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
    run: SourceRun,
    artifact: str,
    file: str,
) -> None:
    # One leg's artifact is gone; the workflow's other package is there, verifies, and lists
    # both. Published like this, the release would lose the file and S3 would keep the previous
    # commit's.
    runs.listing[run.run_id] = [e for e in runs.listing[run.run_id] if e["name"] != artifact]
    argv = ["fetch", "--repo", REPO, "--ref", REF, "--commit", COMMIT, "--runs", RUNS_JSON]

    assert main([*argv, "--out", str(tmp_path / "set")]) == 1

    assert f"{run.workflow} run {run.run_id} lacks {file}, which its workflow marks" in (
        capsys.readouterr().out
    )
    assert not (tmp_path / "set/publish-set.json").exists()


def test_a_package_its_workflow_s_inventory_does_not_list_fails_the_set(
    runs: Runs, tmp_path: Path
) -> None:
    # Both AppImages are marked, and both say that the workflow marks the x86_64 one only.
    x86_64, aarch64 = "QGroundControl-x86_64.AppImage", "QGroundControl-aarch64.AppImage"
    runs.replace(LINUX, "QGroundControl-x86_64", {"publish": _marker(LINUX, x86_64, [x86_64])})
    runs.replace(LINUX, "QGroundControl-aarch64", {"publish": _marker(LINUX, aarch64, [x86_64])})

    with pytest.raises(
        PublishSetError,
        match=rf"{aarch64} of Linux run {LINUX.run_id} is marked for publishing, but is not",
    ):
        _fetch(tmp_path)


def test_the_packages_of_a_workflow_must_agree_on_what_it_publishes(
    runs: Runs, tmp_path: Path
) -> None:
    # The x86_64 AppImage lists itself only; the aarch64 one lists both.
    x86_64 = "QGroundControl-x86_64.AppImage"
    runs.replace(LINUX, "QGroundControl-x86_64", {"publish": _marker(LINUX, x86_64, [x86_64])})

    with pytest.raises(PublishSetError) as excinfo:
        _fetch(tmp_path)

    assert str(excinfo.value) == (
        f"the packages of Linux run {LINUX.run_id} disagree on what the workflow publishes: "
        "QGroundControl-aarch64.AppImage lists QGroundControl-aarch64.AppImage, "
        "QGroundControl-x86_64.AppImage, and QGroundControl-x86_64.AppImage lists "
        "QGroundControl-x86_64.AppImage"
    )


def test_a_name_the_default_branch_does_not_know_is_never_published(
    runs: Runs, tmp_path: Path
) -> None:
    # A branch marks the cross-compiled installer and lists it with the other two. The file
    # name is well formed, but the default branch, which publishes, has no such package.
    cross = "QGroundControl-installer-AMD64-ARM64"
    marked = sorted([*_marker(WINDOWS, "")["packages"], f"{cross}.exe"])
    for artifact, file, _ in ARTIFACTS[WINDOWS.run_id]:
        runs.replace(WINDOWS, artifact, {"publish": _marker(WINDOWS, file, marked)})

    with pytest.raises(
        PublishSetError,
        match=rf"'{cross}\.exe' is not a package name that the default branch publishes",
    ):
        _fetch(tmp_path)
    assert not (tmp_path / f"set/{cross}.exe").exists()


@pytest.mark.parametrize(
    ("change", "reason"),
    [
        ({"workflow": "Linux"}, "names the workflow 'Linux', not 'MacOS'"),
        ({"workflow": None}, "names the workflow None, not 'MacOS'"),
        ({"packages": None}, "does not list the packages its workflow marks"),
        ({"packages": []}, "does not list the packages its workflow marks"),
        ({"packages": "QGroundControl.dmg"}, "does not list the packages its workflow marks"),
        ({"packages": ["QGroundControl.dmg"] * 2}, "does not list the packages its workflow"),
        ({"packages": ["QGroundControl.dmg", 1]}, "does not list the packages its workflow"),
    ],
)
def test_a_marker_must_name_its_workflow_and_the_packages_that_workflow_marks(
    runs: Runs, tmp_path: Path, change: dict[str, Any], reason: str
) -> None:
    marker = {**_marker(MACOS, "QGroundControl.dmg"), **change}
    runs.replace(MACOS, "QGroundControl", {"publish": marker})

    with pytest.raises(PublishSetError, match=f"the publish marker of QGroundControl.dmg {reason}"):
        _fetch(tmp_path)


@pytest.mark.parametrize("state", ["in_progress", "failure", "success"])
def test_a_run_that_was_re_run_after_it_was_approved_publishes_nothing(
    runs: Runs,
    tmp_path: Path,
    gh_output: Path,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
    state: str,
) -> None:
    # Even a re-run that passed: its completion starts a decision that approves that attempt.
    runs.rerun(WINDOWS, state)
    argv = ["fetch", "--repo", REPO, "--ref", REF, "--commit", COMMIT, "--runs", RUNS_JSON]

    assert main([*argv, "--out", str(tmp_path / "set")]) == 0

    assert read_github_file(gh_output) == {"publish": "false"}
    assert runs.downloads == []
    reason = f"was approved at attempt 1 (success), but its latest attempt is 2 ({state})"
    assert f"Windows run {WINDOWS.run_id} {reason}" in gh_summary.read_text(encoding="utf-8")
    assert "::warning title=A build was re-run" in capsys.readouterr().out


def test_a_re_run_that_starts_during_verification_stops_it_before_the_set_is_handed_over(
    runs: Runs, tmp_path: Path, gh_output: Path
) -> None:
    # The re-run starts once the last artifact is being downloaded: every check so far passed.
    last = runs.listing[ANDROID.run_id][-1]["id"]
    runs.meanwhile = lambda artifact: runs.rerun(LINUX, "in_progress") if artifact == last else None

    with pytest.raises(RerunError, match=r"Linux run 41380314322 was approved at attempt 1 \("):
        _fetch(tmp_path)
    assert len(runs.downloads) == sum(len(artifacts) for artifacts in ARTIFACTS.values())
    assert not (tmp_path / "set/publish-set.json").exists()


def test_an_approved_attempt_must_still_be_reported_as_passed(runs: Runs, tmp_path: Path) -> None:
    runs.state[MACOS.run_id].update(status="in_progress", conclusion=None)

    with pytest.raises(RerunError, match=r"its latest attempt is 1 \(in_progress\)"):
        _fetch(tmp_path)
    assert runs.downloads == []


def test_a_package_built_by_a_later_attempt_than_the_approved_one_fails_the_set(
    runs: Runs, tmp_path: Path
) -> None:
    # A re-run uploads under the approved run's id; its attempt has not passed.
    file, data = "QGroundControl-x86_64.AppImage", b"attempt 2, still running"
    metadata = {**_metadata(LINUX, file, data, publish=True), "run_attempt": "2"}
    runs.add(
        LINUX, "QGroundControl-x86_64", file, data, metadata, created_at="2026-11-02T05:00:00Z"
    )

    with pytest.raises(PublishSetError, match="run_attempt is '2', but attempt 1 of the run"):
        _fetch(tmp_path)


def test_a_package_marked_without_a_job_id_fails_the_publish_and_says_why(
    runs: Runs, tmp_path: Path, gh_output: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    # Its build could not tell which job it was, warned, and passed. The failure is here.
    runs.replace(LINUX, "QGroundControl-x86_64", {"job_id": ""})
    argv = ["fetch", "--repo", REPO, "--ref", REF, "--commit", COMMIT, "--runs", RUNS_JSON]

    assert main([*argv, "--out", str(tmp_path / "set")]) == 1

    assert (
        f"::error title=Packages failed verification::Linux run {LINUX.run_id}, artifact "
        "QGroundControl-x86_64: QGroundControl-x86_64.AppImage is marked for publishing without "
        "the id of the job that built it (build metadata job_id is ''), so it cannot be bound "
        "to a job that passed. Its build warned that attest-and-upload got no usable job-id."
    ) in capsys.readouterr().out
    assert gh_output.read_text() == ""
    assert not (tmp_path / "set/publish-set.json").exists()


def _rebuild(runs: Runs, run: SourceRun, artifact: str, attempt: int) -> dict[str, Any]:
    """Upload *artifact* as the job that *attempt* of *run* ran again builds it."""
    file = next(f for a, f, _ in ARTIFACTS[run.run_id] if a == artifact)
    data = f"{run.workflow} {artifact}, attempt {attempt}".encode()
    job = runs.job_of(run, artifact, attempt)
    metadata = _metadata(run, file, data, publish=True, job=job)
    return runs.add(run, artifact, file, data, metadata, created_at="2026-11-02T05:09:00Z")


def test_a_re_run_of_the_failed_jobs_keeps_the_packages_of_the_jobs_that_had_passed(
    runs: Runs, tmp_path: Path
) -> None:
    # Attempt 1 built x86_64 and failed aarch64 after its upload; attempt 2 ran only that job
    # again and passed. It lists the x86_64 job as a copy, under an id no package names.
    failed = runs.job_of(LINUX, "QGroundControl-aarch64")
    failed["conclusion"] = "failure"
    runs.rerun(LINUX, "success", again=[failed["id"]])
    _rebuild(runs, LINUX, "QGroundControl-aarch64", 2)

    fetched = fetch_set(REPO, REF, COMMIT, _approved(LINUX, 2), tmp_path / "set")

    assert fetched is not None
    assert [package.name for package in fetched.packages] == PUBLISHED
    assert (tmp_path / "set/QGroundControl-x86_64.AppImage").read_bytes() == (
        b"Linux QGroundControl-x86_64"
    )
    assert (tmp_path / "set/QGroundControl-aarch64.AppImage").read_bytes() == (
        b"Linux QGroundControl-aarch64, attempt 2"
    )


@pytest.mark.parametrize(
    ("first", "reason"),
    [
        ("failure", "which failed"),
        ("success", "which attempt 2 ran again"),
    ],
)
def test_an_earlier_attempt_s_package_never_stands_in_for_the_approved_attempt_s(
    runs: Runs, tmp_path: Path, first: str, reason: str
) -> None:
    # Upstream run 42969994287: attempt 1 uploaded the x86_64 AppImage ten seconds before its
    # job failed, and attempt 2 built it again. With attempt 2's artifact deleted, attempt 1's
    # is the newest of that name, and the run still reports a successful attempt 2.
    job = runs.job_of(LINUX, "QGroundControl-x86_64")
    job["conclusion"] = first
    runs.rerun(LINUX, "success", again=[job["id"]])
    rebuilt = _rebuild(runs, LINUX, "QGroundControl-x86_64", 2)
    runs.listing[LINUX.run_id].remove(rebuilt)

    with pytest.raises(PublishSetError) as excinfo:
        fetch_set(REPO, REF, COMMIT, _approved(LINUX, 2), tmp_path / "set")

    assert str(excinfo.value) == (
        f"Linux run {LINUX.run_id}, artifact QGroundControl-x86_64: "
        "QGroundControl-x86_64.AppImage was built by the job 'Build QGroundControl-x86_64' "
        f"({job['id']}) of attempt 1, {reason}. The artifact QGroundControl-x86_64 of "
        "attempt 2, which was approved, is missing: re-run the job to build the package again"
    )
    assert not (tmp_path / "set/publish-set.json").exists()


def test_a_package_of_a_job_that_failed_is_not_published(runs: Runs, tmp_path: Path) -> None:
    # The run passed all the same: a job may be allowed to fail.
    job = runs.job_of(MACOS, "QGroundControl")
    job["conclusion"] = "failure"

    with pytest.raises(
        PublishSetError,
        match=rf"QGroundControl\.dmg was built by the job 'Build QGroundControl' \({job['id']}\) "
        "of attempt 1, which failed$",
    ):
        _fetch(tmp_path)


@pytest.mark.parametrize("attempt", [1, 2])
def test_a_package_must_be_built_by_a_job_of_the_run_it_is_published_from(
    runs: Runs, tmp_path: Path, attempt: int
) -> None:
    # Attested metadata of the same commit, naming a job of the Windows run. The job is
    # looked for among those of this run, in the attempt that built the package.
    other = runs.job_of(WINDOWS, "QGroundControl-installer-AMD64")
    runs.replace(MACOS, "QGroundControl", {"job_id": str(other["id"])})
    if attempt == 2:
        runs.rerun(MACOS, "success")

    with pytest.raises(
        PublishSetError,
        match=rf"job {other['id']}, which built QGroundControl\.dmg, is not a job of attempt 1 "
        f"of run {MACOS.run_id}",
    ):
        fetch_set(REPO, REF, COMMIT, _approved(MACOS, attempt), tmp_path / "set")


def test_a_package_names_a_job_of_the_attempt_that_built_it(runs: Runs, tmp_path: Path) -> None:
    # The metadata says attempt 2 and names the job that attempt 1 ran, which passed.
    first = runs.job_of(MACOS, "QGroundControl")
    runs.rerun(MACOS, "success", again=[first["id"]])
    runs.replace(MACOS, "QGroundControl", {"run_attempt": "2"})

    with pytest.raises(
        PublishSetError,
        match=rf"job {first['id']}, which built QGroundControl\.dmg, is not a job of attempt 2",
    ):
        fetch_set(REPO, REF, COMMIT, _approved(MACOS, 2), tmp_path / "set")


def test_a_job_that_cannot_be_told_from_another_is_not_taken_for_carried_over(
    runs: Runs, tmp_path: Path
) -> None:
    # Both Linux legs display as `Build`, and they started and ended in the same seconds.
    # Attempt 2 runs the aarch64 leg again and carries the x86_64 leg over, whose copy has the
    # name and times of both. Attempt 2's aarch64 artifact is deleted: attempt 1's is the newest.
    aarch64 = runs.job_of(LINUX, "QGroundControl-aarch64")
    x86_64 = runs.job_of(LINUX, "QGroundControl-x86_64")
    aarch64["name"] = x86_64["name"] = "Build"
    runs.rerun(LINUX, "success", again=[aarch64["id"]])

    with pytest.raises(PublishSetError) as excinfo:
        fetch_set(REPO, REF, COMMIT, _approved(LINUX, 2), tmp_path / "set")

    assert str(excinfo.value) == (
        f"Linux run {LINUX.run_id}, artifact QGroundControl-aarch64: the job 'Build' "
        f"({aarch64['id']}) of attempt 1 of run {LINUX.run_id}, which built "
        "QGroundControl-aarch64.AppImage, cannot be identified unambiguously in attempt 2. Jobs "
        "with its name and its start and completion times: 2 in attempt 1, 1 passed in attempt "
        "2; exactly one each would show that attempt 2 did not run it again. Re-run the job to "
        "build the package again"
    )
    assert not (tmp_path / "set/publish-set.json").exists()


@pytest.mark.parametrize(("attempt", "counted"), [(1, "2 in attempt 1"), (2, "1 in attempt 1")])
def test_a_carried_over_job_must_be_the_only_one_with_its_name_and_times(
    runs: Runs, tmp_path: Path, attempt: int, counted: str
) -> None:
    # Attempt 2 ran nothing of the MacOS run again, but two of its passed jobs could be the
    # copy of the dmg's job: another job of that name and those times ran beside it in attempt
    # 1, or appears in attempt 2 only.
    if attempt == 1:
        runs.job(MACOS, "Build QGroundControl")
    runs.rerun(MACOS, "success")
    if attempt == 2:
        runs.job(MACOS, "Build QGroundControl", attempt=2)

    with pytest.raises(
        PublishSetError,
        match=rf"cannot be identified unambiguously in attempt 2\. Jobs with its name and its "
        rf"start and completion times: {counted}, 2 passed in attempt 2;",
    ):
        fetch_set(REPO, REF, COMMIT, _approved(MACOS, 2), tmp_path / "set")


def test_a_copied_job_stands_for_the_original_only_with_its_name_and_times(
    runs: Runs, tmp_path: Path
) -> None:
    # Attempt 2 ran nothing of the dmg's job again, but its copy differs from the original.
    runs.rerun(MACOS, "success")
    copy = runs.job_of(MACOS, "QGroundControl", 2)
    copy["completed_at"] = "2026-11-02T03:11:00Z"

    with pytest.raises(PublishSetError, match="of attempt 1, which attempt 2 ran again"):
        fetch_set(REPO, REF, COMMIT, _approved(MACOS, 2), tmp_path / "set")

    copy.update(completed_at="2026-11-02T03:10:00Z", conclusion="cancelled")
    with pytest.raises(PublishSetError, match="of attempt 1, which attempt 2 ran again"):
        fetch_set(REPO, REF, COMMIT, _approved(MACOS, 2), tmp_path / "set")

    copy["conclusion"] = "success"
    for job in (runs.job_of(MACOS, "QGroundControl", attempt) for attempt in (1, 2)):
        job["started_at"] = None
    with pytest.raises(PublishSetError, match="of attempt 1, which attempt 2 ran again"):
        fetch_set(REPO, REF, COMMIT, _approved(MACOS, 2), tmp_path / "set")


def test_the_runs_to_publish_from_name_their_approved_attempt(
    runs: Runs, tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    listed = json.dumps([{"workflow": r.workflow, "path": r.path, "id": r.run_id} for r in RUNS])
    argv = ["fetch", "--repo", REPO, "--ref", REF, "--commit", COMMIT, "--runs", listed]

    assert main([*argv, "--out", str(tmp_path / "set")]) == 1
    assert "cannot read the workflow runs to publish from: 'attempt'" in capsys.readouterr().out


def test_build_metadata_in_a_subdirectory_is_not_a_marker(runs: Runs, tmp_path: Path) -> None:
    entry = runs.listing[MACOS.run_id].pop()
    data = b"MacOS QGroundControl"
    with zipfile.ZipFile(runs.store / f"{entry['id']}.zip", "w") as archive:
        archive.writestr("QGroundControl.dmg", data)
        archive.writestr(
            "nested/QGroundControl.dmg.build.json",
            json.dumps(_metadata(MACOS, "QGroundControl.dmg", data, publish=True)),
        )
    runs.listing[MACOS.run_id].append(entry)

    with pytest.raises(PublishSetError, match="no package of MacOS run"):
        _fetch(tmp_path)


def test_a_rerun_s_artifact_replaces_the_earlier_attempt_s() -> None:
    # Upstream run 42969994287 (attempt 2) lists both attempts' QGroundControl-x86_64.
    run = SourceRun("Linux", ".github/workflows/linux.yml", 42969994287, 2)
    commit = "13faea0245c22a1d763b96721fe5c1711edc52e2"
    source = {"id": run.run_id, "head_branch": "master", "head_sha": commit}
    listing = [
        {"id": 13871236852, "name": "build-profile-d3n3q95e", "workflow_run": source},
        {
            "id": 13871058804,
            "name": "QGroundControl-x86_64",
            "created_at": "2026-11-11T16:15:45Z",
            "expired": False,
            "workflow_run": source,
        },
        {"id": 13833629591, "name": "coverage-report", "workflow_run": source},
        {
            "id": 13833433482,
            "name": "QGroundControl-x86_64",
            "created_at": "2026-11-11T06:21:47Z",
            "expired": False,
            "workflow_run": source,
        },
    ]

    assert [a["id"] for a in newest_artifacts(listing, run, commit)] == [13871058804]
    assert [a["id"] for a in newest_artifacts(reversed(listing), run, commit)] == [13871058804]


@pytest.mark.parametrize(
    ("ref", "advice"),
    [
        (
            REF,
            "Re-run all jobs of each of these runs to build the packages again: the completion "
            "of the last starts a publish of its own.",
        ),
        # The jobs of a tag's runs upload to the stores and push the builder images, and those
        # of an older Stable branch upload to latest/ themselves: re-running them repeats that.
        (
            "refs/tags/v5.1.5",
            "Do not re-run the jobs of a release tag's runs to build them again: the runs would "
            "repeat their uploads to the stores and their builder images. What was published "
            'of v5.1.5 is on the download server under builds/v5.1.5/: "Going back to an '
            'earlier release" and "Release files" in '
            "docs/en/qgc-dev-guide/release_branching_process.md say how to publish it from "
            "there by hand.",
        ),
    ],
    ids=["branch", "tag"],
)
def test_runs_whose_artifacts_expired_fail_the_request_instead_of_passing_for_an_old_layout(
    runs: Runs,
    tmp_path: Path,
    gh_output: Path,
    capsys: pytest.CaptureFixture[str],
    ref: str,
    advice: str,
) -> None:
    # GitHub drops an expired artifact from its run's listing: upstream's Linux run
    # 36878132470 for v5.1.5, 42 days old, lists `size-metrics` alone, and the Windows run
    # nothing. A request for such a release (force_latest, force_release) changed nothing.
    for run in RUNS:
        runs.listing[run.run_id] = [runs._entry(run, "size-metrics")] * (run is LINUX)
    argv = ["fetch", "--repo", REPO, "--ref", ref, "--commit", COMMIT, "--runs", RUNS_JSON]

    assert main([*argv, "--out", str(tmp_path / "set")]) == 1

    assert (
        f"::error title=Packages failed verification::Linux run {LINUX.run_id}, Windows run "
        f"{WINDOWS.run_id}, MacOS run {MACOS.run_id}, Android run {ANDROID.run_id} passed, but "
        "list no package artifact. GitHub deletes a build's artifacts 30 days after their "
        f"upload, and nothing is published without them. {advice}\n"
    ) in capsys.readouterr().out
    assert gh_output.read_text() == ""
    assert runs.downloads == []


def test_one_run_without_its_artifacts_fails_the_set_before_anything_is_downloaded(
    runs: Runs, tmp_path: Path
) -> None:
    # The other three runs were re-run and have their packages again.
    runs.listing[MACOS.run_id] = []

    with pytest.raises(
        PublishSetError, match=rf"^MacOS run {MACOS.run_id} passed, but lists no package artifact"
    ):
        _fetch(tmp_path)
    assert runs.downloads == []


def test_an_artifact_that_is_listed_as_expired_counts_as_gone() -> None:
    source = {"id": LINUX.run_id, "head_sha": COMMIT}
    expired = {"id": 1, "name": "QGroundControl-x86_64", "expired": True, "workflow_run": source}

    assert newest_artifacts([expired], LINUX, COMMIT) == []


def test_a_foreign_artifact_fails_the_set() -> None:
    source = {"id": LINUX.run_id, "head_sha": COMMIT}
    foreign = {"id": 2, "name": "QGroundControl-x86_64", "workflow_run": {**source, "id": 7}}
    with pytest.raises(PublishSetError, match="belongs to run 7"):
        newest_artifacts([foreign], LINUX, COMMIT)
    other_commit = {"id": 3, "name": "QGroundControl", "workflow_run": {**source, "head_sha": "x"}}
    with pytest.raises(PublishSetError, match="of commit x"):
        newest_artifacts([other_commit], LINUX, COMMIT)


X86_64 = "QGroundControl-x86_64.AppImage"
X86_64_DATA = b"Linux QGroundControl-x86_64"


def _embed_update_information(
    runs: Runs,
    artifact: str = "QGroundControl-x86_64",
    *,
    listed: bytes | None = None,
    uploaded: bytes | None = None,
    header: Mapping[str, str | None] | None = None,
) -> None:
    """Build *artifact* again as upstream does: with update information, so with a ``.zsync``.

    The marker lists the file *listed* and the artifact holds *uploaded*; both default to
    the update file of the package, with the fields of *header* changed.
    """
    file = f"{artifact}.AppImage"
    zsync = _zsync(file, f"Linux {artifact}".encode(), **(header or {}))
    marker = _marker(LINUX, file, companions={f"{file}.zsync": zsync if listed is None else listed})
    runs.replace(
        LINUX,
        artifact,
        {"publish": marker},
        companions={f"{file}.zsync": zsync if uploaded is None else uploaded},
    )


def test_an_appimage_s_update_file_is_verified_and_kept_for_the_github_release(
    runs: Runs, tmp_path: Path, gh_summary: Path
) -> None:
    # Upstream's AppImages embed `gh-releases-zsync|mavlink|qgroundcontrol|latest|<name>.zsync`:
    # an updater looks for that file on the repository's latest release.
    _embed_update_information(runs, "QGroundControl-x86_64")
    _embed_update_information(runs, "QGroundControl-aarch64")

    assert main(_fetch_command(tmp_path, RUNS)) == 0

    loaded = load_set(tmp_path / "set", ref=REF, commit=COMMIT)
    assert [package.name for package in loaded.packages] == PUBLISHED
    update_files = [f"{name}.zsync" for name in PUBLISHED if name.endswith(".AppImage")]
    assert [c.name for package in loaded.packages for c in package.companions] == update_files
    assert (tmp_path / f"set/{X86_64}.zsync").read_bytes() == _zsync(X86_64, X86_64_DATA)
    # A release gets each update file after its package and checksum. S3 and the daily
    # release, where no update information points, get the packages and checksums alone.
    assert [name for _, name in loaded.release_files()][:4] == [
        PUBLISHED[0],
        f"{PUBLISHED[0]}.sha256",
        f"{PUBLISHED[0]}.zsync",
        PUBLISHED[1],
    ]
    assert len(loaded.release_files()) == 14
    assert not any(name.endswith(".zsync") for _, name in loaded.files())
    # Attested by the build like the package itself.
    package = next(args for args in runs.verified if args[2].endswith(f"/{X86_64}"))
    update_file = next(args for args in runs.verified if args[2].endswith(f"/{X86_64}.zsync"))
    assert update_file[3:] == package[3:]
    assert len(runs.verified) == 2 * len(PUBLISHED) + 2
    assert (
        f"| For the GitHub Release only | `{update_files[0]}`, `{update_files[1]}` |"
    ) in gh_summary.read_text(encoding="utf-8")


def test_an_update_file_that_its_marker_lists_must_be_in_the_artifact(
    runs: Runs, tmp_path: Path
) -> None:
    marker = _marker(LINUX, X86_64, companions={f"{X86_64}.zsync": _zsync(X86_64, X86_64_DATA)})
    runs.replace(LINUX, "QGroundControl-x86_64", {"publish": marker})

    with pytest.raises(
        PublishSetError,
        match=rf"the publish marker of {X86_64} lists {X86_64}\.zsync, which the artifact lacks",
    ):
        _fetch(tmp_path)
    assert not (tmp_path / "set/publish-set.json").exists()


def test_an_update_file_needs_the_build_s_attestation(runs: Runs, tmp_path: Path) -> None:
    _embed_update_information(runs)
    runs.unattest(LINUX, "QGroundControl-x86_64", f"{X86_64}.zsync")

    with pytest.raises(
        PublishSetError, match=rf": {X86_64}\.zsync has no build provenance attestation from"
    ):
        _fetch(tmp_path)


def test_an_update_file_must_be_the_one_its_build_recorded(runs: Runs, tmp_path: Path) -> None:
    # Replaced in the artifact by an update file of the same package, with other block sums.
    _embed_update_information(runs, uploaded=_zsync(X86_64, X86_64_DATA, Blocksize="2048"))

    with pytest.raises(
        PublishSetError,
        match=rf"{X86_64}\.zsync is not the file its package's build metadata lists",
    ):
        _fetch(tmp_path)


@pytest.mark.parametrize(
    ("header", "problem"),
    [
        (
            {"Filename": "QGroundControl-aarch64.AppImage"},
            "has `Filename: QGroundControl-aarch64.AppImage` in its header, where the "
            f"package's is {X86_64}",
        ),
        ({"Length": "26"}, "has `Length: 26` in its header, where the package's is 27"),
        ({"SHA-1": "0" * 40}, f"has `SHA-1: {'0' * 40}` in its header, where the package's is"),
        ({"SHA-1": None}, "has `SHA-1: (none)` in its header"),
        # What an updater would download instead of the release's own file.
        (
            {"URL": f"https://example.com/{X86_64}"},
            f"has `URL: https://example.com/{X86_64}` in its header, where the package's is",
        ),
        ({"URL": f"../v5.1.4/{X86_64}"}, f"has `URL: ../v5.1.4/{X86_64}` in its header"),
        (
            {"Z-URL": f"https://example.com/{X86_64}.gz"},
            f"has `Z-URL: https://example.com/{X86_64}.gz` in its header, where the "
            "package's is (none)",
        ),
        ({"zsync": None}, "has no zsync header"),
    ],
)
def test_an_update_file_must_describe_the_verified_package(
    runs: Runs, tmp_path: Path, header: dict[str, str | None], problem: str
) -> None:
    # Attested and recorded by the build, and still not this AppImage's update file.
    _embed_update_information(runs, header=header)

    with pytest.raises(PublishSetError) as excinfo:
        _fetch(tmp_path)

    assert f"artifact QGroundControl-x86_64: {X86_64}.zsync {problem}" in str(excinfo.value)
    assert not (tmp_path / "set/publish-set.json").exists()


def test_an_update_file_offers_one_download_and_has_a_header(tmp_path: Path) -> None:
    package = tmp_path / X86_64
    package.write_bytes(X86_64_DATA)
    zsync = tmp_path / f"{X86_64}.zsync"

    zsync.write_bytes(_zsync(X86_64, X86_64_DATA))
    assert zsync_problem(zsync, package) is None

    # A second download, also written as zsync accepts it: without a blank after the colon.
    for second in (b"URL: https://example.com/x", b"URL:https://example.com/x"):
        twice = _zsync(X86_64, X86_64_DATA).replace(b"SHA-1", second + b"\nSHA-1")
        zsync.write_bytes(twice)
        assert zsync_problem(zsync, package) == (
            f"has `URL: {X86_64}, https://example.com/x` in its header, where the package's "
            f"is {X86_64}"
        )

    for content in (b"", b"zsync: 0.6.2\nFilename: x", b"\xff\xfe\n\n", b"\n\n" + twice):
        zsync.write_bytes(content)
        assert zsync_problem(zsync, package) == "has no zsync header"


@pytest.mark.parametrize(
    ("run", "artifact", "file", "companions", "problem"),
    [
        # Only an AppImage has an update file, and only under its own name.
        (
            MACOS,
            "QGroundControl",
            "QGroundControl.dmg",
            [{"name": "QGroundControl.dmg.zsync", "sha256": "0" * 64}],
            "lists the companion file 'QGroundControl.dmg.zsync', which is not a file",
        ),
        (
            LINUX,
            "QGroundControl-x86_64",
            X86_64,
            [{"name": "QGroundControl-aarch64.AppImage.zsync", "sha256": "0" * 64}],
            "lists the companion file 'QGroundControl-aarch64.AppImage.zsync', which is not",
        ),
        (
            LINUX,
            "QGroundControl-x86_64",
            X86_64,
            [{"name": f"../{X86_64}.zsync", "sha256": "0" * 64}],
            "which is not a file that is published beside that package",
        ),
        (
            LINUX,
            "QGroundControl-x86_64",
            X86_64,
            [{"name": "published.json", "sha256": "0" * 64}],
            "lists the companion file 'published.json', which is not",
        ),
        (LINUX, "QGroundControl-x86_64", X86_64, [f"{X86_64}.zsync"], "gives no SHA-256 of"),
        (
            LINUX,
            "QGroundControl-x86_64",
            X86_64,
            [{"name": f"{X86_64}.zsync", "sha256": "0" * 64}] * 2,
            "or is listed twice",
        ),
        (
            LINUX,
            "QGroundControl-x86_64",
            X86_64,
            {"name": f"{X86_64}.zsync"},
            "does not list its companion files",
        ),
    ],
)
def test_a_marker_lists_no_other_file_than_its_appimage_s_update_file(
    runs: Runs,
    tmp_path: Path,
    run: SourceRun,
    artifact: str,
    file: str,
    companions: object,
    problem: str,
) -> None:
    runs.replace(run, artifact, {"publish": {**_marker(run, file), "companions": companions}})

    with pytest.raises(PublishSetError, match=f"the publish marker of {file} .*{problem}"):
        _fetch(tmp_path)
    assert not list((tmp_path / "set").glob("*.zsync"))


def test_an_update_file_changed_after_verification_is_not_loaded(
    runs: Runs, tmp_path: Path
) -> None:
    _embed_update_information(runs)
    _fetch(tmp_path)
    directory = tmp_path / "set"
    manifest = json.loads((directory / "publish-set.json").read_text())

    (directory / f"{X86_64}.zsync").write_bytes(b"tampered")
    with pytest.raises(PublishSetError, match=rf"{X86_64}\.zsync is not the file that was"):
        load_set(directory, ref=REF, commit=COMMIT)

    # A manifest that names another file as a companion: only an update file is one.
    package = next(entry for entry in manifest["packages"] if entry["name"] == X86_64)
    package["companions"][0]["name"] = "QGroundControl.dmg"
    (directory / "publish-set.json").write_text(json.dumps(manifest))
    with pytest.raises(PublishSetError, match=r"'QGroundControl\.dmg' is not a file that is"):
        load_set(directory, ref=REF, commit=COMMIT)


def test_a_set_names_the_runs_that_built_it(runs: Runs, tmp_path: Path) -> None:
    _fetch(tmp_path)
    directory = tmp_path / "set"
    manifest = json.loads((directory / "publish-set.json").read_text())

    def load_with(**changes: Any) -> None:
        (directory / "publish-set.json").write_text(json.dumps({**manifest, **changes}))
        load_set(directory, ref=REF, commit=COMMIT)

    with pytest.raises(PublishSetError, match=r"QGroundControl\.apk is built by none of the"):
        load_with(runs=manifest["runs"][:3])
    cancelled = [*manifest["runs"][:3], {**manifest["runs"][3], "conclusion": "cancelled"}]
    with pytest.raises(PublishSetError, match="not built by runs that refs/heads/master is"):
        load_with(runs=cancelled)
    with pytest.raises(PublishSetError, match="cannot read"):
        load_with(runs=None)
    with pytest.raises(PublishSetError, match="unknown manifest version 1"):
        load_with(schema_version=1)


def test_unmarked_metadata_is_not_checked_against_the_run() -> None:
    # A package that is not published may come from any event.
    metadata = {"schema_version": 1, "event": "pull_request", "commit": "other"}
    assert marked_package(metadata, repo=REPO, ref=REF, commit=COMMIT, run=LINUX) is None


def test_publishing_jobs_load_the_set_they_were_told_to_publish(runs: Runs, tmp_path: Path) -> None:
    _fetch(tmp_path)
    directory = tmp_path / "set"

    loaded = load_set(directory, ref=REF, commit=COMMIT)

    assert [package.name for package in loaded.packages] == PUBLISHED
    assert (loaded.repository, loaded.ref_name, loaded.is_tag) == (REPO, "master", False)
    # Each package is followed by its checksum, as the build jobs uploaded them.
    assert [name for _, name in loaded.files()][:2] == [PUBLISHED[0], f"{PUBLISHED[0]}.sha256"]
    assert all(path.is_file() for path, _ in loaded.files())

    with pytest.raises(PublishSetError, match=r"not refs/heads/Stable_V5\.1"):
        load_set(directory, ref="refs/heads/Stable_V5.1", commit=COMMIT)
    with pytest.raises(PublishSetError, match="not refs/heads/master at 0000"):
        load_set(directory, ref=REF, commit="0" * 40)


def test_a_set_changed_after_verification_is_not_loaded(runs: Runs, tmp_path: Path) -> None:
    _fetch(tmp_path)
    directory = tmp_path / "set"
    manifest = json.loads((directory / "publish-set.json").read_text())

    (directory / "QGroundControl.dmg").write_bytes(b"tampered")
    with pytest.raises(PublishSetError, match="SHA-256 checksum mismatch"):
        load_set(directory, ref=REF, commit=COMMIT)

    # A sidecar rewritten to match does not help: the manifest holds the verified digest.
    digest = hashlib.sha256(b"tampered").hexdigest()
    (directory / "QGroundControl.dmg.sha256").write_text(f"{digest}  QGroundControl.dmg\n")
    with pytest.raises(PublishSetError, match="not the file that was verified"):
        load_set(directory, ref=REF, commit=COMMIT)

    manifest["packages"][0]["name"] = "../QGroundControl.dmg"
    (directory / "publish-set.json").write_text(json.dumps(manifest))
    with pytest.raises(PublishSetError, match="is not a package name that the default branch"):
        load_set(directory, ref=REF, commit=COMMIT)

    (directory / "publish-set.json").write_text("[]")
    with pytest.raises(PublishSetError, match="cannot read"):
        load_set(directory, ref=REF, commit=COMMIT)
