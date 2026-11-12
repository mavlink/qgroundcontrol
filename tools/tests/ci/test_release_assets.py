"""Tests for release_assets.py: a tag's verified packages reach its draft GitHub Release."""

from __future__ import annotations

import hashlib
import json
import re
import subprocess
import time
from pathlib import Path
from typing import TYPE_CHECKING, Any

import common.gh_actions
import pytest
from ci.publish import daily_release
from ci.publish.release_assets import DRAFT_WAIT_ATTEMPTS, DRAFT_WAIT_SECONDS, main

from .._helpers import completed
from ._publish_sets import COMMIT, MASTER, NAMES, REPO, TAG_COMMIT, write_set

if TYPE_CHECKING:
    from collections.abc import Callable

    from ci.publish.publish_set import PublishSet

TAG = "v5.1.5"
TAG_REF = f"refs/tags/{TAG}"
ASSETS = sorted(name for package in NAMES for name in (package, f"{package}.sha256"))
ARM = "QGroundControl-installer-ARM64.exe"
# Upstream's v5.1.5 is an annotated tag: this object names the commit.
TAG_OBJECT = "9428722253158ae7600e45a72f2951c8e60b88b1"


class Releases:
    """Stand-in for the releases API behind ``gh``: the repository's releases and their assets."""

    def __init__(self, monkeypatch: pytest.MonkeyPatch) -> None:
        self.releases: list[dict[str, Any]] = []
        # Assets by id: their release, name, state and digest, as the assets API lists them.
        self.assets: dict[int, dict[str, Any]] = {}
        # What the release tags name: a commit, or the tag object of an annotated tag.
        self.tags: dict[str, str] = {TAG: TAG_COMMIT}
        self.annotated: set[str] = set()
        self.log: list[str] = []
        self.listings = 0
        # Called with the number of listings so far, before the next one is answered.
        self.meanwhile: Callable[[int], object] = lambda _listings: None
        # Called with each change after it is made, and with each path before it is read.
        self.after: Callable[[str], object] = lambda _step: None
        self.before_read: Callable[[str], object] = lambda _path: None
        # Every call of gh, and the one that fails as GitHub does when it is unavailable.
        self.calls = 0
        self.fail_at: int | None = None
        self.refuse_deletes = False
        # The changes that GitHub refused because their release is immutable.
        self.refused: list[str] = []
        self.slept: list[float] = []
        self._next_id = 401155735
        monkeypatch.setattr(common.gh_actions, "gh", self.gh)
        monkeypatch.setattr(daily_release, "gh", self.gh)
        monkeypatch.setattr(time, "sleep", self.slept.append)

    def release(
        self, tag: str = TAG, *, draft: bool = True, immutable: bool = False
    ) -> dict[str, Any]:
        """Create a release for *tag* as release.yml drafts it, or as a maintainer published it.

        *immutable* is what GitHub reports of a release that was published while the
        repository's release immutability setting was on.
        """
        self._next_id += 1
        release = {
            "id": self._next_id,
            "tag_name": tag,
            "draft": draft,
            "immutable": immutable,
            "prerelease": False,
            "name": f"QGroundControl {tag}",
            "body": "## Features\n\n- generated notes",
            "html_url": f"https://github.com/{REPO}/releases/tag/{tag}",
        }
        self.releases.append(release)
        return release

    def add(
        self, release: dict[str, Any], name: str, data: bytes, *, state: str = "uploaded"
    ) -> int:
        assert name not in self.names(release), f"asset {name} exists"
        self._next_id += 1
        uploaded = state == "uploaded"
        self.assets[self._next_id] = {
            "id": self._next_id,
            "release": release["id"],
            "name": name,
            "state": state,
            "size": len(data) if uploaded else 0,
            "digest": f"sha256:{hashlib.sha256(data).hexdigest()}" if uploaded else None,
        }
        return self._next_id

    def hold(self, release: dict[str, Any], files: PublishSet, names: list[str]) -> None:
        """Give *release* the files *names* of the set, as an earlier run uploaded them."""
        for name in names:
            self.add(release, name, (files.directory / name).read_bytes())

    def names(self, release: dict[str, Any]) -> list[str]:
        return sorted(a["name"] for a in self.assets.values() if a["release"] == release["id"])

    def ids(self, release: dict[str, Any]) -> dict[str, int]:
        return {a["name"]: i for i, a in self.assets.items() if a["release"] == release["id"]}

    def _did(self, step: str) -> None:
        self.log.append(step)
        self.after(step)

    def _locked(self, release: dict[str, Any], args: tuple[str, ...]) -> None:
        """Refuse, as GitHub does, a change to the files of an immutable release."""
        if release["immutable"]:
            self.refused.append(" ".join(args[:4]))
            reason = "gh: Cannot modify assets of an immutable release (HTTP 422)"
            raise subprocess.CalledProcessError(1, list(args), "", reason)

    def gh(self, *args: str, **_: object) -> subprocess.CompletedProcess:
        self.calls += 1
        if self.calls == self.fail_at:
            raise subprocess.CalledProcessError(1, list(args), "", "gh: Bad Gateway (HTTP 502)")
        if args[:2] == ("release", "upload"):
            assert args[-2:] == ("--repo", REPO)
            # gh finds a draft by its tag, and one release only.
            (release,) = [r for r in self.releases if r["tag_name"] == args[2]]
            self._locked(release, args)
            for path in args[3:-2]:
                self.add(release, Path(path).name, Path(path).read_bytes())
            self._did(f"upload {len(args[3:-2])}")
            return completed()
        assert args[0] == "api"
        if "--paginate" in args:
            return self._list(args)
        if args[1] != "--method":
            return self._get(args[1].removeprefix(f"repos/{REPO}/"))
        method, path = args[2], args[3].removeprefix(f"repos/{REPO}/")
        # Only assets ever change: never the release, its notes or its state.
        assert path.startswith("releases/assets/"), f"{method} {path}"
        asset = int(path.removeprefix("releases/assets/"))
        if asset not in self.assets:
            raise subprocess.CalledProcessError(1, list(args), "", "gh: Not Found (HTTP 404)")
        owner = next(r for r in self.releases if r["id"] == self.assets[asset]["release"])
        self._locked(owner, args)
        if method == "DELETE":
            assert args[4:] == ()
            if self.refuse_deletes:
                raise subprocess.CalledProcessError(
                    1, list(args), "", "gh: Server Error (HTTP 500)"
                )
            self._did(f"delete {self.assets.pop(asset)['name']}")
        else:
            assert (method, args[4]) == ("PATCH", "-f") and args[6:] == ()
            name = args[5].removeprefix("name=")
            release = next(r for r in self.releases if r["id"] == self.assets[asset]["release"])
            assert name not in self.names(release), f"asset {name} exists"
            self.assets[asset]["name"] = name
            self._did(f"name {name}")
        return completed()

    def _list(self, args: tuple[str, ...]) -> subprocess.CompletedProcess:
        """Answer the listing of releases, which shows drafts to a token that may write."""
        assert f"repos/{REPO}/releases" in args and args[1:3] == ("--method", "GET")
        self.meanwhile(self.listings)
        self.listings += 1
        tag = re.fullmatch(
            r'\.\[\] \| select\(\.tag_name == "([\w.]+)"\)', args[args.index("--jq") + 1]
        )
        assert tag
        found = [release for release in self.releases if release["tag_name"] == tag[1]]
        return completed("".join(f"{json.dumps(release)}\n" for release in found))

    def _get(self, path: str) -> subprocess.CompletedProcess:
        self.before_read(path)
        if match := re.fullmatch(r"releases/(\d+)/assets\?per_page=100", path):
            listed = [
                {key: value for key, value in asset.items() if key != "release"}
                for asset in self.assets.values()
                if asset["release"] == int(match[1])
            ]
            return completed(json.dumps(listed))
        if match := re.fullmatch(r"releases/(\d+)", path):
            found = [release for release in self.releases if release["id"] == int(match[1])]
            if found:
                return completed(json.dumps(found[0]))
        tag = path.removeprefix("git/ref/tags/")
        if tag in self.tags and tag != path:
            kind, sha = ("tag", TAG_OBJECT) if tag in self.annotated else ("commit", self.tags[tag])
            return completed(json.dumps({"object": {"type": kind, "sha": sha}}))
        if path == f"git/tags/{TAG_OBJECT}" and self.annotated:
            (name,) = self.annotated
            return completed(json.dumps({"object": {"type": "commit", "sha": self.tags[name]}}))
        return completed('{"message":"Not Found"}', 1, "gh: Not Found (HTTP 404)")


@pytest.fixture
def releases(monkeypatch: pytest.MonkeyPatch) -> Releases:
    return Releases(monkeypatch)


@pytest.fixture
def files(tmp_path: Path) -> PublishSet:
    return write_set(tmp_path / "set", ref=TAG_REF, commit=TAG_COMMIT)


def _attach(files: PublishSet, *extra: str) -> int:
    argv = ["--set", str(files.directory), "--ref", files.ref, "--commit", files.commit]
    return main([*argv, *extra])


def test_the_verified_packages_are_attached_to_the_tag_s_draft(
    releases: Releases, files: PublishSet, gh_summary: Path
) -> None:
    draft = releases.release()
    before = dict(draft)

    assert _attach(files) == 0

    # Every package and its checksum, under the names they have on S3.
    assert releases.names(draft) == ASSETS
    assert releases.log == ["upload 12", *(f"name {name}" for name in ASSETS)]
    # Still the draft that release.yml wrote: publishing it is the maintainer's step.
    assert draft == before
    summary = gh_summary.read_text(encoding="utf-8")
    assert f"### GitHub Release {TAG}" in summary
    assert "| Result | ✅ attached to the draft release |" in summary
    assert f"https://github.com/{REPO}/releases/tag/{TAG}" in summary
    assert all(f"| `{name}` |" in summary for name in NAMES)


def test_a_published_release_is_not_changed_unless_a_request_by_hand_forces_it(
    releases: Releases,
    files: PublishSet,
    gh_summary: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    # Upstream's v5.1.5: published, with six packages uploaded by hand and no checksums.
    published = releases.release(draft=False)
    for name in NAMES:
        releases.add(published, name, f"{name}, uploaded by hand".encode())
    before, assets = dict(published), dict(releases.ids(published))

    assert _attach(files) == 1

    assert (releases.ids(published), releases.log) == (assets, [])
    out = capsys.readouterr().out
    assert "It lacks 6 of this build's 12 files" in out
    assert "and holds 6 with other content" in out
    assert "| Result | ❌ failed |" in gh_summary.read_text(encoding="utf-8")

    monkeypatch.setenv("PUBLISH_FORCE_RELEASE", "true")
    assert _attach(files) == 0

    assert releases.names(published) == ASSETS
    assert not set(releases.ids(published).values()) & set(assets.values())
    # The old files stay complete until every upload is done.
    assert releases.log[0] == "upload 12"
    assert published == before
    assert "✅ replaced on the published release" in gh_summary.read_text(encoding="utf-8")


def _published_early(releases: Releases) -> dict[str, Any]:
    """A release that its maintainer published while the builds were still running."""
    return releases.release(draft=False)


def test_a_published_release_that_has_every_file_is_left_alone_and_passes(
    releases: Releases, files: PublishSet, gh_summary: Path
) -> None:
    published = _published_early(releases)
    releases.hold(published, files, ASSETS)
    releases.add(published, "QGroundControl-source.tar.gz", b"sources")
    assets = dict(releases.ids(published))

    assert _attach(files) == 0

    assert (releases.ids(published), releases.log) == (assets, [])
    summary = gh_summary.read_text(encoding="utf-8")
    assert "| Result | ✅ the published release already has these files |" in summary


def test_a_release_published_before_its_packages_were_attached_fails_the_job(
    releases: Releases, files: PublishSet, gh_summary: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    # Publish Builds used to pass here, and the README's download buttons answered 404.
    published = _published_early(releases)

    assert _attach(files) == 1

    assert (releases.names(published), releases.log) == ([], [])
    assert (
        f"::error title=Release assets not attached::The {TAG} release is already published, "
        "and a published release is not changed without force_release. It lacks 12 of this "
        f"build's 12 files ({', '.join(name for _, name in files.release_files())}). To attach "
        f"them: gh workflow run publish.yml -f ref={TAG} -f force_release=true"
    ) in capsys.readouterr().out
    assert "| Result | ❌ failed |" in gh_summary.read_text(encoding="utf-8")


def test_a_published_release_with_some_of_the_files_fails_naming_the_others(
    releases: Releases, files: PublishSet, capsys: pytest.CaptureFixture[str]
) -> None:
    # Ten files are in place; the dmg is another build's and its checksum is missing.
    published = _published_early(releases)
    dmg = "QGroundControl.dmg"
    releases.hold(published, files, [name for name in ASSETS if not name.startswith(dmg)])
    releases.add(published, dmg, b"uploaded by hand")
    assets = dict(releases.ids(published))

    assert _attach(files) == 1

    assert (releases.ids(published), releases.log) == (assets, [])
    out = capsys.readouterr().out
    assert f"It lacks 1 of this build's 12 files ({dmg}.sha256)" in out
    assert f"and holds 1 with other content ({dmg})" in out
    assert f"To replace and attach them: gh workflow run publish.yml -f ref={TAG}" in out


@pytest.mark.parametrize("complete", [True, False])
def test_a_failed_run_s_temporary_uploads_are_removed_from_a_published_release(
    releases: Releases, files: PublishSet, capsys: pytest.CaptureFixture[str], complete: bool
) -> None:
    # A run that was stopped by the publication could not withdraw its uploads, which the
    # release's users see. They are this job's own and no file of the release, so they go:
    # the one change that is made to a published release without force_release.
    published = _published_early(releases)
    releases.hold(published, files, ASSETS if complete else ASSETS[:6])
    releases.add(published, f"{ASSETS[6]}.next", b"left by a failed run")
    releases.add(published, f"{ASSETS[7]}.next", b"", state="starter")
    releases.add(published, "release-notes.next", b"a maintainer's file")
    kept = {name: asset for name, asset in releases.ids(published).items() if "next" not in name}

    assert _attach(files) == (0 if complete else 1)

    assert releases.log == [f"delete {ASSETS[6]}.next", f"delete {ASSETS[7]}.next"]
    left = releases.ids(published)
    assert {name: left[name] for name in kept} == kept
    assert sorted(set(left) - set(kept)) == ["release-notes.next"]
    out = capsys.readouterr().out
    assert ("It lacks 6 of this build's 12 files" in out) is not complete
    assert (
        "::notice title=Temporary uploads removed::Removed 2 temporary uploads (<file>.next) "
        f"that a failed run of this job left on the published {TAG} release. Nothing else of "
        "it was changed."
    ) in out


def test_a_dry_run_reports_a_published_release_that_lacks_files_without_touching_it(
    releases: Releases, files: PublishSet, gh_summary: Path
) -> None:
    published = _published_early(releases)
    releases.add(published, f"{ASSETS[0]}.next", b"left by a failed run")
    assets = dict(releases.ids(published))

    assert _attach(files, "--dry-run") == 0

    assert (releases.ids(published), releases.log) == (assets, [])
    summary = gh_summary.read_text(encoding="utf-8")
    assert "| Result | 🧪 dry run: the published release lacks 12 of this build's 12 " in summary
    assert "), and its job would fail without force_release |" in summary


IMMUTABLE = (
    f"The {TAG} release is published and immutable: it was published while the repository's "
    "release immutability setting was on, and GitHub accepts no change to its files."
)
ON_S3 = (
    f"The download server has this build's files under builds/{TAG}/ when the S3 builds/ job of "
    "this run passed."
)


def test_an_immutable_release_that_has_every_file_passes(
    releases: Releases, files: PublishSet, gh_summary: Path
) -> None:
    # Published with its files attached to the draft, as the release checklist says.
    published = releases.release(draft=False, immutable=True)
    releases.hold(published, files, ASSETS)

    assert _attach(files) == 0

    assert (releases.log, releases.refused) == ([], [])
    summary = gh_summary.read_text(encoding="utf-8")
    assert "| Result | ✅ the published release already has these files |" in summary


@pytest.mark.parametrize("extra", [(), ("--force-release",)])
def test_an_immutable_release_that_lacks_files_fails_and_nothing_can_attach_them(
    releases: Releases,
    files: PublishSet,
    capsys: pytest.CaptureFixture[str],
    extra: tuple[str, ...],
) -> None:
    # Upstream's v5.1.0 to v5.1.5: published while the setting was on, the packages uploaded
    # by hand before that. force_release would upload and be refused by GitHub.
    published = releases.release(draft=False, immutable=True)
    for name in NAMES:
        releases.add(published, name, f"{name}, uploaded by hand".encode())
    assets = dict(releases.ids(published))

    assert _attach(files, *extra) == 1

    assert (releases.ids(published), releases.log, releases.refused) == (assets, [], [])
    out = capsys.readouterr().out
    assert (
        f"::error title=Release assets not attached::{IMMUTABLE} It lacks 6 of this build's 12 "
        f"files ({', '.join(f'{name}.sha256' for name in NAMES)}) and holds 6 with other content "
        f"({', '.join(NAMES)}), and force_release cannot replace and attach them. {ON_S3}"
    ) in out
    assert "force_release=true" not in out
    assert "--clobber" not in out


@pytest.mark.parametrize("extra", [(), ("--force-release",)])
def test_a_dry_run_says_that_an_immutable_release_cannot_be_completed(
    releases: Releases, files: PublishSet, gh_summary: Path, extra: tuple[str, ...]
) -> None:
    published = releases.release(draft=False, immutable=True)
    releases.hold(published, files, ASSETS[:10])

    assert _attach(files, "--dry-run", *extra) == 0

    assert (releases.log, releases.refused) == ([], [])
    assert (
        f"| Result | 🧪 dry run: the published release lacks 2 of this build's 12 files "
        f"({', '.join(ASSETS[10:])}), and its job would fail: the release is immutable, so "
        "force_release cannot attach them |"
    ) in gh_summary.read_text(encoding="utf-8")


def test_a_failed_run_s_temporary_uploads_stay_on_an_immutable_release(
    releases: Releases, files: PublishSet, capsys: pytest.CaptureFixture[str]
) -> None:
    # The draft was published while a run was uploading: GitHub would refuse the removal.
    published = releases.release(draft=False, immutable=True)
    releases.hold(published, files, ASSETS)
    releases.add(published, f"{ASSETS[6]}.next", b"left by a failed run")

    assert _attach(files) == 0

    assert (releases.log, releases.refused) == ([], [])
    assert (
        "::warning title=Temporary uploads not removed::1 temporary uploads (<file>.next) that "
        f"a failed run of this job left stay on the published {TAG} release: it is immutable, "
        "and GitHub accepts no change to its files."
    ) in capsys.readouterr().out


@pytest.mark.parametrize("extra", [(), ("--force-release",)])
@pytest.mark.parametrize(
    ("published_after", "attached"),
    [("upload 12", []), (f"name {ASSETS[0]}", [ASSETS[0]])],
)
def test_a_release_that_becomes_immutable_while_the_job_runs_keeps_what_it_has(
    releases: Releases,
    files: PublishSet,
    capsys: pytest.CaptureFixture[str],
    extra: tuple[str, ...],
    published_after: str,
    attached: list[str],
) -> None:
    draft = releases.release()
    _publish_after(releases, draft, published_after, immutable=True)

    assert _attach(files, *extra) == 1

    # No change is tried after the publication: GitHub would refuse each, the removal of the
    # run's own temporary uploads included.
    assert releases.log[-1] == published_after
    assert releases.refused == []
    waiting = [name for name in ASSETS if name not in attached]
    assert releases.names(draft) == sorted([*attached, *(f"{name}.next" for name in waiting)])
    out = capsys.readouterr().out
    assert (
        f"::error title=Release assets not attached::The {TAG} release was published while this "
        "job was attaching its files, and it is immutable: the repository's release immutability "
        "setting is on, and GitHub accepts no further change to its files. The job stopped "
        f"before its next change: {len(waiting)} of 12 files were not attached "
        f"({', '.join(waiting)}), and their temporary uploads (<file>.next) stay on the release. "
        f"The release keeps the files it has: force_release cannot attach the others. {ON_S3}"
    ) in out
    assert "force_release=true" not in out


def test_a_tag_without_a_release_is_waited_for_and_then_fails_naming_release_yml(
    releases: Releases, files: PublishSet, capsys: pytest.CaptureFixture[str]
) -> None:
    # release.yml failed, or was never started for this tag. Its release is not created here.
    releases.release("v5.1.4")

    assert _attach(files) == 1

    assert releases.listings == DRAFT_WAIT_ATTEMPTS
    assert releases.slept == [DRAFT_WAIT_SECONDS] * (DRAFT_WAIT_ATTEMPTS - 1)
    assert [release["tag_name"] for release in releases.releases] == ["v5.1.4"]
    assert releases.log == []
    assert (
        f"::error title=Release assets not attached::The tag {TAG} has no GitHub Release, and "
        "this job never creates one. The Release workflow (release.yml) drafts it when the tag "
        f"is pushed: look at its run, or draft the release with `gh workflow run release.yml -f "
        f"tag={TAG}`. Then re-run this job, or publish the tag again with `gh workflow run "
        f"publish.yml -f ref={TAG}`."
    ) in capsys.readouterr().out


def test_a_draft_that_appears_while_the_job_waits_gets_the_packages(
    releases: Releases, files: PublishSet
) -> None:
    # release.yml is still running when the builds complete.
    releases.meanwhile = lambda listings: releases.release() if listings == 2 else None

    assert _attach(files) == 0

    assert releases.listings == 3
    assert releases.names(releases.releases[0]) == ASSETS


def test_a_partial_previous_upload_is_completed(releases: Releases, files: PublishSet) -> None:
    draft = releases.release()
    done = [NAMES[0], f"{NAMES[0]}.sha256"]
    # An earlier run uploaded one package and stopped: one upload finished under its temporary
    # name, the next did not finish. A maintainer's own upload of the dmg was cut off too, and
    # the release has a file that is no package.
    releases.hold(draft, files, done)
    releases.add(draft, f"{NAMES[1]}.next", (files.directory / NAMES[1]).read_bytes())
    releases.add(draft, f"{NAMES[2]}.next", b"", state="starter")
    releases.add(draft, "QGroundControl.dmg", b"", state="starter")
    releases.add(draft, "QGroundControl-source.tar.gz", b"sources")
    kept = {name: releases.ids(draft)[name] for name in [*done, "QGroundControl-source.tar.gz"]}

    assert _attach(files) == 0

    assert releases.names(draft) == sorted([*ASSETS, "QGroundControl-source.tar.gz"])
    # What was complete is not uploaded again, and the unrelated file is left alone.
    assert {name: releases.ids(draft)[name] for name in kept} == kept
    assert releases.log[:3] == [f"delete {NAMES[1]}.next", f"delete {NAMES[2]}.next", "upload 10"]
    assert releases.log.index("delete QGroundControl.dmg") > 2
    uploaded = {asset["name"]: asset for asset in releases.assets.values()}
    assert all(uploaded[name]["state"] == "uploaded" for name in ASSETS)


def test_a_release_of_an_older_line_gets_the_packages_that_its_branch_builds(
    releases: Releases, tmp_path: Path
) -> None:
    # A Stable branch cut before the Arm installer existed: its set has five packages.
    files = write_set(tmp_path / "set", ref=TAG_REF, commit=TAG_COMMIT, without=[ARM])
    draft = releases.release()

    assert _attach(files) == 0

    assert releases.names(draft) == [name for name in ASSETS if not name.startswith(ARM)]
    assert releases.log[0] == "upload 10"


def test_an_appimage_s_update_file_goes_to_the_release_beside_its_package(
    releases: Releases, tmp_path: Path, gh_summary: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    # Upstream's AppImages name the latest release as where an updater finds `<name>.zsync`.
    files = write_set(tmp_path / "set", ref=TAG_REF, commit=TAG_COMMIT, update_files=True)
    update_files = [f"{name}.zsync" for name in NAMES if name.endswith(".AppImage")]
    draft = releases.release()

    assert _attach(files) == 0

    assert releases.names(draft) == sorted([*ASSETS, *update_files])
    assert releases.log[0] == "upload 14"
    assert f"| Update files | `{update_files[0]}`, `{update_files[1]}` |" in (
        gh_summary.read_text(encoding="utf-8")
    )
    held = {asset["name"]: asset["digest"] for asset in releases.assets.values()}
    companion = files.packages[0].companions[0]
    assert held[companion.name] == f"sha256:{companion.sha256}"

    # Nothing to do the second time, and a published release is complete only with them.
    steps = len(releases.log)
    assert _attach(files) == 0
    assert len(releases.log) == steps
    draft["draft"] = False
    del releases.assets[releases.ids(draft)[update_files[1]]]
    assert _attach(files) == 1
    assert f"It lacks 1 of this build's 14 files ({update_files[1]}). To attach them:" in (
        capsys.readouterr().out
    )


def test_a_dry_run_reads_the_release_and_changes_nothing(
    releases: Releases, files: PublishSet, gh_summary: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    # No release yet: a dry run does not wait for one.
    assert _attach(files, "--dry-run") == 0
    assert (releases.listings, releases.slept) == (1, [])
    assert "the tag has no release yet" in gh_summary.read_text(encoding="utf-8")

    draft = releases.release()
    releases.hold(draft, files, [NAMES[0]])
    releases.add(draft, NAMES[1], b"another build")
    assets = dict(releases.ids(draft))

    assert _attach(files, "--dry-run") == 0

    assert (releases.ids(draft), releases.log) == (assets, [])
    assert "1 replaced, 10 added, 1 unchanged" in capsys.readouterr().out
    assert "dry run: the release was not changed" in gh_summary.read_text(encoding="utf-8")


def test_a_second_run_for_the_same_packages_changes_nothing(
    releases: Releases, files: PublishSet, gh_summary: Path
) -> None:
    draft = releases.release()
    assert _attach(files) == 0
    assets, steps = dict(releases.ids(draft)), len(releases.log)

    # Another build of the tag completed, or the job was run again.
    assert _attach(files) == 0

    assert releases.ids(draft) == assets
    assert len(releases.log) == steps
    assert "✅ the release already has these files" in gh_summary.read_text(encoding="utf-8")


def test_packages_that_were_built_again_replace_the_draft_s(
    releases: Releases, files: PublishSet, tmp_path: Path
) -> None:
    draft = releases.release()
    assert _attach(files) == 0
    releases.log.clear()
    # The same tag and names, other bytes: the tag was moved and its builds ran again.
    rebuilt = write_set(tmp_path / "rebuilt", ref=TAG_REF, commit=COMMIT)
    releases.tags[TAG] = COMMIT

    assert _attach(rebuilt) == 0

    swap = [step for name in ASSETS for step in (f"delete {name}", f"name {name}")]
    assert releases.log == ["upload 12", *swap]
    assert releases.names(draft) == ASSETS
    package = next(a for a in releases.assets.values() if a["name"] == NAMES[0])
    assert package["digest"] == f"sha256:{rebuilt.packages[0].sha256}"


def test_two_releases_for_one_tag_stop_the_job(
    releases: Releases, files: PublishSet, capsys: pytest.CaptureFixture[str]
) -> None:
    first, second = releases.release(), releases.release(draft=False)

    assert _attach(files, "--force-release") == 1

    assert releases.log == []
    assert f"2 releases have the tag {TAG} (ids {first['id']}, {second['id']})" in (
        capsys.readouterr().out
    )


def test_only_a_release_tag_s_packages_are_attached(
    releases: Releases, tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    releases.release("master")
    files = write_set(tmp_path / "set", ref=MASTER, commit=COMMIT)

    assert _attach(files) == 1

    assert (releases.listings, releases.log) == (0, [])
    assert f"{MASTER} is not a release tag" in capsys.readouterr().out


def _publish_after(
    releases: Releases, release: dict[str, Any], step: str, *, immutable: bool = False
) -> None:
    """Have a maintainer publish *release* right after the job made the change *step*."""
    published = {"draft": False, "immutable": immutable}
    releases.after = lambda done: release.update(published) if done == step else None


@pytest.mark.parametrize(
    ("published_after", "attached"),
    [
        # While the files were uploaded: none had its name yet.
        ("upload 12", []),
        # During the swap, when the first file was in place.
        (f"name {ASSETS[0]}", [ASSETS[0]]),
        (f"name {ASSETS[10]}", ASSETS[:11]),
    ],
)
def test_a_release_that_is_published_while_the_job_runs_is_changed_no_further(
    releases: Releases,
    files: PublishSet,
    capsys: pytest.CaptureFixture[str],
    published_after: str,
    attached: list[str],
) -> None:
    draft = releases.release()
    _publish_after(releases, draft, published_after)

    assert _attach(files) == 1

    # What was in place before the publication stays, and nothing temporary is left for the
    # release's users to see: the job deleted its own uploads and nothing else.
    assert releases.names(draft) == attached
    later = releases.log[releases.log.index(published_after) + 1 :]
    assert later == [f"delete {name}.next" for name in ASSETS if name not in attached]
    missing = 12 - len(attached)
    assert (
        f"::error title=Release assets not attached::The {TAG} release was published while this "
        "job was attaching its files, and a published release is not changed without "
        f"force_release. The job stopped before its next change: {missing} of 12 files were not "
        "attached"
    ) in (out := capsys.readouterr().out)
    assert "this run's temporary uploads were removed" in out
    assert f"gh workflow run publish.yml -f ref={TAG} -f force_release=true" in out


def test_temporary_uploads_that_cannot_be_withdrawn_are_named_for_someone_to_delete(
    releases: Releases, files: PublishSet, capsys: pytest.CaptureFixture[str]
) -> None:
    def publish(step: str) -> None:
        if step == "upload 12":
            draft["draft"] = False
            releases.refuse_deletes = True

    draft = releases.release()
    releases.after = publish

    assert _attach(files) == 1

    assert releases.names(draft) == [f"{name}.next" for name in ASSETS]
    out = capsys.readouterr().out
    assert f"The {TAG} release was published while this job was attaching its files" in out
    assert "its temporary uploads (<file>.next) could not all be removed and show on the" in out


@pytest.mark.parametrize("left_by_a_failed_run", [True, False])
def test_a_release_published_before_the_first_change_is_not_touched(
    releases: Releases,
    files: PublishSet,
    capsys: pytest.CaptureFixture[str],
    left_by_a_failed_run: bool,
) -> None:
    # The draft is published after the job found it, while it was reading what the draft holds.
    # The first change would be the removal of an earlier run's upload, or else the upload.
    draft = releases.release()
    if left_by_a_failed_run:
        releases.add(draft, f"{NAMES[0]}.next", b"left by a failed run")
    releases.before_read = lambda path: draft.update(draft=False) if "assets" in path else None
    assets = dict(releases.ids(draft))

    assert _attach(files) == 1

    assert (releases.ids(draft), releases.log) == (assets, [])
    assert "12 of 12 files were not attached" in capsys.readouterr().out


def test_a_forced_run_goes_on_when_the_release_is_published_meanwhile(
    releases: Releases, files: PublishSet
) -> None:
    draft = releases.release()
    _publish_after(releases, draft, "upload 12")

    assert _attach(files, "--force-release") == 0

    assert releases.names(draft) == ASSETS


@pytest.mark.parametrize(
    ("now", "problem"),
    [
        (COMMIT, f"the tag {TAG} now names {COMMIT}, not {TAG_COMMIT}"),
        (None, f"the tag {TAG} no longer exists"),
    ],
)
@pytest.mark.parametrize("moved_after", [None, "upload 12"])
def test_the_packages_of_a_commit_that_the_tag_no_longer_names_are_not_attached(
    releases: Releases,
    files: PublishSet,
    capsys: pytest.CaptureFixture[str],
    now: str | None,
    problem: str,
    moved_after: str | None,
) -> None:
    def move(step: str | None) -> None:
        if step == moved_after:
            releases.tags.pop(TAG) if now is None else releases.tags.update({TAG: now})

    # The tag is moved or deleted while the job waits its turn, or while it uploads.
    draft = releases.release()
    releases.after = move
    move(None)

    assert _attach(files) == 1

    assert releases.names(draft) == []
    if moved_after is None:
        assert releases.log == []
    else:
        assert releases.log[1:] == [f"delete {name}.next" for name in ASSETS]
    assert f"::error title=Release assets not attached::Nothing more is attached: {problem}" in (
        capsys.readouterr().out
    )


def test_an_annotated_tag_is_followed_to_its_commit(releases: Releases, files: PublishSet) -> None:
    draft = releases.release()
    releases.annotated.add(TAG)

    assert _attach(files) == 0

    assert releases.names(draft) == ASSETS


@pytest.mark.parametrize("complete", [True, False])
def test_only_the_temporary_names_of_its_own_files_are_ever_cleaned_up(
    releases: Releases, files: PublishSet, complete: bool
) -> None:
    draft = releases.release()
    others = ["release-notes.next", "QGroundControl-source.tar.gz.next", f"{NAMES[0]}.old.next"]
    for name in others:
        releases.add(draft, name, b"a maintainer's file")
    if complete:
        releases.hold(draft, files, ASSETS)
    # A failed run left these two, for a package and for a checksum.
    releases.add(draft, f"{NAMES[0]}.next", b"half an upload")
    releases.add(draft, f"{NAMES[5]}.sha256.next", b"", state="starter")
    kept = {name: releases.ids(draft)[name] for name in others}

    assert _attach(files) == 0

    assert releases.names(draft) == sorted([*ASSETS, *others])
    assert {name: releases.ids(draft)[name] for name in others} == kept
    assert releases.log[:2] == [f"delete {NAMES[0]}.next", f"delete {NAMES[5]}.sha256.next"]
    assert not any(step.split()[1] in others for step in releases.log)
    assert ("upload 12" in releases.log) is not complete


def test_a_run_that_fails_at_any_call_is_repaired_by_the_next(
    monkeypatch: pytest.MonkeyPatch, files: PublishSet
) -> None:
    def scenario() -> tuple[Releases, dict[str, Any]]:
        # A draft that holds other builds of four files, what a failed run left of a fifth,
        # and two files that are not this workflow's.
        releases = Releases(monkeypatch)
        draft = releases.release()
        for name in ASSETS[:4]:
            releases.add(draft, name, b"an earlier build")
        releases.add(draft, f"{ASSETS[4]}.next", b"", state="starter")
        releases.add(draft, "QGroundControl-source.tar.gz", b"sources")
        releases.add(draft, "release-notes.next", b"a maintainer's file")
        return releases, draft

    def held(releases: Releases, draft: dict[str, Any]) -> dict[str, str | None]:
        mine = (asset for asset in releases.assets.values() if asset["release"] == draft["id"])
        return {asset["name"]: asset["digest"] for asset in mine}

    releases, draft = scenario()
    assert _attach(files) == 0
    expected, calls = held(releases, draft), releases.calls
    assert sorted(expected) == sorted(
        [*ASSETS, "QGroundControl-source.tar.gz", "release-notes.next"]
    )

    for call in range(1, calls + 1):
        releases, draft = scenario()
        releases.fail_at = call
        # A change that failed is tried again, so the run may pass all the same.
        failed = _attach(files) == 1
        # The files of others are untouched whatever happened.
        assert {"QGroundControl-source.tar.gz", "release-notes.next"} <= set(held(releases, draft))
        assert failed or held(releases, draft) == expected, call

        assert _attach(files) == 0, call
        assert held(releases, draft) == expected, call
