#!/usr/bin/env python3
"""Fetch the packages a commit's platform runs marked for publishing, and verify them.

``fetch`` takes the workflow runs to publish from (``--runs``, the JSON ``publish_decide.py
decide`` outputs) and downloads their distributable artifacts as data: nothing from a run is
executed. A package counts when its build metadata (``<file>.build.json``) carries the publish
marker. Each one must agree with the run it was downloaded from (repository, ref, commit,
workflow, run), have a name this checkout publishes (``is_publishable_name``: the default
branch's list, whatever branch was built), match its ``.sha256`` sidecar, and have a build
provenance attestation issued to that workflow file for that ref and commit. The metadata needs
that attestation too: what it says of the package is then the build's word, not the word of
whoever uploaded the artifact.

A set is complete by the build's own account: each marker lists the packages its workflow marks
in the commit that was built, and the run must provide exactly those. An older Stable branch
lists fewer packages than the default branch builds, and its set is complete all the same.
Every workflow must contribute. The verified files and their manifest (``publish-set.json``)
go to ``--out``; the publishing jobs read them back with ``load_set``.

A re-run keeps a run's id and adds artifacts to it. Each run is therefore approved at one
attempt and with the conclusion it had: a package built by a later attempt fails the set, and a
run whose latest attempt is no longer the approved one when the set is complete publishes
nothing (output ``publish=false``), as do runs without any marker, whose workflows publish for
themselves. The metadata names the job that built the package. It must be a job of the approved
attempt that passed, or one of an earlier attempt that passed and that the approved attempt,
beyond doubt, did not run again. Only the newest artifact of a name is read, so when the
approved attempt's is missing the set fails: an earlier attempt's package never takes its place.

A release tag is published only from runs that passed. A branch is also published from a run
that failed (``PUBLISHABLE_CONCLUSIONS``), so that a job which builds no package does not hold
the packages back: the job that built each package still has to have passed, and the run still
has to provide every package its markers list. When a failed run does not, its build failed
and nothing is wrong with publishing: ``fetch`` says which package is missing and ends with
``publish=false``. The same from a run that passed fails the set, as does a run that passed and
lists no package artifact at all, because GitHub deletes artifacts when they expire.

A marker can also list companion files (``companion_names``: an AppImage's ``.zsync``), which
only the tag's GitHub Release gets. Each must be in the artifact, match the SHA-256 the marker
gives, have the build's attestation like the package, and describe the verified package.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import sys
import zipfile
from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING, Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.format import format_bytes
from common.gh_actions import (
    download_artifact_zip,
    env_default,
    gh,
    gh_api_json,
    gh_error,
    gh_notice,
    gh_warning,
    list_run_artifacts,
    list_run_jobs,
    resolve_ref_commit,
    write_github_output,
    write_step_summary,
)
from common.io import read_json, sha256_file, verify_sha256_sidecar, write_json
from common.markdown import code_span, md_table, summary_section
from qgc_tools.build_artifacts import (
    ARTIFACT_RETENTION_DAYS,
    BUILD_METADATA_SCHEMA_VERSION,
    BUILD_METADATA_SUFFIX,
    companion_names,
    is_distributable_artifact,
    is_publishable_name,
)

if TYPE_CHECKING:
    from collections.abc import Callable, Iterable, Mapping, Sequence

MANIFEST_NAME = "publish-set.json"
# Version 2 added the runs with their conclusions, and each package's companion files.
MANIFEST_SCHEMA_VERSION = 2
PROVENANCE_PREDICATE = "https://slsa.dev/provenance/v1"
# GitHub rejects larger release assets, and no package comes near it.
MAX_FILE_BYTES = 2 * 1024**3
MAX_METADATA_BYTES = 1024**2
# A .zsync holds a few bytes for every block of its package: 350 kB for a 200 MB AppImage.
MAX_COMPANION_BYTES = 64 * 1024**2
# Its header is a dozen short lines, which a blank line ends.
ZSYNC_HEADER_BYTES = 4096
# The conclusions of a workflow run from which a ref of each kind is published. A branch's
# commit is not held back by a job that builds no package, so its run may have failed: every
# package then still comes from a job that passed. A release needs every job to pass. A run
# with any other conclusion (cancelled, timed_out, skipped, action_required, ...) was stopped
# or never ran, which says nothing of its commit: the commit waits for a re-run.
PUBLISHABLE_CONCLUSIONS = {
    "branch": frozenset({"success", "failure"}),
    "tag": frozenset({"success"}),
}
_COMMIT = re.compile(r"[0-9a-f]{40}")
_SHA256 = re.compile(r"[0-9a-f]{64}")
_JOB_ID = re.compile(r"[1-9][0-9]*")
_REF = re.compile(r"refs/(?:heads|tags)/[A-Za-z0-9._-]+")


class PublishSetError(RuntimeError):
    """The packages cannot be published as they are."""


class RerunError(PublishSetError):
    """A run is no longer at the attempt that was approved, so its packages are not published."""


class NotBuiltError(PublishSetError):
    """A run lacks a package its workflow marks, or the job that built one did not pass.

    From a run that failed this is the build's failure and leaves nothing to publish.
    ``fetch_set`` raises it for such a run only: the same from a run that passed fails the set.
    """


@dataclass(frozen=True)
class SourceRun:
    """A platform workflow's run for the commit, at the attempt the decision approved."""

    workflow: str
    path: str
    run_id: int
    attempt: int
    # How that attempt concluded: one of the ref's PUBLISHABLE_CONCLUSIONS.
    conclusion: str = "success"

    @property
    def passed(self) -> bool:
        return self.conclusion == "success"

    def url(self, repo: str) -> str:
        """The page of the approved attempt, which a later re-run does not change."""
        return f"https://github.com/{repo}/actions/runs/{self.run_id}/attempts/{self.attempt}"


@dataclass(frozen=True)
class Marker:
    """What a package's build metadata says of publishing it."""

    name: str
    sha256: str
    # Every package name that its workflow marks in the commit that was built.
    inventory: frozenset[str]
    # The attempt of the run that built the package, and the id of the job that did.
    attempt: int
    job_id: int
    # The companion files it lists, each with its SHA-256.
    companions: tuple[tuple[str, str], ...] = ()


@dataclass(frozen=True)
class Companion:
    """A verified file that is published beside its package on a GitHub Release only."""

    name: str
    sha256: str
    size: int


@dataclass(frozen=True)
class Package:
    name: str
    sha256: str
    size: int
    workflow: str
    run_id: int
    companions: tuple[Companion, ...] = ()


@dataclass(frozen=True)
class PublishSet:
    """The verified packages of one commit of one ref, and the runs that built them."""

    directory: Path
    repository: str
    ref: str
    commit: str
    packages: tuple[Package, ...]
    runs: tuple[SourceRun, ...] = ()

    @property
    def ref_name(self) -> str:
        return self.ref.split("/", 2)[2]

    @property
    def is_tag(self) -> bool:
        return self.ref.startswith("refs/tags/")

    def files(self) -> list[tuple[Path, str]]:
        """Every file to publish with its public name: each package, then its checksum."""
        return [
            (self.directory / name, name)
            for package in self.packages
            for name in (package.name, f"{package.name}.sha256")
        ]

    def release_files(self) -> list[tuple[Path, str]]:
        """Every file of a GitHub Release: ``files()``, and each package's companions after it."""
        return [
            (self.directory / name, name)
            for package in self.packages
            for name in (
                package.name,
                f"{package.name}.sha256",
                *(companion.name for companion in package.companions),
            )
        ]


def publishable_conclusions(ref: str) -> frozenset[str]:
    """Return the run conclusions from which the full ref *ref* is published."""
    return PUBLISHABLE_CONCLUSIONS["tag" if ref.startswith("refs/tags/") else "branch"]


def tag_problem(repo: str, tag: str, commit: str) -> str | None:
    """Return why the packages of *commit* are not release *tag*'s any more, or None.

    A publishing job waits its turn, and a tag can be moved or deleted meanwhile. Each job asks
    this right before it writes; an annotated tag is followed to its commit.
    """
    found = resolve_ref_commit(repo, f"tags/{tag}")
    if found is None:
        return f"the tag {tag} no longer exists"
    if found != commit:
        return (
            f"the tag {tag} now names {found}, not {commit}, which these packages were built from"
        )
    return None


def _run_entry(run: SourceRun) -> dict[str, Any]:
    return {
        "workflow": run.workflow,
        "path": run.path,
        "id": run.run_id,
        "attempt": run.attempt,
        "conclusion": run.conclusion,
    }


def _source_run(entry: Mapping[str, Any]) -> SourceRun:
    return SourceRun(
        str(entry["workflow"]),
        str(entry["path"]),
        int(entry["id"]),
        int(entry["attempt"]),
        str(entry["conclusion"]),
    )


def parse_runs(text: str) -> list[SourceRun]:
    """Parse the decision's ``runs`` output.

    It is a JSON list of ``{workflow, path, id, attempt, conclusion}``.
    """
    try:
        runs = [_source_run(run) for run in json.loads(text)]
    except (KeyError, TypeError, ValueError) as exc:
        raise PublishSetError(f"cannot read the workflow runs to publish from: {exc}") from exc
    if not runs:
        raise PublishSetError("no workflow runs to publish from")
    return runs


def require_approved_attempts(repo: str, runs: Iterable[SourceRun]) -> None:
    """Raise RerunError unless every run's latest attempt is the approved one, as it concluded."""
    for run in runs:
        data = gh_api_json(f"repos/{repo}/actions/runs/{run.run_id}")
        completed = data.get("status") == "completed"
        state = data.get("conclusion") if completed else data.get("status")
        if (data.get("run_attempt"), state) != (run.attempt, run.conclusion):
            raise RerunError(
                f"{run.workflow} run {run.run_id} was approved at attempt {run.attempt} "
                f"({run.conclusion}), but its latest attempt is {data.get('run_attempt')} "
                f"({state}). Nothing is published now: when a re-run completes, its completion "
                "starts a publish of its own."
            )


def newest_artifacts(
    artifacts: Iterable[Mapping[str, Any]], run: SourceRun, commit: str
) -> list[Mapping[str, Any]]:
    """Return the run's distributable artifacts, the newest of each name.

    A re-run adds an artifact of the same name beside the earlier attempt's. The older ones are
    never read: ``require_approved_job`` holds the newest to the approved attempt. GitHub drops
    an artifact from the listing when it expires, and one that is still listed as expired is
    as gone: ``fetch_set`` tells a run without any from a run that never built one.
    """
    newest: dict[str, Mapping[str, Any]] = {}
    for artifact in artifacts:
        name = str(artifact.get("name") or "")
        if not is_distributable_artifact(name) or artifact.get("expired"):
            continue
        source = artifact.get("workflow_run") or {}
        if source.get("id") != run.run_id or source.get("head_sha") != commit:
            raise PublishSetError(
                f"artifact {name} of {run.workflow} run {run.run_id} belongs to run "
                f"{source.get('id')} of commit {source.get('head_sha')}"
            )
        current = newest.get(name)
        if current is None or _age(artifact) > _age(current):
            newest[name] = artifact
    return [newest[name] for name in sorted(newest)]


def _age(artifact: Mapping[str, Any]) -> tuple[str, int]:
    return str(artifact.get("created_at") or ""), int(artifact.get("id") or 0)


def read_metadata(archive: zipfile.ZipFile) -> tuple[str, dict[str, Any]] | None:
    """Return the name and content of the build metadata at an archive's root, or None."""
    names = [
        name
        for name in archive.namelist()
        if name.endswith(BUILD_METADATA_SUFFIX) and "/" not in name
    ]
    if not names:
        return None
    if len(names) > 1:
        raise PublishSetError(f"more than one build metadata file: {', '.join(sorted(names))}")
    if archive.getinfo(names[0]).file_size > MAX_METADATA_BYTES:
        raise PublishSetError(f"{names[0]} is too large to be build metadata")
    try:
        metadata = json.loads(archive.read(names[0]))
    except ValueError as exc:
        raise PublishSetError(f"{names[0]} is not valid JSON: {exc}") from exc
    if not isinstance(metadata, dict):
        raise PublishSetError(f"{names[0]} is not a JSON object")
    return names[0], metadata


def marked_package(
    metadata: Mapping[str, Any], *, repo: str, ref: str, commit: str, run: SourceRun
) -> Marker | None:
    """Return the publish marker of a package's build metadata, or None without one.

    The marker comes from the artifact, so everything in it that the run already tells us must
    agree with the run, and the name it asks for must be one this checkout publishes. The
    attempt that built the package cannot be later than the approved one;
    ``require_approved_job`` holds its job to that attempt.
    """
    marker = metadata.get("publish")
    if marker is None:
        return None
    expected = {
        "schema_version": BUILD_METADATA_SCHEMA_VERSION,
        "repository": repo,
        "ref": ref,
        "commit": commit,
        "event": "push",
        "workflow": run.workflow,
        "run_id": str(run.run_id),
    }
    for key, value in expected.items():
        if metadata.get(key) != value:
            raise PublishSetError(
                f"build metadata {key} is {metadata.get(key)!r}, but the run's is {value!r}"
            )
    built = str(metadata.get("run_attempt"))
    if not built.isdigit() or int(built) > run.attempt:
        raise PublishSetError(
            f"build metadata run_attempt is {metadata.get('run_attempt')!r}, but attempt "
            f"{run.attempt} of the run was approved"
        )
    marker = marker if isinstance(marker, dict) else {}
    name = marker.get("name")
    artifact = metadata.get("artifact")
    artifact = artifact if isinstance(artifact, dict) else {}
    if not isinstance(name, str) or not is_publishable_name(name):
        raise PublishSetError(
            f"publish name {name!r} is not a package name that the default branch publishes"
        )
    if artifact.get("name") != name:
        raise PublishSetError(
            f"publish name {name!r} is not the package file {artifact.get('name')!r}"
        )
    digest = artifact.get("sha256")
    if not isinstance(digest, str) or not _SHA256.fullmatch(digest):
        raise PublishSetError(f"build metadata of {name} has no SHA-256")
    if marker.get("workflow") != run.workflow:
        raise PublishSetError(
            f"the publish marker of {name} names the workflow {marker.get('workflow')!r}, "
            f"not {run.workflow!r}"
        )
    packages = marker.get("packages")
    if (
        not isinstance(packages, list)
        or not packages
        or not all(isinstance(package, str) for package in packages)
        or len(set(packages)) != len(packages)
    ):
        raise PublishSetError(
            f"the publish marker of {name} does not list the packages its workflow marks"
        )
    job_id = metadata.get("job_id")
    if not isinstance(job_id, str) or not _JOB_ID.fullmatch(job_id):
        raise PublishSetError(
            f"{name} is marked for publishing without the id of the job that built it (build "
            f"metadata job_id is {job_id!r}), so it cannot be bound to a job that passed. Its "
            "build warned that attest-and-upload got no usable job-id."
        )
    return Marker(
        name, digest, frozenset(packages), int(built), int(job_id), _marked_companions(marker, name)
    )


def _marked_companions(marker: Mapping[str, Any], name: str) -> tuple[tuple[str, str], ...]:
    """Return the companion files that the publish marker of package *name* lists.

    Each comes with the SHA-256 the build recorded. Only a name that the package may have a
    companion under is accepted: a companion is no way to publish an arbitrary file.
    """
    listed = marker.get("companions", [])
    if not isinstance(listed, list):
        raise PublishSetError(f"the publish marker of {name} does not list its companion files")
    found: dict[str, str] = {}
    for entry in listed:
        companion, digest = (
            (entry.get("name"), entry.get("sha256")) if isinstance(entry, dict) else (entry, None)
        )
        if companion not in companion_names(name) or companion in found:
            raise PublishSetError(
                f"the publish marker of {name} lists the companion file {companion!r}, which is "
                "not a file that is published beside that package, or is listed twice"
            )
        if not isinstance(digest, str) or not _SHA256.fullmatch(digest):
            raise PublishSetError(f"the publish marker of {name} gives no SHA-256 of {companion}")
        found[companion] = digest
    return tuple(found.items())


def require_approved_job(
    run: SourceRun,
    marked: Marker,
    jobs_of: Callable[[int], Sequence[Mapping[str, Any]]],
    artifact: str,
) -> None:
    """Raise unless the job that built the package passed and the approved attempt stands by it.

    NotBuiltError when the approved attempt has no such package: the job did not pass, or
    that attempt ran it again and the package it built then is missing.

    *jobs_of* lists the run's jobs of an attempt; a package names a job of the attempt that
    built it. An earlier attempt's job counts only if the approved attempt did not run it
    again. GitHub then lists a copy of it under a new id, with the name and the start and
    completion times of the original, and links the two in no other way. Those three must
    therefore single the job out: no other job of its attempt may have them, and exactly one
    passed job of the approved attempt must. Any other package of an earlier attempt was built
    again since, so the newer artifact of its name is missing, or cannot be told from one that
    was.
    """
    job = next((job for job in jobs_of(marked.attempt) if job.get("id") == marked.job_id), None)
    if job is None:
        raise PublishSetError(
            f"job {marked.job_id}, which built {marked.name}, is not a job of attempt "
            f"{marked.attempt} of run {run.run_id}"
        )
    built_by = f"the job {job.get('name')!r} ({marked.job_id}) of attempt {marked.attempt}"
    conclusion = job.get("conclusion")
    if conclusion != "success":
        problem = "failed" if conclusion == "failure" else f"did not pass ({conclusion})"
    elif marked.attempt == run.attempt:
        return
    else:
        originals = _alike(job, jobs_of(marked.attempt))
        copies = [
            other
            for other in _alike(job, jobs_of(run.attempt))
            if other.get("conclusion") == "success"
        ]
        if len(originals) == len(copies) == 1:
            return
        if copies:
            raise PublishSetError(
                f"{built_by} of run {run.run_id}, which built {marked.name}, cannot be "
                f"identified unambiguously in attempt {run.attempt}. Jobs with its name and its "
                f"start and completion times: {len(originals)} in attempt {marked.attempt}, "
                f"{len(copies)} passed in attempt {run.attempt}; exactly one each would show "
                f"that attempt {run.attempt} did not run it again. Re-run the job to build the "
                "package again"
            )
        problem = f"attempt {run.attempt} ran again"
    reason = f"{marked.name} was built by {built_by}, which {problem}"
    if marked.attempt != run.attempt:
        reason += (
            f". The artifact {artifact} of attempt {run.attempt}, which was approved, is "
            "missing: re-run the job to build the package again"
        )
    raise NotBuiltError(reason)


def _alike(job: Mapping[str, Any], jobs: Iterable[Mapping[str, Any]]) -> list[Mapping[str, Any]]:
    """Return the *jobs* that have the name and the start and completion times of *job*."""

    def identity(item: Mapping[str, Any]) -> tuple[Any, Any, Any]:
        return item.get("name"), item.get("started_at"), item.get("completed_at")

    wanted = identity(job)
    # A job that lacks one of the three is like none.
    return [other for other in jobs if identity(other) == wanted] if all(wanted) else []


def _extract(
    archive: zipfile.ZipFile, member: str, destination: Path, *, limit: int = MAX_FILE_BYTES
) -> None:
    try:
        info = archive.getinfo(member)
    except KeyError:
        raise PublishSetError(f"the artifact has no file {member}") from None
    if info.file_size > limit:
        raise PublishSetError(f"{member} is larger than {format_bytes(limit)}")
    with archive.open(info) as source, destination.open("wb") as target:
        shutil.copyfileobj(source, target, 1024 * 1024)


def verify_attestation(
    path: Path, *, repo: str, ref: str, commit: str, run: SourceRun, name: str = ""
) -> None:
    """Require a build provenance attestation of *path* from the run's workflow, ref and commit.

    The source and signer digests agree because the platform workflows attest in their own jobs,
    not in a reusable workflow. Self-hosted runners are allowed: upstream builds on RunsOn.
    A failure names the file as *name* (default: its file name).
    """
    result = gh(
        "attestation",
        "verify",
        str(path),
        "--repo",
        repo,
        "--signer-workflow",
        f"{repo}/{run.path}",
        "--source-ref",
        ref,
        "--source-digest",
        commit,
        "--signer-digest",
        commit,
        "--predicate-type",
        PROVENANCE_PREDICATE,
        check=False,
        timeout=300,
    )
    if result.returncode != 0:
        detail = (result.stderr or result.stdout).strip().splitlines()
        raise PublishSetError(
            f"{name or path.name} has no build provenance attestation from {run.path} for "
            f"{ref} at {commit}: {detail[-1] if detail else 'gh attestation verify failed'}"
        )


def zsync_problem(zsync: Path, package: Path) -> str | None:
    """Return why *zsync* is not the update file of *package*, or None when it is.

    An updater downloads what the header's ``URL`` names, relative to the ``.zsync`` itself,
    and keeps the result when it has the header's ``Length`` and ``SHA-1``. All of them must
    therefore describe the verified package, and the header must offer no other download.
    A line is read as zsync reads it: the key ends at the first colon, and the value starts
    after the blanks that follow it.
    """
    with zsync.open("rb") as stream:
        header, ended, _ = stream.read(ZSYNC_HEADER_BYTES).partition(b"\n\n")
    fields: dict[str, list[str]] = {}
    try:
        for line in header.decode("ascii").split("\n") if ended else []:
            key, _, value = line.partition(":")
            fields.setdefault(key, []).append(value.lstrip(" \t"))
    except UnicodeDecodeError:
        fields.clear()
    if "zsync" not in fields:
        return "has no zsync header"
    with package.open("rb") as stream:
        sha1 = hashlib.file_digest(stream, lambda: hashlib.sha1(usedforsecurity=False))
    expected = {
        "Filename": package.name,
        "Length": str(package.stat().st_size),
        "SHA-1": sha1.hexdigest(),
        "URL": package.name,
        "Z-URL": None,
    }
    for key, value in expected.items():
        found = fields.get(key, [])
        if found != ([] if value is None else [value]):
            return (
                f"has `{key}: {', '.join(found) or '(none)'}` in its header, where the "
                f"package's is {value or '(none)'}"
            )
    return None


def verify_companion(
    path: Path, digest: str, package: Path, *, repo: str, ref: str, commit: str, run: SourceRun
) -> Companion:
    """Return the companion file *path* of *package*, once it is the build's and describes it."""
    if sha256_file(path) != digest:
        raise PublishSetError(f"{path.name} is not the file its package's build metadata lists")
    verify_attestation(path, repo=repo, ref=ref, commit=commit, run=run)
    if problem := zsync_problem(path, package):
        raise PublishSetError(f"{path.name} {problem}")
    return Companion(path.name, digest, path.stat().st_size)


def fetch_run(
    repo: str,
    ref: str,
    commit: str,
    run: SourceRun,
    artifacts: Iterable[Mapping[str, Any]],
    out: Path,
    work: Path,
) -> list[Package]:
    """Download *run*'s *artifacts* and return its verified packages, extracted into *out*.

    They are every package its workflow marks, as their markers list them, or the run fails:
    with NotBuiltError when the approved attempt did not build one. A marker counts once the
    build's attestation covers the metadata that holds it, and the package once the job that
    built it is one the approved attempt stands by.
    """
    packages = []
    inventories: dict[str, frozenset[str]] = {}
    listed: dict[int, list[dict[str, Any]]] = {}

    def jobs_of(attempt: int) -> list[dict[str, Any]]:
        if attempt not in listed:
            listed[attempt] = list_run_jobs(repo, run.run_id, attempt=attempt)
        return listed[attempt]

    for artifact in artifacts:
        label = f"{run.workflow} run {run.run_id}, artifact {artifact['name']}"
        archive_path = work / f"{int(artifact['id'])}.zip"
        record = archive_path.with_suffix(BUILD_METADATA_SUFFIX)
        try:
            download_artifact_zip(repo, artifact["id"], archive_path)
            with zipfile.ZipFile(archive_path) as archive:
                member, metadata = read_metadata(archive) or ("", {})
                marked = marked_package(metadata, repo=repo, ref=ref, commit=commit, run=run)
                if marked is None:
                    print(f"{label}: not marked for publishing")
                    continue
                _extract(archive, member, record)
                verify_attestation(
                    record,
                    repo=repo,
                    ref=ref,
                    commit=commit,
                    run=run,
                    name=f"the build metadata {member}",
                )
                require_approved_job(run, marked, jobs_of, str(artifact["name"]))
                name, digest = marked.name, marked.sha256
                package = out / name
                if package.exists():
                    raise PublishSetError(f"two packages ask for the name {name}")
                _extract(archive, name, package)
                _extract(archive, f"{name}.sha256", package.with_name(f"{name}.sha256"))
                for companion, _ in marked.companions:
                    if companion not in archive.namelist():
                        raise PublishSetError(
                            f"the publish marker of {name} lists {companion}, which the "
                            "artifact lacks"
                        )
                    _extract(archive, companion, out / companion, limit=MAX_COMPANION_BYTES)
            if verify_sha256_sidecar(package) != digest:
                raise PublishSetError(f"{name} is not the file its build metadata describes")
            verify_attestation(package, repo=repo, ref=ref, commit=commit, run=run)
            companions = tuple(
                verify_companion(
                    out / companion, expected, package, repo=repo, ref=ref, commit=commit, run=run
                )
                for companion, expected in marked.companions
            )
        except NotBuiltError as exc:
            raise NotBuiltError(f"{label}: {exc}") from exc
        except (RuntimeError, OSError, ValueError, zipfile.BadZipFile) as exc:
            raise PublishSetError(f"{label}: {exc}") from exc
        finally:
            archive_path.unlink(missing_ok=True)
        with_companions = "".join(f", with {companion.name}" for companion in companions)
        print(f"{label}: verified {name} ({digest}){with_companions}")
        packages.append(
            Package(name, digest, package.stat().st_size, run.workflow, run.run_id, companions)
        )
        inventories[name] = marked.inventory
    require_inventory(run, inventories)
    return packages


def require_inventory(run: SourceRun, inventories: Mapping[str, frozenset[str]]) -> None:
    """Raise unless *run* provided exactly the packages its workflow marks, by its own account.

    NotBuiltError when it lacks one. *inventories* is, by package, the list its marker gives
    of the packages the workflow marks in the commit that was built. That list, not the default
    branch's, says what is complete: a Stable branch cut before a package was added releases
    without it.
    """
    source = f"{run.workflow} run {run.run_id}"
    listed: dict[frozenset[str], str] = {}
    for name in sorted(inventories):
        listed.setdefault(inventories[name], name)
    if len(listed) > 1:
        (first, one), (second, other) = list(listed.items())[:2]
        raise PublishSetError(
            f"the packages of {source} disagree on what the workflow publishes: {one} lists "
            f"{', '.join(sorted(first))}, and {other} lists {', '.join(sorted(second))}"
        )
    for inventory in listed:
        if missing := sorted(inventory - inventories.keys()):
            raise NotBuiltError(
                f"{source} lacks {', '.join(missing)}, which its workflow marks: "
                "a commit is published with every package or not at all"
            )
        if unlisted := sorted(inventories.keys() - inventory):
            raise PublishSetError(
                f"{', '.join(unlisted)} of {source} is marked for publishing, "
                "but is not a package its workflow lists"
            )


def fetch_set(
    repo: str, ref: str, commit: str, runs: Sequence[SourceRun], out: Path
) -> PublishSet | None:
    """Fetch and verify every run's packages into *out*; None when no run marked any.

    Once one does, every run must provide the packages its workflow marks: a partial set is
    never published. A run that passed and lacks one fails the set, and so does one that passed
    and lists no package artifact, which GitHub deleted since. A run that failed and lacks one
    raises NotBuiltError: its build failed, which leaves nothing to publish. Raises RerunError
    when a run was re-run after it was approved.
    """
    allowed = publishable_conclusions(ref)
    for run in runs:
        if run.conclusion not in allowed:
            raise PublishSetError(
                f"{run.workflow} run {run.run_id} concluded `{run.conclusion}`: {ref} is not "
                "published from such a run"
            )
    require_approved_attempts(repo, runs)
    artifacts = {
        run.run_id: newest_artifacts(list_run_artifacts(repo, run.run_id), run, commit)
        for run in runs
    }
    if gone := [run for run in runs if run.passed and not artifacts[run.run_id]]:
        one = len(gone) == 1
        advice = (
            f"Re-run all jobs of {'this run' if one else 'each of these runs'} to build the "
            "packages again: the completion of the last starts a publish of its own."
        )
        if ref.startswith("refs/tags/"):
            tag = ref.removeprefix("refs/tags/")
            advice = (
                "Do not re-run the jobs of a release tag's runs to build them again: the runs "
                "would repeat their uploads to the stores and their builder images. What was "
                f"published of {tag} is on the download server under builds/{tag}/: "
                '"Going back to an earlier release" and "Release files" in '
                "docs/en/qgc-dev-guide/release_branching_process.md say how to publish it "
                "from there by hand."
            )
        raise PublishSetError(
            f"{_names(gone)} passed, but {'lists' if one else 'list'} no package artifact. "
            f"GitHub deletes a build's artifacts {ARTIFACT_RETENTION_DAYS} days after their "
            f"upload, and nothing is published without them. {advice}"
        )
    if out.exists():
        shutil.rmtree(out)
    work = out.with_name(f"{out.name}.download")
    work.mkdir(parents=True, exist_ok=True)
    out.mkdir(parents=True)
    packages: list[Package] = []
    unmarked = []
    try:
        # The runs that failed come first: one that lacks a package ends the fetch before the
        # packages of the others are downloaded.
        for run in sorted(runs, key=lambda run: run.passed):
            try:
                found = fetch_run(repo, ref, commit, run, artifacts[run.run_id], out, work)
            except NotBuiltError as exc:
                raise _unbuilt(str(exc), [run]) from exc
            packages += found
            if not found:
                unmarked.append(run)
            if packages and unmarked:
                break
    finally:
        shutil.rmtree(work, ignore_errors=True)
    if not packages:
        if any(artifacts.values()):
            return None
        raise _unbuilt("none of the commit's runs built a package", runs)
    if unmarked:
        raise _unbuilt(
            f"no package of {_names(unmarked)} is marked for publishing, but other workflows "
            "of the commit marked theirs",
            unmarked,
        )
    # A re-run that started meanwhile may have added the artifacts that were just read.
    require_approved_attempts(repo, runs)
    packages.sort(key=lambda package: package.name)
    write_json(
        out / MANIFEST_NAME,
        {
            "schema_version": MANIFEST_SCHEMA_VERSION,
            "repository": repo,
            "ref": ref,
            "commit": commit,
            "runs": [_run_entry(run) for run in runs],
            "packages": [
                {
                    "name": package.name,
                    "sha256": package.sha256,
                    "size": package.size,
                    "workflow": package.workflow,
                    "run_id": package.run_id,
                    "companions": [
                        {"name": companion.name, "sha256": companion.sha256, "size": companion.size}
                        for companion in package.companions
                    ],
                }
                for package in packages
            ],
        },
    )
    return PublishSet(out, repo, ref, commit, tuple(packages), tuple(runs))


def _names(runs: Iterable[SourceRun]) -> str:
    return ", ".join(f"{run.workflow} run {run.run_id}" for run in runs)


def _unbuilt(reason: str, runs: Sequence[SourceRun]) -> PublishSetError:
    """Return the error for packages that *runs* lack: the build's own when none of them passed."""
    if any(run.passed for run in runs):
        return PublishSetError(reason)
    concluded = ", ".join(
        f"{run.workflow} run {run.run_id} concluded `{run.conclusion}`" for run in runs
    )
    return NotBuiltError(f"{reason}. {concluded}.")


def load_set(directory: Path, *, ref: str, commit: str) -> PublishSet:
    """Read back the set ``fetch`` wrote to *directory*, which must be *commit* of *ref*.

    Names are checked again and every file is rehashed, so a publishing job uses nothing but the
    files that were verified.
    """
    try:
        data = read_json(directory / MANIFEST_NAME)
        if data["schema_version"] != MANIFEST_SCHEMA_VERSION:
            raise PublishSetError(f"unknown manifest version {data['schema_version']!r}")
        packages = tuple(
            Package(
                str(entry["name"]),
                str(entry["sha256"]),
                int(entry["size"]),
                str(entry["workflow"]),
                int(entry["run_id"]),
                tuple(
                    Companion(str(item["name"]), str(item["sha256"]), int(item["size"]))
                    for item in entry["companions"]
                ),
            )
            for entry in data["packages"]
        )
        loaded = PublishSet(
            directory,
            str(data["repository"]),
            str(data["ref"]),
            str(data["commit"]),
            packages,
            tuple(_source_run(run) for run in data["runs"]),
        )
    except (OSError, KeyError, TypeError, ValueError) as exc:
        raise PublishSetError(f"cannot read {directory / MANIFEST_NAME}: {exc}") from exc
    if not _REF.fullmatch(loaded.ref) or not _COMMIT.fullmatch(loaded.commit):
        raise PublishSetError(f"the set names no ref and commit: {loaded.ref!r} {loaded.commit!r}")
    if (loaded.ref, loaded.commit) != (ref, commit):
        raise PublishSetError(
            f"the set holds {loaded.ref} at {loaded.commit}, not {ref} at {commit}"
        )
    names = [package.name for package in packages]
    if not names or len(set(names)) != len(names):
        raise PublishSetError(f"the set must hold distinct packages: {names}")
    built = {(run.workflow, run.run_id): run.conclusion for run in loaded.runs}
    if len(built) != len(loaded.runs) or not set(built.values()) <= publishable_conclusions(ref):
        raise PublishSetError(f"the set is not built by runs that {ref} is published from")
    for package in packages:
        if not is_publishable_name(package.name):
            raise PublishSetError(
                f"{package.name!r} is not a package name that the default branch publishes"
            )
        if (package.workflow, package.run_id) not in built:
            raise PublishSetError(f"{package.name} is built by none of the set's runs")
        path = directory / package.name
        try:
            digest = verify_sha256_sidecar(path)
            for companion in package.companions:
                if companion.name not in companion_names(package.name):
                    raise PublishSetError(
                        f"{companion.name!r} is not a file that is published beside {package.name}"
                    )
                if sha256_file(directory / companion.name) != companion.sha256:
                    raise PublishSetError(f"{companion.name} is not the file that was verified")
        except (OSError, ValueError) as exc:
            raise PublishSetError(str(exc)) from exc
        if digest != package.sha256:
            raise PublishSetError(f"{package.name} is not the file that was verified")
    return loaded


def set_table(packages: Iterable[Package]) -> str:
    """Return the packages as a Markdown table for job summaries."""
    return md_table(
        ["File", "Size", "SHA-256", "Built by"],
        [
            (code_span(p.name), format_bytes(p.size), code_span(p.sha256), p.workflow)
            for p in packages
        ],
        markdown=True,
    )


def build_facts(repo: str, runs: Iterable[SourceRun]) -> list[tuple[str, str]]:
    """Return, for job summaries, each workflow's run of the commit and how it concluded.

    A branch's packages are published although a job that builds none of them failed, and
    whoever reads what was published is told so.
    """
    return [
        (
            f"{run.workflow} build",
            f"packages built; workflow {'passed' if run.passed else f'concluded `{run.conclusion}`'} "
            f"([run {run.run_id}, attempt {run.attempt}]({run.url(repo)}))",
        )
        for run in runs
    ]


def cmd_fetch(args: argparse.Namespace) -> int:
    facts = [("Ref", code_span(args.ref)), ("Commit", code_span(args.commit))]
    try:
        runs = parse_runs(args.runs)
        fetched = fetch_set(args.repo, args.ref, args.commit, runs, args.out)
    except RerunError as exc:
        gh_warning(str(exc), title="A build was re-run: nothing is published")
        write_step_summary(
            summary_section(
                "Fetch and verify", [*facts, ("Result", "⏭️ skipped"), ("Reason", str(exc))]
            )
        )
        write_github_output({"publish": "false"})
        return 0
    except NotBuiltError as exc:
        # A build that failed is no failure to publish: the run stays green and says why.
        gh_notice(str(exc), title="Nothing to publish yet")
        write_step_summary(
            summary_section(
                "Fetch and verify",
                [*facts, ("Result", "⏭️ nothing to publish yet"), ("Reason", str(exc))],
                body="The commit is published when the failed jobs are re-run and build "
                "their packages. Otherwise the next commit that builds them all is.",
            )
        )
        write_github_output({"publish": "false"})
        return 0
    except PublishSetError as exc:
        gh_error(str(exc), title="Packages failed verification")
        write_step_summary(
            summary_section("Fetch and verify", [("Result", "❌ failed"), ("Reason", str(exc))])
        )
        return 1
    if fetched is None:
        reason = (
            "No package of these runs carries a publish marker: their workflows predate Publish "
            "Builds and publish for themselves, or none of them built a package that it marks. "
            "Nothing is published."
        )
        print(reason)
        write_step_summary(
            summary_section("Fetch and verify", [*facts, ("Result", "⏭️ skipped")], body=reason)
        )
        write_github_output({"publish": "false"})
        return 0
    companions = [
        companion.name for package in fetched.packages for companion in package.companions
    ]
    write_step_summary(
        summary_section(
            "Fetch and verify",
            [
                *facts,
                ("Result", f"✅ {len(fetched.packages)} packages verified"),
                *build_facts(args.repo, fetched.runs),
                ("For the GitHub Release only", ", ".join(map(code_span, companions))),
            ],
            body=set_table(fetched.packages),
        )
    )
    write_github_output({"publish": "true"})
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    fetch = sub.add_parser("fetch", help="Download, verify and collect the packages to publish")
    for option, variable, text in (
        ("--repo", "GITHUB_REPOSITORY", "Repository in owner/repo format"),
        ("--ref", "PUBLISH_REF", "Full ref the packages were built from"),
        ("--commit", "PUBLISH_COMMIT", "Commit the packages were built from"),
        (
            "--runs",
            "PUBLISH_RUNS",
            "JSON list of the workflow runs: {workflow, path, id, attempt, conclusion}",
        ),
    ):
        value = env_default(variable)
        fetch.add_argument(
            option, default=value, required=not value, help=f"{text} (default: ${variable})"
        )
    fetch.add_argument("--out", type=Path, required=True, help="Directory to write the set to")
    fetch.set_defaults(func=cmd_fetch)
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
