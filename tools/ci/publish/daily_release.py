#!/usr/bin/env python3
"""Update the rolling ``daily`` GitHub prerelease to a verified set of master packages.

The release always holds the packages of one ``master`` commit that every platform built. Its
assets keep the names the files have on S3, so download links stay stable, and it is updated in
place: a release per build would notify everyone who watches the repository's releases. The
notes say how each platform's workflow run concluded: ``master`` is published although a job
that builds no package failed, and the notes do not pass such a build for one that passed.

The new files are first uploaded under temporary names beside the old ones. The ``daily`` tag
then moves to the commit with an unforced update, which GitHub accepts only as a fast-forward
(the repository's tag ruleset allows no deletion and no forced update). Only then are the old
assets swapped for the new ones, which takes seconds. The release notes are written last and
record the commit: until they do, the release does not count as published and a re-run starts
over. That record, not the tag, is what keeps an older commit from taking the release back.

When GitHub refuses to move the tag, the release is updated all the same, with a warning: its
notes name the commit the files were built from and say where the tag still points, and the
next publish tries the move again. A tag that could never move would otherwise stop every
publish. Creating the tag for the first release is required.

The first publication is made as a draft, and the change that writes its notes is the one that
publishes it: the release appears with its files, and a first run that failed leaves a draft
for the tag, which the next run finds and finishes. A release that is already published is
never published again.

A rolling release needs the repository's release immutability setting to be off. While it is
on, GitHub locks every release when it is published: its files and its tag can never change
again. The workflow's token cannot read the setting, so it shows only in a release: a daily
release that reports itself immutable is refused before any change, and a first publication
that comes back immutable fails the job. Both say what to do.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path
from typing import TYPE_CHECKING, Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import (
    compare_commits,
    env_default,
    env_flag,
    gh,
    gh_api_json,
    gh_api_json_or_none,
    gh_error,
    gh_warning,
    resolve_ref_commit,
    write_step_summary,
)
from common.markdown import code_span, md_table, summary_section

from ci.publish.publish_set import (
    PublishSet,
    PublishSetError,
    build_facts,
    load_set,
    set_table,
)

if TYPE_CHECKING:
    from collections.abc import Callable, Iterable, Mapping, Sequence

DAILY_TAG = "daily"
DAILY_REF = "refs/heads/master"
RELEASE_NAME = "QGroundControl Daily Build"
# New files wait under this suffix, beside the assets they replace, until every upload is done.
TEMP_SUFFIX = ".next"
# About 1 GB of packages; ends before the job's own timeout does.
UPLOAD_TIMEOUT_SECONDS = 1200
_RECORD = re.compile(r"<!-- daily-build-commit: ([0-9a-f]{40}) -->")
# GitHub understood a change and declined it (permissions, rulesets): repeating it cannot help.
_REFUSED = re.compile(r"HTTP 4(?:03|09|22)\b")
_PENDING_NOTES = "The first daily build is being published."
_IMMUTABLE_REMEDY = (
    "To publish daily builds again: turn the setting off (Settings > General > Releases), delete "
    f"the release and then the {DAILY_TAG} tag, and give the daily release another tag name "
    "(DAILY_TAG in tools/ci/publish/daily_release.py, and what else names the tag: see "
    '"Release immutability" in .github/ci-notes/platforms.md). GitHub never lets the tag name '
    "of a deleted immutable release be used again."
)


class ReleaseAssetsError(RuntimeError):
    """A release's files cannot be replaced."""


class DailyReleaseError(ReleaseAssetsError):
    """The release cannot be moved to the commit."""


@dataclass(frozen=True)
class AssetPlan:
    """How the release's assets change: asset ids to delete, and the files that replace them."""

    stale: tuple[int, ...]
    replaced: tuple[tuple[str, int | None], ...]
    removed: tuple[int, ...]


def recorded_commit(release: Mapping[str, Any] | None) -> str | None:
    """Return the commit whose packages *release* holds, or None before its first full publish."""
    match = _RECORD.search(str((release or {}).get("body") or ""))
    return match[1] if match else None


def published_commit(repo: str) -> str | None:
    """Return the commit the repository's daily release holds, or None without one."""
    return recorded_commit(gh_api_json_or_none(f"repos/{repo}/releases/tags/{DAILY_TAG}"))


def find_releases(repo: str, tag: str) -> list[dict[str, Any]]:
    """Return every release of *repo* whose tag is *tag*, drafts included.

    ``releases/tags/<tag>`` would not do: it answers for published releases only.
    """
    result = gh(
        "api",
        "--method",
        "GET",
        "--paginate",
        "--jq",
        f'.[] | select(.tag_name == "{tag}")',
        f"repos/{repo}/releases",
        "-F",
        "per_page=100",
        retry_transient=True,
    )
    return [json.loads(line) for line in result.stdout.splitlines() if line.strip()]


def first_draft(repo: str) -> dict[str, Any] | None:
    """Return the draft that a failed first publication left for the daily tag, or None."""
    drafts = [found for found in find_releases(repo, DAILY_TAG) if found.get("draft") is True]
    if len(drafts) > 1:
        ids = ", ".join(str(draft.get("id")) for draft in drafts)
        raise DailyReleaseError(
            f"{len(drafts)} draft releases have the tag {DAILY_TAG} (ids {ids}): delete all but "
            "one, then re-run this job"
        )
    return drafts[0] if drafts else None


def plan_assets(
    assets: Iterable[Mapping[str, Any]],
    names: Sequence[str],
    *,
    owns_release: bool,
    reserved: Iterable[str] = (),
) -> AssetPlan:
    """Plan the swap of the release's *assets* for the files *names*.

    With *owns_release* the release holds this publication and nothing else, as the daily
    release does: every temporary asset is what a failed run left behind, and an asset that no
    file replaces belongs to an older commit and is removed. Without it, other people add files
    to the release: only the temporary names of *names* and of *reserved* (files of the
    publication that are already in place) are a failed run's, and no other asset is touched.
    """
    current = {str(asset["name"]): int(asset["id"]) for asset in assets}
    temporary = {name: i for name, i in current.items() if name.endswith(TEMP_SUFFIX)}
    if owns_release:
        stale = temporary.values()
        removed = [i for name, i in current.items() if name not in names and name not in temporary]
    else:
        own = {f"{name}{TEMP_SUFFIX}" for name in (*names, *reserved)}
        stale = [i for name, i in temporary.items() if name in own]
        removed = []
    return AssetPlan(
        stale=tuple(sorted(stale)),
        replaced=tuple((name, current.get(name)) for name in names),
        removed=tuple(sorted(removed)),
    )


def tag_step(
    tag_commit: str | None, commit: str, relation: Callable[[str, str], str | None]
) -> str:
    """Return ``create``, ``move`` or ``keep`` for the daily tag; raise when it cannot advance.

    *relation* is how its second commit relates to its first (``compare_commits``).
    """
    if tag_commit is None:
        return "create"
    if tag_commit == commit:
        return "keep"
    found = relation(tag_commit, commit)
    if found == "ahead":
        return "move"
    raise DailyReleaseError(
        f"the {DAILY_TAG} tag is at {tag_commit}, and {commit} is {found or 'unknown'} relative "
        "to it: the tag only moves forward"
    )


def release_notes(
    files: PublishSet,
    *,
    committed_at: str,
    published_at: datetime,
    run_url: str,
    tag_commit: str = "",
) -> str:
    """Return the release notes: the commit and its date, the files, the builds, verification.

    The builds are named with how their workflow runs concluded: every file comes from a job
    that passed, which is not to say that every job of every run did. *tag_commit* is where
    the ``daily`` tag stayed when GitHub refused to move it.
    """
    repo, commit = files.repository, files.commit
    base = f"https://github.com/{repo}"
    example = files.packages[0].name
    passed = ", ".join(f"[{run.workflow}]({run.url(repo)})" for run in files.runs if run.passed)
    builds = f"Every file was built by a job that passed, in workflow runs that passed: {passed}."
    if failed := [run for run in files.runs if not run.passed]:
        builds = (
            "Every file was built by a job that passed, but not every workflow run of this "
            "commit did: "
            + ", ".join(
                f"[{run.workflow}]({run.url(repo)}) concluded `{run.conclusion}`" for run in failed
            )
            + ". Another job of such a run failed, so this build passed fewer checks than "
            "one whose runs all passed." + (f" These runs passed: {passed}." if passed else "")
        )
    rows = [
        (
            f"[{code_span(p.name)}]({base}/releases/download/{DAILY_TAG}/{p.name})",
            code_span(p.sha256),
        )
        for p in files.packages
    ]
    published = published_at.astimezone(UTC).strftime("%Y-%m-%d %H:%M UTC")
    lagging_tag = (
        f"The `{DAILY_TAG}` tag still points at [{code_span(tag_commit[:10])}]"
        f"({base}/commit/{tag_commit}): GitHub refused to move it to the commit above, which "
        "these files are built from."
    )
    return "\n\n".join(
        [
            f"Development build of `master` at [{code_span(commit[:10])}]({base}/commit/{commit}) "
            f"(committed {committed_at}), published {published} by "
            f"[Publish Builds]({run_url or f'{base}/actions/workflows/publish.yml'}).",
            "This release is replaced whenever every platform has built the packages of a "
            "newer `master` commit, so its files change without notice and are not tested "
            f"like a stable release. The current stable release is [here]({base}/releases/latest).",
            *([lagging_tag] if tag_commit else []),
            md_table(["File", "SHA-256"], rows, markdown=True),
            builds,
            "### Verify a download",
            "Every file has a `<file>.sha256` checksum beside it and a build provenance "
            "attestation from this repository's workflows:",
            f"```sh\nsha256sum -c {example}.sha256\n"
            f"gh attestation verify {example} --repo {repo}\n```",
            f"<!-- daily-build-commit: {commit} -->",
        ]
    )


def _change(
    method: str, path: str, fields: Mapping[str, str | bool] | None = None, *, attempts: int = 3
) -> str:
    """Send a GitHub API change; retry it when it is idempotent (*attempts* > 1).

    Return GitHub's answer. Deleting what is already gone succeeds.
    """
    args = ["api", "--method", method, path]
    for key, value in (fields or {}).items():
        typed = isinstance(value, bool)
        args += ["-F" if typed else "-f", f"{key}={str(value).lower() if typed else value}"]
    for attempt in range(1, attempts + 1):
        try:
            return gh(*args).stdout
        except subprocess.CalledProcessError as error:
            if method == "DELETE" and "HTTP 404" in (error.stderr or ""):
                break
            if attempt == attempts:
                raise
            time.sleep(2.0 * attempt)
    return ""


def _stage(files: Iterable[tuple[Path, str]], staging: Path) -> list[Path]:
    """Link every file into *staging* under its temporary name."""
    if staging.exists():
        shutil.rmtree(staging)
    staging.mkdir(parents=True)
    staged = []
    for path, name in files:
        target = staging / f"{name}{TEMP_SUFFIX}"
        try:
            os.link(path, target)
        except OSError:
            shutil.copy2(path, target)
        staged.append(target)
    return staged


def upload_assets(
    repo: str,
    tag: str,
    release_id: int,
    files: Sequence[tuple[Path, str]],
    plan: AssetPlan,
    staging: Path,
    *,
    before_change: Callable[[], None] | None = None,
) -> dict[str, int]:
    """Upload *files* to the release of *tag* under temporary names; return their asset ids.

    The ids are by the name each file is to have. What a failed run left under temporary names
    is deleted first. The assets that the files replace stay as they are until ``swap_assets``.
    *before_change* is called before each change of the release, and stops the run by raising.
    """
    releases = f"repos/{repo}/releases"
    for asset_id in plan.stale:
        if before_change:
            before_change()
        _change("DELETE", f"{releases}/assets/{asset_id}")
    if not files:
        return {}
    staged = _stage(files, staging)
    if before_change:
        before_change()
    gh("release", "upload", tag, *map(str, staged), "--repo", repo, timeout=UPLOAD_TIMEOUT_SECONDS)
    uploaded = {
        str(asset["name"]): int(asset["id"])
        for asset in gh_api_json(f"{releases}/{release_id}/assets?per_page=100")
    }
    names = [name for _, name in files]
    if missing := [name for name in names if f"{name}{TEMP_SUFFIX}" not in uploaded]:
        raise ReleaseAssetsError(f"the release did not keep the uploads of {', '.join(missing)}")
    return {name: uploaded[f"{name}{TEMP_SUFFIX}"] for name in names}


def swap_assets(
    repo: str,
    plan: AssetPlan,
    uploaded: Mapping[str, int],
    *,
    before_change: Callable[[], None] | None = None,
) -> None:
    """Give every uploaded file its name, in place of the asset that had it.

    *before_change* is called before each file's turn, and stops the run by raising. It is
    not called between the removal of an asset and the renaming of its replacement.
    """
    releases = f"repos/{repo}/releases"
    for name, old in plan.replaced:
        if before_change:
            before_change()
        if old is not None:
            _change("DELETE", f"{releases}/assets/{old}")
        _change("PATCH", f"{releases}/assets/{uploaded[name]}", {"name": name})


def waiting_uploads(repo: str, release_id: int, uploaded: Mapping[str, int]) -> list[str]:
    """Return the files whose uploads of this run still wait under temporary names.

    *uploaded* is what ``upload_assets`` returned. Uploads that ``swap_assets`` has given their
    names are the release's files by now.
    """
    waiting = {
        int(asset["id"])
        for asset in gh_api_json(f"repos/{repo}/releases/{release_id}/assets?per_page=100")
        if str(asset["name"]).endswith(TEMP_SUFFIX)
    }
    return [name for name, asset_id in uploaded.items() if asset_id in waiting]


def withdraw_uploads(repo: str, release_id: int, uploaded: Mapping[str, int]) -> list[str]:
    """Delete the uploads of this run that still wait under temporary names; return their files.

    The others are the release's files by now and stay.
    """
    withdrawn = waiting_uploads(repo, release_id, uploaded)
    for name in withdrawn:
        _change("DELETE", f"repos/{repo}/releases/assets/{uploaded[name]}")
    return withdrawn


def update_release(
    files: PublishSet, *, dry_run: bool, run_url: str = "", now: datetime | None = None
) -> str:
    """Move the daily release to the set's commit; return the result for the job summary."""
    repo, commit = files.repository, files.commit
    releases = f"repos/{repo}/releases"
    tag_ref = f"repos/{repo}/git/refs/tags/{DAILY_TAG}"
    names = [name for _, name in files.files()]
    release = gh_api_json_or_none(f"{releases}/tags/{DAILY_TAG}")
    if release and release.get("immutable") is True:
        raise DailyReleaseError(
            "The daily release is immutable: the repository's release immutability setting was "
            "on when it was published. GitHub accepts no change to its files or to its tag, so "
            f"it cannot be updated in place. {_IMMUTABLE_REMEDY}"
        )
    # GitHub answers by tag for a published release only.
    first = release is None
    if first:
        release = first_draft(repo)
    tag_commit = resolve_ref_commit(repo, f"tags/{DAILY_TAG}")
    step = tag_step(tag_commit, commit, lambda base, head: compare_commits(repo, base, head))
    # The tag lags behind the files after a refused move, so it alone does not show a step back.
    held = recorded_commit(release)
    if held not in (None, commit, tag_commit):
        found = compare_commits(repo, held, commit)
        if found != "ahead":
            raise DailyReleaseError(
                f"the release holds {held}, and {commit} is {found or 'unknown'} relative to "
                "it: the release only moves forward"
            )
    plan = plan_assets((release or {}).get("assets") or [], names, owns_release=True)
    replaced = sum(old is not None for _, old in plan.replaced)
    action = "create" if release is None else "publish the draft of a failed run"
    print(
        f"release: {action if first else 'update'}; tag: {step}; assets: {replaced} replaced, "
        f"{len(names) - replaced} added, {len(plan.removed)} removed, {len(plan.stale)} stale"
    )
    if dry_run:
        return "🧪 dry run: the release was not changed"

    if step == "create":
        _change(
            "POST",
            f"repos/{repo}/git/refs",
            {"ref": f"refs/tags/{DAILY_TAG}", "sha": commit},
            attempts=1,
        )
    # A prerelease is never the repository's latest release, which the README's buttons download.
    settings: dict[str, str | bool] = {
        "name": RELEASE_NAME,
        "prerelease": True,
        "make_latest": "false",
    }
    if release is None:
        fields = {"tag_name": DAILY_TAG, "body": _PENDING_NOTES, "draft": True, **settings}
        release = json.loads(_change("POST", releases, fields, attempts=1))
    release_id = int(release["id"])
    staging = files.directory.with_name(f"{files.directory.name}.staging")
    uploaded = upload_assets(repo, DAILY_TAG, release_id, files.files(), plan, staging)

    lagging_tag = ""
    if step == "move":
        try:
            _change("PATCH", tag_ref, {"sha": commit, "force": False})
        except subprocess.CalledProcessError as error:
            detail = (error.stderr or "").strip()
            if not _REFUSED.search(detail):
                raise DailyReleaseError(
                    f"the {DAILY_TAG} tag could not be moved to {commit}, so the release keeps "
                    f"its previous files: {detail}"
                ) from error
            lagging_tag = str(tag_commit)
            gh_warning(
                f"GitHub refused to move the {DAILY_TAG} tag from {lagging_tag} to {commit} "
                f"({detail}). The release is updated all the same and its notes say where the "
                "tag points; the next publish tries again.",
                title="Daily tag not moved",
            )
    swap_assets(repo, plan, uploaded)
    for asset_id in plan.removed:
        _change("DELETE", f"{releases}/assets/{asset_id}")

    committed_at = gh_api_json(f"repos/{repo}/commits/{commit}")["commit"]["committer"]["date"]
    notes = release_notes(
        files,
        committed_at=str(committed_at),
        published_at=now or datetime.now(UTC),
        run_url=run_url,
        tag_commit=lagging_tag,
    )
    # Only the first publication sends `draft`: publishing a release again is what makes one
    # that was published before the immutability setting was turned on immutable.
    publish: dict[str, str | bool] = {"draft": False} if first else {}
    _change("PATCH", f"{releases}/{release_id}", {"body": notes, **settings, **publish})
    if first and gh_api_json(f"{releases}/{release_id}").get("immutable") is True:
        raise DailyReleaseError(
            f"The daily release was published complete, with the files of {commit}, but GitHub "
            "locked it: the repository's release immutability setting is on, which makes every "
            "release immutable when it is published. No later commit can update it. "
            f"{_IMMUTABLE_REMEDY}"
        )
    if lagging_tag:
        return f"⚠️ published, but the `{DAILY_TAG}` tag still points at {code_span(lagging_tag)}"
    return "✅ published"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--set", dest="directory", type=Path, required=True, metavar="DIR")
    for option, variable in (("--ref", "PUBLISH_REF"), ("--commit", "PUBLISH_COMMIT")):
        value = env_default(variable)
        parser.add_argument(
            option, default=value, required=not value, help=f"(default: ${variable})"
        )
    parser.add_argument(
        "--run-url",
        default=env_default("PUBLISH_RUN_URL"),
        help="The publishing workflow run, for the release notes (default: $PUBLISH_RUN_URL)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=env_flag("PUBLISH_DRY_RUN"),
        help="Print what would change without changing it (default: $PUBLISH_DRY_RUN)",
    )
    args = parser.parse_args(argv)

    try:
        files = load_set(args.directory, ref=args.ref, commit=args.commit)
        if files.ref != DAILY_REF:
            raise DailyReleaseError(f"the daily release follows {DAILY_REF}, not {files.ref}")
        result = update_release(files, dry_run=args.dry_run, run_url=args.run_url)
    except (PublishSetError, ReleaseAssetsError, subprocess.CalledProcessError) as exc:
        reason = str(getattr(exc, "stderr", None) or exc).strip()
        gh_error(reason, title="Daily release not updated")
        write_step_summary(
            summary_section("GitHub daily release", [("Result", "❌ failed"), ("Reason", reason)])
        )
        return 1
    release_url = f"https://github.com/{files.repository}/releases/tag/{DAILY_TAG}"
    write_step_summary(
        summary_section(
            "GitHub daily release",
            [
                ("Result", result),
                ("Commit", code_span(files.commit)),
                ("Release", release_url),
                *build_facts(files.repository, files.runs),
            ],
            body=set_table(files.packages),
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
