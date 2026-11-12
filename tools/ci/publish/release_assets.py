#!/usr/bin/env python3
"""Attach a release tag's verified packages to the tag's GitHub Release.

The Release workflow drafts the tag's release when the tag is pushed. Once every platform has
built the tag, this uploads the packages of the verified set to that release, each with its
``.sha256``, under the names they have on S3, and each AppImage's ``.zsync`` when its build
embeds update information: that names the repository's latest release as the place where an
updater finds the file. Only a draft is changed. A tag without a release is waited for briefly
and then fails the job: the release is ``release.yml``'s to create, never this script's.

A published release keeps its files, so that nothing changes under its users, unless
``--force-release`` says to replace them, which only a request by hand sets. Without it a
published release is only checked: the job passes when the release has every file of the set,
byte for byte, and fails when it does not, naming the files and the request that attaches
them. A release that is published before its packages arrive is therefore a failed job, not a
release without downloads that nobody notices. The one thing removed from a published release
is what a failed run of this job left under its temporary names.

A release that GitHub reports as immutable was published while the repository's release
immutability setting was on, and GitHub accepts no change to its files, ``--force-release`` or
not. It is only ever checked, and when it lacks a file the job fails and says that nothing can
attach it. A draft that becomes such a release while the job attaches is changed no further,
and keeps the temporary uploads it has.

The REST API answers by tag for published releases only. A draft is found in the listing of
releases, which shows drafts to a token that may write, and is addressed by its id from then on.

Only assets are written: never the notes, the title, or whether the release is a draft, a
prerelease or the latest. A file the release already has, byte for byte, is left alone, so a
second run changes nothing. The others are uploaded under temporary names beside the assets
they replace and swapped in afterwards (``daily_release.py``'s helpers): an interrupted run
leaves the old asset or a temporary one, which the next run deletes. Only the temporary names
of the set's own files are ever cleaned up: any other asset is someone else's and stays,
whatever its name ends in.

The job waits its turn and then uploads for minutes, and both the tag and the release can
change meanwhile. The tag is resolved again before the uploads and before the swap, and must
still name the set's commit. The release is read again by its id before every change, and a
draft that was published meanwhile is changed no further: the run withdraws the temporary
uploads it made, which users of the release would otherwise see, leaves what is in place,
and fails with what remains to do. A release that is published in the moment between such a
check and the change that follows it still gets that one change.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
import time
from pathlib import Path
from typing import TYPE_CHECKING, Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import (
    env_default,
    env_flag,
    gh_api_json,
    gh_error,
    gh_notice,
    gh_warning,
    write_step_summary,
)
from common.io import sha256_file
from common.markdown import code_span, summary_section
from qgc_tools.release_tag_check import is_release_tag

from ci.publish.daily_release import (
    AssetPlan,
    ReleaseAssetsError,
    find_releases,
    plan_assets,
    swap_assets,
    upload_assets,
    waiting_uploads,
    withdraw_uploads,
)
from ci.publish.publish_set import PublishSet, PublishSetError, load_set, set_table, tag_problem

if TYPE_CHECKING:
    from collections.abc import Iterable, Mapping, Sequence

# release.yml drafts the release minutes after the tag push, and the builds take about an hour:
# a draft that is missing now is late by more than this wait can cover.
DRAFT_WAIT_ATTEMPTS = 6
DRAFT_WAIT_SECONDS = 30.0


class _Stopped(ReleaseAssetsError):
    """The tag or the release changed under the run, which goes no further.

    *locked* is that the release became immutable, so that nothing of it can be withdrawn.
    """

    def __init__(self, reason: str, remedy: str, *, locked: bool = False) -> None:
        super().__init__(reason)
        self.remedy = remedy
        self.locked = locked


def _on_s3(tag: str) -> str:
    return (
        f"The download server has this build's files under builds/{tag}/ when the S3 builds/ "
        "job of this run passed."
    )


def tag_release(repo: str, tag: str, *, wait: bool) -> dict[str, Any] | None:
    """Return the one release of *tag*, or None without any; *wait* for a draft that is late."""
    attempts = DRAFT_WAIT_ATTEMPTS if wait else 1
    for attempt in range(1, attempts + 1):
        found = find_releases(repo, tag)
        if len(found) > 1:
            ids = ", ".join(str(release.get("id")) for release in found)
            raise ReleaseAssetsError(
                f"{len(found)} releases have the tag {tag} (ids {ids}): delete all but the one "
                "to keep, then re-run this job"
            )
        if found:
            return found[0]
        if attempt < attempts:
            print(f"no release for {tag} yet: waiting {DRAFT_WAIT_SECONDS:.0f} s for its draft")
            time.sleep(DRAFT_WAIT_SECONDS)
    return None


def changed_files(files: PublishSet, assets: Iterable[Mapping[str, Any]]) -> list[tuple[Path, str]]:
    """Return the files of the set that the release lacks, or holds with other content.

    An asset whose upload did not finish, or whose digest GitHub does not give, counts as other
    content.
    """
    held = {str(asset["name"]): asset for asset in assets}
    digests = {package.name: package.sha256 for package in files.packages}
    digests |= {c.name: c.sha256 for package in files.packages for c in package.companions}
    changed = []
    for path, name in files.release_files():
        asset = held.get(name, {})
        digest = digests.get(name) or sha256_file(path)
        if asset.get("state") != "uploaded" or asset.get("digest") != f"sha256:{digest}":
            changed.append((path, name))
    return changed


def check_published(
    files: PublishSet,
    release_id: int,
    assets: Sequence[Mapping[str, Any]],
    changed: Sequence[tuple[Path, str]],
    plan: AssetPlan,
    *,
    dry_run: bool,
    immutable: bool = False,
) -> str:
    """Return the result for a published release that has the set's files; raise when not.

    Nothing of it is changed without force_release, except that the temporary uploads a
    failed run of this job left are removed: they are no files of the release, and its users
    see them. The job's concurrency group keeps another run from uploading meanwhile. An
    *immutable* release keeps them too: GitHub accepts no change to its files.
    """
    repo, tag = files.repository, files.ref_name
    total = len(files.release_files())
    held = {str(asset["name"]) for asset in assets}
    lacking = [name for _, name in changed if name not in held]
    differing = [name for _, name in changed if name in held]
    found = []
    if lacking:
        found.append(f"lacks {len(lacking)} of this build's {total} files ({', '.join(lacking)})")
    if differing:
        which = "" if lacking else f" of this build's {total} files"
        found.append(f"holds {len(differing)}{which} with other content ({', '.join(differing)})")
    remedy = " and ".join(["replace"] * bool(differing) + ["attach"] * bool(lacking))
    if dry_run:
        if not changed:
            return "🧪 dry run: the published release already has these files"
        if immutable:
            return (
                f"🧪 dry run: the published release {' and '.join(found)}, and its job would "
                f"fail: the release is immutable, so force_release cannot {remedy} them"
            )
        return (
            f"🧪 dry run: the published release {' and '.join(found)}, and its job would fail "
            "without force_release"
        )
    if plan.stale and immutable:
        gh_warning(
            f"{len(plan.stale)} temporary uploads (<file>.next) that a failed run of this job "
            f"left stay on the published {tag} release: it is immutable, and GitHub accepts no "
            "change to its files.",
            title="Temporary uploads not removed",
        )
    elif plan.stale:
        staging = files.directory.with_name(f"{files.directory.name}.staging")
        upload_assets(repo, tag, release_id, [], plan, staging)
        gh_notice(
            f"Removed {len(plan.stale)} temporary uploads (<file>.next) that a failed run of "
            f"this job left on the published {tag} release. Nothing else of it was changed.",
            title="Temporary uploads removed",
        )
    if not changed:
        return "✅ the published release already has these files"
    if immutable:
        raise ReleaseAssetsError(
            f"The {tag} release is published and immutable: it was published while the "
            "repository's release immutability setting was on, and GitHub accepts no change to "
            f"its files. It {' and '.join(found)}, and force_release cannot {remedy} them. "
            f"{_on_s3(tag)}"
        )
    raise ReleaseAssetsError(
        f"The {tag} release is already published, and a published release is not changed "
        f"without force_release. It {' and '.join(found)}. To {remedy} them: gh workflow run "
        f"publish.yml -f ref={tag} -f force_release=true"
    )


def attach(files: PublishSet, *, dry_run: bool, force: bool) -> tuple[str, str]:
    """Give the tag's release the files of the set; return the result and the release's URL."""
    repo, tag = files.repository, files.ref_name
    wanted = [name for _, name in files.release_files()]
    release = tag_release(repo, tag, wait=not dry_run)
    if release is None:
        if dry_run:
            return "🧪 dry run: the tag has no release yet, and its job would wait for one", ""
        raise ReleaseAssetsError(
            f"The tag {tag} has no GitHub Release, and this job never creates one. The Release "
            "workflow (release.yml) drafts it when the tag is pushed: look at its run, or "
            f"draft the release with `gh workflow run release.yml -f tag={tag}`. Then re-run "
            f"this job, or publish the tag again with `gh workflow run publish.yml -f ref={tag}`."
        )
    release_id, url = int(release["id"]), str(release.get("html_url") or "")
    draft = release.get("draft") is True
    immutable = not draft and release.get("immutable") is True
    assets = gh_api_json(f"repos/{repo}/releases/{release_id}/assets?per_page=100")
    changed = changed_files(files, assets)
    names = [name for _, name in changed]
    # Other people add files to a tag's release: only this set's own temporary names are ours.
    plan = plan_assets(assets, names, owns_release=False, reserved=wanted)
    replaced = sum(old is not None for _, old in plan.replaced)
    print(
        f"release {release_id} ({'draft' if draft else 'published'}): {replaced} replaced, "
        f"{len(changed) - replaced} added, {len(wanted) - len(changed)} unchanged, "
        f"{len(plan.stale)} left by a failed run"
    )
    if not draft and (immutable or not force):
        result = check_published(
            files, release_id, assets, changed, plan, dry_run=dry_run, immutable=immutable
        )
        return result, url
    if dry_run:
        return "🧪 dry run: the release was not changed", url

    def require_tag() -> None:
        if moved := tag_problem(repo, tag, files.commit):
            raise _Stopped(
                f"Nothing more is attached: {moved}",
                "A tag's packages come from the builds of the commit that it names",
            )

    def require_draft() -> None:
        if not draft:
            return
        current = gh_api_json(f"repos/{repo}/releases/{release_id}")
        if current.get("draft") is True:
            return
        if current.get("immutable") is True:
            raise _Stopped(
                f"The {tag} release was published while this job was attaching its files, and "
                "it is immutable: the repository's release immutability setting is on, and "
                "GitHub accepts no further change to its files",
                "The release keeps the files it has: force_release cannot attach the others. "
                f"{_on_s3(tag)}",
                locked=True,
            )
        if force:
            return
        raise _Stopped(
            f"The {tag} release was published while this job was attaching its files, and a "
            "published release is not changed without force_release",
            f"To attach them all the same: gh workflow run publish.yml -f ref={tag} "
            "-f force_release=true",
        )

    staging = files.directory.with_name(f"{files.directory.name}.staging")
    uploaded: dict[str, int] = {}
    try:
        require_tag()
        uploaded = upload_assets(
            repo, tag, release_id, changed, plan, staging, before_change=require_draft
        )
        require_tag()
        swap_assets(repo, plan, uploaded, before_change=require_draft)
    except _Stopped as stopped:
        # Temporary assets would show on a release that is published by now.
        try:
            withdraw = waiting_uploads if stopped.locked else withdraw_uploads
            missing = withdraw(repo, release_id, uploaded) if uploaded else names
            uploads = "this run's temporary uploads were removed"
            if stopped.locked:
                uploads = "their temporary uploads (<file>.next) stay on the release"
            uploads = uploads if uploaded else "nothing was uploaded"
        except subprocess.CalledProcessError as error:
            missing = list(uploaded)
            uploads = (
                "its temporary uploads (<file>.next) could not all be removed and show on "
                f"the release until someone deletes them ({(error.stderr or '').strip()})"
            )
        raise ReleaseAssetsError(
            f"{stopped}. The job stopped before its next change: {len(missing)} of "
            f"{len(wanted)} files were not attached ({', '.join(missing)}), and "
            f"{uploads}. {stopped.remedy}"
        ) from None
    if not changed:
        return "✅ the release already has these files", url
    return f"✅ {'attached to the draft' if draft else 'replaced on the published'} release", url


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--set", dest="directory", type=Path, required=True, metavar="DIR")
    for option, variable in (("--ref", "PUBLISH_REF"), ("--commit", "PUBLISH_COMMIT")):
        value = env_default(variable)
        parser.add_argument(
            option, default=value, required=not value, help=f"(default: ${variable})"
        )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=env_flag("PUBLISH_DRY_RUN"),
        help="Print what would change without changing it (default: $PUBLISH_DRY_RUN)",
    )
    parser.add_argument(
        "--force-release",
        action="store_true",
        default=env_flag("PUBLISH_FORCE_RELEASE"),
        help="Replace the files of a release that is already published "
        "(default: $PUBLISH_FORCE_RELEASE)",
    )
    args = parser.parse_args(argv)

    title = f"GitHub Release {args.ref.rpartition('/')[2]}"
    try:
        files = load_set(args.directory, ref=args.ref, commit=args.commit)
        if not files.is_tag or not is_release_tag(files.ref_name):
            raise ReleaseAssetsError(f"{files.ref} is not a release tag")
        result, url = attach(files, dry_run=args.dry_run, force=args.force_release)
    except (PublishSetError, ReleaseAssetsError, subprocess.CalledProcessError) as exc:
        reason = str(getattr(exc, "stderr", None) or exc).strip()
        gh_error(reason, title="Release assets not attached")
        write_step_summary(summary_section(title, [("Result", "❌ failed"), ("Reason", reason)]))
        return 1
    update_files = [c.name for package in files.packages for c in package.companions]
    write_step_summary(
        summary_section(
            title,
            [
                ("Result", result),
                ("Commit", code_span(files.commit)),
                ("Release", url),
                ("Update files", ", ".join(map(code_span, update_files))),
            ],
            body=set_table(files.packages),
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
