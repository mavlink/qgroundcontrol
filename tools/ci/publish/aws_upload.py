#!/usr/bin/env python3
"""Publish a verified set of build packages to AWS S3 and CloudFront.

``publish`` uploads every file of the set that ``publish_set.py fetch`` verified to
``builds/<ref>/``. ``promote`` uploads them to ``latest/``, but only for a ``vX.Y.Z`` tag
reachable from a ``Stable*`` branch. Each then writes ``published.json`` beside the files: the
record of the repository, ref and commit the directory holds, of the files that belong to
them, and of the workflow runs that built them with how each concluded (a branch is published
although a job that builds no package failed). It is written last, so a command that failed
midway left no record and runs again in full.

``invalidate`` drops CloudFront's copies of ``builds/<ref>/``, or of ``latest/`` with
``--latest``, with one wildcard path. The job runs it after ``publish`` or ``promote``, and
also when that had nothing to do, so that re-running the job after a failed invalidation
repeats the invalidation alone: the files and the record are in S3 by then, whatever
CloudFront serves.

Exactly the set is uploaded, whatever packages it has, and nothing is ever deleted. A file under
a name that the set no longer has therefore stays where it was: the record and the job summary
list what belongs to the publication, and the command warns of the names that the previous
record lists and this set lacks.

Before its first upload each command reads that record and stops when the directory would go
backwards, when it holds another repository's build, or when S3 does not say what it holds.
``--record-read`` (``PUBLISH_RECORD_READ``) says how. ``public``, the default, fetches the
record's public URL, as ``publish_decide.py`` does and as anyone can: the role needs no read
permission, a missing object (403, or 404) counts as nothing published, and any other answer
stops the command. A record that exists and is not public therefore passes for none, which is
why each command reads its own record back from that URL after the upload and fails when it
cannot. ``credentialed`` reads with ``aws s3api get-object`` instead, which tells the two apart:
only "no such key" counts as nothing published. It needs ``s3:GetObject`` on the records and
``s3:ListBucket`` on the bucket.

``latest/`` is the exception: it held a release before this workflow recorded any, so without
a record ``promote`` stops instead of taking it for empty. ``--force-latest``, which only a
request by hand sets, replaces whatever ``latest/`` holds: a release it has no record of, or a
newer one. ``seed`` prints the record of a release that ``latest/`` already holds, for an
administrator to upload once; it reads no set and calls nothing.

AWS credentials come from the environment, never from the command line. Without any
(``HAS_AWS_CREDENTIALS=false``) nothing is uploaded and the command warns instead of failing.
An ``aws`` command that fails, or that cannot be started, ends the command with an error
annotation and status 1.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path
from typing import TYPE_CHECKING, Any
from urllib.error import HTTPError

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.aws import upload_public_file
from common.gh_actions import (
    compare_commits,
    env_default,
    env_flag,
    gh_error,
    gh_warning,
    write_step_summary,
)
from common.io import write_json
from common.markdown import code_span, summary_section
from common.net import fetch_bytes
from common.proc import run_captured
from qgc_tools.release_tag_check import (
    GitQueryError,
    check_release_tag,
    is_release_tag,
    release_version,
)

from ci.publish.publish_set import (
    PublishSet,
    PublishSetError,
    build_facts,
    load_set,
    set_table,
    tag_problem,
)

if TYPE_CHECKING:
    from collections.abc import Mapping, Sequence

S3_BUCKET = "qgroundcontrol"
# The repository whose builds the bucket holds.
S3_REPOSITORY = "mavlink/qgroundcontrol"
# The bucket itself, not CloudFront, whose cached copy of the record could be an older one.
S3_URL = "https://qgroundcontrol.s3.us-west-2.amazonaws.com"
LATEST_PREFIX = "latest"
STATE_NAME = "published.json"
STATE_SCHEMA_VERSION = 1
# How a command reads the record of the directory it is about to write: from the public URL,
# or with the job's AWS credentials. The first is the default.
RECORD_READS = ("public", "credentialed")
RECORD_READ_VARIABLE = "PUBLISH_RECORD_READ"
_COMMIT = re.compile(r"[0-9a-f]{40}")
_REF = re.compile(r"refs/(?:heads|tags)/([A-Za-z0-9._-]+)")
# S3's answer to a read of an object that does not exist, given only to a caller that may list
# the bucket: any other caller is denied, whether the object exists or not.
_NO_SUCH_KEY = re.compile(r"\((?:NoSuchKey|404)\)")
ROLE_PERMISSIONS = (
    "The publishing role needs s3:GetObject on builds/*/published.json and latest/published.json, "
    "s3:ListBucket on the bucket (without it S3 denies the read of a missing record instead of "
    "reporting it missing), s3:PutObject and s3:PutObjectAcl on builds/* and latest/*, and "
    "cloudfront:CreateInvalidation on the distribution."
)


class PublishedStateError(RuntimeError):
    """A directory's ``published.json`` cannot be read or is not a record this script wrote."""


class UploadError(RuntimeError):
    """A file was not written to S3, or the record that was written cannot be read back."""


@dataclass(frozen=True)
class Record:
    """What a directory's ``published.json`` says the directory holds."""

    commit: str
    ref: str = ""
    # The package names it lists as belonging to the publication.
    files: tuple[str, ...] = ()
    # The repository whose build it is, or "" when the record does not say.
    repository: str = ""

    @property
    def release(self) -> str | None:
        """The release tag the directory was published from, or None when it names none."""
        name = self.ref.removeprefix("refs/tags/")
        return name if name != self.ref and is_release_tag(name) else None


@dataclass(frozen=True)
class S3Plan:
    """The objects one command writes, in order."""

    prefix: str
    uploads: tuple[tuple[Path, str], ...]
    state_key: str


def sanitize_ref(ref_name: str) -> str:
    """Remove path traversal and unsafe characters from a git ref name."""
    safe = ref_name.replace("..", "")
    return re.sub(r"[^a-zA-Z0-9._-]", "_", safe)


def builds_prefix(ref_name: str) -> str:
    return f"builds/{sanitize_ref(ref_name)}"


def plan_uploads(files: PublishSet, prefix: str) -> S3Plan:
    """Plan the upload of every file of the set under *prefix*, then its record."""
    uploads = tuple((path, f"{prefix}/{name}") for path, name in files.files())
    return S3Plan(prefix, uploads, f"{prefix}/{STATE_NAME}")


def _state(
    repository: str,
    ref: str,
    commit: str,
    files: Mapping[str, str],
    *,
    run_url: str,
    published_at: datetime,
) -> dict[str, Any]:
    return {
        "schema_version": STATE_SCHEMA_VERSION,
        "repository": repository,
        "ref": ref,
        "commit": commit,
        "published_at": published_at.astimezone(UTC).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "run": run_url,
        "files": dict(files),
    }


def published_state(files: PublishSet, *, run_url: str, published_at: datetime) -> dict[str, Any]:
    """Return the record of the set a directory holds.

    ``builds`` says how each workflow's run for the commit had concluded when its packages
    were published, at which attempt: a run that is re-run later does not change the record.
    """
    state = _state(
        files.repository,
        files.ref,
        files.commit,
        {package.name: package.sha256 for package in files.packages},
        run_url=run_url,
        published_at=published_at,
    )
    state["builds"] = [
        {
            "workflow": run.workflow,
            "run": run.url(files.repository),
            "attempt": run.attempt,
            "conclusion": run.conclusion,
        }
        for run in files.runs
    ]
    return state


def seed_record(
    tag: str, commit: str, *, repository: str, published_at: datetime
) -> dict[str, Any]:
    """Return the record of release *tag* at *commit*, for a ``latest/`` that holds it unrecorded.

    It is the record a publication writes, without a run and without the files, whose
    checksums nothing here can vouch for. Raises ValueError unless *tag* is a ``vX.Y.Z`` tag
    and *commit* a full commit id.
    """
    if not is_release_tag(tag):
        raise ValueError(f"{tag!r} is not a vX.Y.Z release tag")
    if not _COMMIT.fullmatch(commit):
        raise ValueError(f"{commit!r} is not a full commit id")
    return _state(repository, f"refs/tags/{tag}", commit, {}, run_url="", published_at=published_at)


def _record(source: str, text: str | bytes) -> Record:
    try:
        data = json.loads(text)
    except ValueError as exc:
        raise PublishedStateError(f"cannot read {source}: {exc}") from exc
    commit = data.get("commit") if isinstance(data, dict) else None
    if not isinstance(commit, str) or not _COMMIT.fullmatch(commit):
        raise PublishedStateError(f"{source} does not record a commit")
    ref, files, repository = data.get("ref"), data.get("files"), data.get("repository")
    return Record(
        commit,
        ref if isinstance(ref, str) else "",
        tuple(sorted(name for name in files if isinstance(name, str)))
        if isinstance(files, dict)
        else (),
        repository if isinstance(repository, str) else "",
    )


def public_record(prefix: str) -> Record | None:
    """Return what ``<prefix>/published.json`` records as anyone can read it, or None.

    None is no proof that nothing is published: S3 answers 403 both for a missing object and
    for one that is not public. A command finds out that its own record is not public by
    reading it back after the upload (``execute``), and ``read_record`` tells the two apart.
    Raises PublishedStateError when the record cannot be fetched or does not name a commit.
    """
    url = f"{S3_URL}/{prefix}/{STATE_NAME}"
    try:
        text = fetch_bytes(url, timeout=30)
    except HTTPError as exc:
        if exc.code in (403, 404):
            return None
        raise PublishedStateError(f"cannot read {url}: HTTP {exc.code}") from exc
    except (OSError, RuntimeError) as exc:
        raise PublishedStateError(f"cannot read {url}: {exc}") from exc
    return _record(url, text)


def read_record(prefix: str, scratch: Path) -> Record | None:
    """Return what ``<prefix>/published.json`` records, read with the job's AWS credentials.

    None only when S3 reports that the object does not exist. Access denied, and any other
    failure, raises PublishedStateError: the directory may hold a newer build. *scratch* is
    where the record is downloaded to.
    """
    key = f"{prefix}/{STATE_NAME}"
    source = f"s3://{S3_BUCKET}/{key}"
    command = ["aws", "s3api", "get-object", "--bucket", S3_BUCKET, "--key", key, str(scratch)]
    try:
        result = run_captured(command)
    except OSError as exc:
        raise PublishedStateError(f"cannot read {source}, so nothing is uploaded: {exc}") from exc
    if result.returncode != 0:
        detail = (result.stderr or "").strip() or f"aws exited with status {result.returncode}"
        if _NO_SUCH_KEY.search(detail):
            return None
        raise PublishedStateError(
            f"cannot read {source}, so nothing is uploaded: {detail}. {ROLE_PERMISSIONS}"
        )
    try:
        return _record(source, scratch.read_bytes())
    except OSError as exc:
        raise PublishedStateError(f"cannot read {source}: {exc}") from exc


def held_record(prefix: str, read: str, scratch: Path) -> Record | None:
    """Return the record of what *prefix*/ holds before anything is uploaded there, or None.

    *read* is one of RECORD_READS. Raises PublishedStateError when the read does not say what
    the directory holds.
    """
    if read == "credentialed":
        return read_record(prefix, scratch)
    try:
        return public_record(prefix)
    except PublishedStateError as exc:
        raise PublishedStateError(
            f"{exc}, so nothing is uploaded: what the directory holds is unknown"
        ) from exc


def regression(
    held: Record | None, files: PublishSet, prefix: str, *, force: bool = False
) -> str | None:
    """Return why *prefix*/ keeps what it holds instead of taking the set, or None when it may.

    A branch's directory only moves forward, and a tag's own directory takes what the tag
    names; without a record either holds nothing. ``latest/`` held a release before it had a
    record, so it takes only the release its record names or a newer one, and none without a
    record, unless *force* says to replace whatever it holds. Another repository's build is
    not compared with and not replaced: its commits mean nothing here.
    """
    if prefix == LATEST_PREFIX and force:
        return None
    if held is not None and held.repository not in ("", files.repository):
        return (
            f"holds a build of {held.repository}, not of {files.repository}: its record must "
            "be removed before this repository publishes there"
        )
    if prefix == LATEST_PREFIX:
        if held is None:
            return (
                f"has no publication record ({STATE_NAME}), so the release it holds cannot be "
                f"compared with {files.ref_name}. Write the record of the release that "
                "latest/ holds (`aws_upload.py seed`, "
                ".github/ci-notes/platforms.md#seeding-the-latest-record) and re-run this job, "
                f"or publish {files.ref_name} by hand with force_latest."
            )
        version = release_version(held.release or "")
        if version is None:
            return f"holds {held.commit}, and its record does not name the release"
        # promote admits release tags only, so the set's ref has a version.
        if version > (release_version(files.ref_name) or (0, 0, 0)):
            return f"holds the newer release {held.release}"
        return None
    if held is None or files.is_tag or held.commit == files.commit:
        return None
    found = compare_commits(files.repository, held.commit, files.commit)
    if found == "ahead":
        return None
    return (
        f"holds {held.commit}, and {files.commit} is {found or 'unknown to the repository'} "
        "relative to it"
    )


def _upload(path: Path, key: str) -> None:
    try:
        upload_public_file(path, S3_BUCKET, key)
    except (RuntimeError, ValueError) as error:
        raise UploadError(str(error)) from error
    except OSError as error:
        raise UploadError(f"S3 upload failed for s3://{S3_BUCKET}/{key}: {error}") from error


def execute(plan: S3Plan, state: dict[str, Any], state_path: Path) -> None:
    """Upload the planned files and then the record, which anyone must be able to read."""
    for path, key in plan.uploads:
        _upload(path, key)
    write_json(state_path, state)
    _upload(state_path, plan.state_key)
    why = ""
    try:
        record = public_record(plan.prefix)
    except PublishedStateError as error:
        record, why = None, f" ({error})"
    if record is None or record.commit != state["commit"]:
        raise UploadError(
            f"s3://{S3_BUCKET}/{plan.state_key} was uploaded but cannot be read back from "
            f"{S3_URL}/{plan.state_key}{why}. A run that reads the record there (the default) "
            "takes a record it cannot read for none, which switches the never-regress check "
            "off: an older commit can then replace a newer one in a builds/ directory, and "
            "latest/ refuses every release. Do not ignore this failure: make the object "
            "publicly readable (the bucket's public-access and ACL settings), then re-run "
            "this job."
        )


def _summarize(
    title: str, files: PublishSet, plan: S3Plan, result: str, *, left: Sequence[str] = ()
) -> None:
    """Write the job summary: what was published, and the files *left* from an earlier set."""
    write_step_summary(
        summary_section(
            title,
            [
                ("Result", result),
                ("Ref", code_span(files.ref)),
                ("Commit", code_span(files.commit)),
                ("Record", f"{S3_URL}/{plan.state_key}"),
                ("Objects", len(plan.uploads) + 1),
                *build_facts(files.repository, files.runs),
                ("Left from an earlier publication", ", ".join(map(code_span, left))),
            ],
            body=set_table(files.packages),
        )
    )


def _run(args: argparse.Namespace, files: PublishSet, plan: S3Plan, *, force: bool = False) -> int:
    title = f"S3 {plan.prefix}/"
    for path, key in plan.uploads:
        print(f"{path.name} -> s3://{S3_BUCKET}/{key}")
    print(f"record -> s3://{S3_BUCKET}/{plan.state_key}")
    if args.dry_run:
        _summarize(title, files, plan, "🧪 dry run: nothing was uploaded")
        return 0
    if not env_flag("HAS_AWS_CREDENTIALS", default=True):
        gh_warning(f"No AWS credentials are configured: nothing was uploaded to {plan.prefix}/")
        _summarize(title, files, plan, "⚠️ skipped: no AWS credentials are configured")
        return 0
    state_path = files.directory / STATE_NAME
    try:
        held = held_record(plan.prefix, args.record_read, state_path.with_suffix(".held.json"))
        refused = regression(held, files, plan.prefix, force=force)
    except (PublishedStateError, subprocess.CalledProcessError) as exc:
        gh_error(str(getattr(exc, "stderr", None) or exc).strip(), title=f"{title} not published")
        return 1
    if refused:
        gh_error(f"Nothing is uploaded: {plan.prefix}/ {refused}", title=f"{title} not published")
        return 1
    # The job waited its turn since the decision, and a tag can be moved meanwhile.
    if files.is_tag and (moved := tag_problem(files.repository, files.ref_name, files.commit)):
        gh_error(f"Nothing is uploaded: {moved}", title=f"{title} not published")
        return 1
    state = published_state(files, run_url=args.run_url, published_at=datetime.now(UTC))
    try:
        execute(plan, state, state_path)
    except UploadError as exc:
        gh_error(str(exc), title=f"{title} not published")
        return 1
    names = {package.name for package in files.packages}
    left = sorted(set(held.files) - names) if held else []
    if left:
        gh_warning(
            f"{plan.prefix}/ still holds {', '.join(left)} from an earlier publication. This "
            "one has no such package and nothing deletes the files, so they stay downloadable: "
            "delete them by hand, with their .sha256 files, if they should not be.",
            title=f"{title} holds files that are not part of this publication",
        )
    _summarize(title, files, plan, "✅ published", left=left)
    return 0


def cmd_publish(args: argparse.Namespace, files: PublishSet) -> int:
    return _run(args, files, plan_uploads(files, builds_prefix(files.ref_name)))


def cmd_promote(args: argparse.Namespace, files: PublishSet) -> int:
    # latest/ is the public stable download: only release tags on a Stable* branch may overwrite it.
    if not files.is_tag:
        gh_error(f"{files.ref} is not a tag: not publishing to latest/")
        return 1
    try:
        release_tag = check_release_tag(
            files.ref_name, "not publishing to latest/", commit=files.commit
        )
    except GitQueryError as exc:
        gh_error(f"Cannot determine whether tag is on a Stable* branch: {exc}")
        return 1
    if not release_tag:
        gh_error(f"{files.ref_name} is not a release tag on a Stable* branch")
        return 1
    return _run(args, files, plan_uploads(files, LATEST_PREFIX), force=args.force_latest)


def cmd_invalidate(args: argparse.Namespace) -> int:
    if args.latest:
        prefix = LATEST_PREFIX
    elif ref := _REF.fullmatch(args.ref):
        prefix = builds_prefix(ref[1])
    else:
        gh_error(f"{args.ref!r} is not a branch or tag whose builds/ directory can be invalidated")
        return 1
    # CloudFront takes a wildcard only at the end of a path, and counts the path as one.
    path = f"/{prefix}/*"
    title = f"CloudFront {prefix}/"
    print(f"one CloudFront invalidation: {path}")
    if args.dry_run:
        result = "🧪 dry run: nothing was invalidated"
    elif not env_flag("HAS_AWS_CREDENTIALS", default=True):
        result = "⏭️ skipped: no AWS credentials are configured"
    elif not args.distribution_id:
        gh_warning(
            "No CloudFront distribution is configured (AWS_DISTRIBUTION_ID): cached copies of "
            f"{prefix}/ are served until they expire"
        )
        result = "⚠️ skipped: no CloudFront distribution is configured"
    else:
        command = ["aws", "cloudfront", "create-invalidation"]
        try:
            run_captured(
                [*command, "--distribution-id", args.distribution_id, "--paths", path], check=True
            )
        except (subprocess.CalledProcessError, OSError) as exc:
            detail = str(getattr(exc, "stderr", None) or exc).strip()
            gh_error(
                f"The files and the record of {prefix}/ are in S3 and are correct. Only the "
                f"CloudFront invalidation of {path} failed, so CloudFront may serve the previous "
                f"files until its copies expire: {detail}. Re-run this job to send the "
                "invalidation again. Only a run that was started by hand, like a new dispatch "
                "for the ref, uploads the files again first.",
                title=f"{title} not invalidated",
            )
            write_step_summary(summary_section(title, [("Result", "❌ not invalidated")]))
            return 1
        result = f"✅ invalidated {code_span(path)}"
    print(result)
    write_step_summary(summary_section(title, [("Result", result)]))
    return 0


def cmd_seed(args: argparse.Namespace) -> int:
    try:
        record = seed_record(
            args.tag, args.commit, repository=args.repo, published_at=datetime.now(UTC)
        )
    except ValueError as exc:
        gh_error(str(exc))
        return 1
    # As write_json lays out the records that are uploaded.
    print(json.dumps(record, indent=2))
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    commands = (
        ("publish", cmd_publish, "Upload the set to builds/<ref>/"),
        ("promote", cmd_promote, "Upload a release tag's set to latest/"),
    )
    for name, func, text in commands:
        command = sub.add_parser(name, help=text)
        command.add_argument("--set", dest="directory", type=Path, required=True, metavar="DIR")
        for option, variable in (("--ref", "PUBLISH_REF"), ("--commit", "PUBLISH_COMMIT")):
            value = env_default(variable)
            command.add_argument(
                option, default=value, required=not value, help=f"(default: ${variable})"
            )
        command.add_argument(
            "--run-url",
            default=env_default("PUBLISH_RUN_URL"),
            help="The publishing workflow run, for the record (default: $PUBLISH_RUN_URL)",
        )
        command.add_argument(
            "--dry-run",
            action="store_true",
            default=env_flag("PUBLISH_DRY_RUN"),
            help="Print what would be uploaded without uploading (default: $PUBLISH_DRY_RUN)",
        )
        command.add_argument(
            "--record-read",
            default=env_default(RECORD_READ_VARIABLE, RECORD_READS[0]),
            metavar=f"{{{','.join(RECORD_READS)}}}",
            help="Read the record of what the directory holds from its public URL, or with the "
            f"AWS credentials (default: ${RECORD_READ_VARIABLE}, else {RECORD_READS[0]})",
        )
        if name == "promote":
            command.add_argument(
                "--force-latest",
                action="store_true",
                default=env_flag("PUBLISH_FORCE_LATEST"),
                help="Replace whatever latest/ holds: a newer release, or one it has no record "
                "of (default: $PUBLISH_FORCE_LATEST)",
            )
        command.set_defaults(func=func)
    seed = sub.add_parser(
        "seed", help="Print the record of a release that latest/ holds without one"
    )
    seed.add_argument("--tag", required=True, help="The release in latest/, as vX.Y.Z")
    seed.add_argument("--commit", required=True, help="The full id of the commit the tag names")
    seed.add_argument(
        "--repo", default=S3_REPOSITORY, help="Repository of the release (default: %(default)s)"
    )
    invalidate = sub.add_parser(
        "invalidate",
        help="Invalidate builds/<ref>/ or latest/ in CloudFront with one wildcard path",
    )
    invalidate.add_argument(
        "--ref",
        default=env_default("PUBLISH_REF"),
        help="Full ref whose builds/ directory to invalidate (default: $PUBLISH_REF)",
    )
    invalidate.add_argument(
        "--latest", action="store_true", help="Invalidate latest/ instead of the ref's directory"
    )
    invalidate.add_argument("--distribution-id", default="", help="CloudFront distribution")
    invalidate.add_argument(
        "--dry-run",
        action="store_true",
        default=env_flag("PUBLISH_DRY_RUN"),
        help="Print the path without invalidating it (default: $PUBLISH_DRY_RUN)",
    )

    args = parser.parse_args(argv)
    if args.command == "seed":
        return cmd_seed(args)
    if args.command == "invalidate":
        return cmd_invalidate(args)
    if args.record_read not in RECORD_READS:
        gh_error(
            f"The record read must be {' or '.join(RECORD_READS)}, not {args.record_read!r} "
            f"(--record-read, {RECORD_READ_VARIABLE})"
        )
        return 1
    try:
        files = load_set(args.directory, ref=args.ref, commit=args.commit)
    except PublishSetError as exc:
        gh_error(str(exc))
        return 1
    return args.func(args, files)


if __name__ == "__main__":
    raise SystemExit(main())
