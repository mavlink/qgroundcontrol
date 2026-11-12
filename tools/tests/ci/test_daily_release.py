"""Tests for daily_release.py: one rolling prerelease that only moves forward."""

from __future__ import annotations

import json
import re
import subprocess
from datetime import UTC, datetime
from typing import TYPE_CHECKING, Any

import common.gh_actions
import pytest
from ci.publish import daily_release
from ci.publish.daily_release import (
    DailyReleaseError,
    main,
    plan_assets,
    published_commit,
    recorded_commit,
    release_notes,
    tag_step,
)

from .._helpers import completed
from ._publish_sets import COMMIT, MASTER, NAMES, OLDER_COMMIT, REPO, RUN_IDS, write_set

if TYPE_CHECKING:
    from pathlib import Path

    from ci.publish.publish_set import PublishSet

ASSETS = [name for package in NAMES for name in (package, f"{package}.sha256")]
# A master commit between OLDER_COMMIT and COMMIT.
MIDDLE_COMMIT = "77ed40dbfb99ead1b61e85a3f34a6df018946d32"
RUN_URL = "https://github.com/mavlink/qgroundcontrol/actions/runs/43100000000"
NOW = datetime(2026, 11, 11, 16, 40, tzinfo=UTC)


class GitHub:
    """Stand-in for the releases, refs and compare APIs behind ``gh``, holding one release."""

    def __init__(self, monkeypatch: pytest.MonkeyPatch) -> None:
        self.tag: str | None = None
        self.release: dict[str, Any] | None = None
        self.assets: dict[int, str] = {}
        # How the second commit relates to the first; master only moves forward.
        self.relations = {
            (OLDER_COMMIT, COMMIT): "ahead",
            (COMMIT, OLDER_COMMIT): "behind",
            (OLDER_COMMIT, MIDDLE_COMMIT): "ahead",
            (COMMIT, MIDDLE_COMMIT): "behind",
        }
        # GitHub's answer to every request that creates or moves the tag, when it is not success.
        self.tag_error: str | None = None
        self.tag_requests = 0
        self.log: list[str] = []
        self._next_id = 500
        monkeypatch.setattr(common.gh_actions, "gh", self.gh)
        monkeypatch.setattr(daily_release, "gh", self.gh)
        monkeypatch.setattr(daily_release.time, "sleep", lambda _seconds: None)

    def publish(self, commit: str, names: list[str], *, notes: bool = True) -> None:
        """Start from a release that holds *commit*'s files."""
        self.tag = commit
        body = f"<!-- daily-build-commit: {commit} -->" if notes else "pending"
        self.release = {"id": 300, "body": body, "prerelease": True, "name": "Daily"}
        for name in names:
            self.add(name)

    def add(self, name: str) -> int:
        assert name not in self.assets.values(), f"asset {name} exists"
        self._next_id += 1
        self.assets[self._next_id] = name
        return self._next_id

    @property
    def names(self) -> list[str]:
        return sorted(self.assets.values())

    def gh(self, *args: str, **_: object) -> subprocess.CompletedProcess:
        if args[:2] == ("release", "upload"):
            assert args[2] == "daily" and args[-2:] == ("--repo", REPO)
            for path in args[3:-2]:
                self.add(path.rsplit("/", 1)[1])
            self.log.append(f"upload {len(args[3:-2])}")
            return completed()
        assert args[0] == "api"
        if args[1] != "--method":
            return self._get(args[1])
        method, path = args[2], args[3].removeprefix(f"repos/{REPO}/")
        fields = dict(field.split("=", 1) for field in args[5::2])
        typed = {
            field.split("=", 1)[0]
            for flag, field in zip(args[4::2], args[5::2], strict=True)
            if flag == "-F"
        }
        try:
            self._change(method, path, fields, typed)
        except LookupError as error:
            raise subprocess.CalledProcessError(1, list(args), "", f"gh: {error}") from error
        return completed()

    def _get(self, path: str) -> subprocess.CompletedProcess:
        path = path.removeprefix(f"repos/{REPO}/")
        if path == "releases/tags/daily" and self.release:
            assets = [{"id": i, "name": name} for i, name in self.assets.items()]
            return completed(json.dumps({**self.release, "assets": assets}))
        if path == "git/ref/tags/daily" and self.tag:
            return completed(json.dumps({"object": {"type": "commit", "sha": self.tag}}))
        if match := re.fullmatch(r"compare/(\w+)\.\.\.(\w+)", path):
            return completed(self.relations[match[1], match[2]])
        if path == "releases/300/assets?per_page=100":
            return completed(json.dumps([{"id": i, "name": n} for i, n in self.assets.items()]))
        if path == f"commits/{COMMIT}":
            return completed(
                json.dumps({"commit": {"committer": {"date": "2026-11-11T06:05:20Z"}}})
            )
        return completed('{"message":"Not Found"}', 1, "gh: Not Found (HTTP 404)")

    def _change(self, method: str, path: str, fields: dict[str, str], typed: set[str]) -> None:
        if path in ("git/refs", "git/refs/tags/daily"):
            self.tag_requests += 1
            if self.tag_error:
                raise LookupError(self.tag_error)
        if (method, path) == ("POST", "git/refs"):
            assert fields["ref"] == "refs/tags/daily" and self.tag is None
            self.tag = fields["sha"]
            self.log.append("create tag")
        elif (method, path) == ("POST", "releases"):
            assert fields["tag_name"] == "daily" and self.release is None and self.tag
            self.release = {"id": 300, **fields}
            self.release["prerelease"] = fields["prerelease"] == "true" and "prerelease" in typed
            self.log.append("create release")
        elif (method, path) == ("PATCH", "git/refs/tags/daily"):
            # GitHub's rule for an unforced update, and the tag ruleset's for any update.
            forced = fields["force"] != "false" or "force" not in typed
            ahead = self.relations.get((self.tag or "", fields["sha"])) == "ahead"
            if forced or not ahead:
                raise LookupError("Update is not a fast forward (HTTP 422)")
            self.tag = fields["sha"]
            self.log.append("move tag")
        elif method == "DELETE":
            asset = int(path.removeprefix("releases/assets/"))
            if asset not in self.assets:
                raise LookupError("Not Found (HTTP 404)")
            self.log.append(f"delete {self.assets.pop(asset)}")
        elif path == "releases/300":
            assert method == "PATCH" and self.release
            self.release.update(fields)
            self.release["prerelease"] = fields["prerelease"] == "true" and "prerelease" in typed
            self.log.append("write notes")
        else:
            asset = int(path.removeprefix("releases/assets/"))
            assert method == "PATCH" and fields["name"] not in self.assets.values()
            self.assets[asset] = fields["name"]
            self.log.append(f"name {fields['name']}")


@pytest.fixture
def github(monkeypatch: pytest.MonkeyPatch) -> GitHub:
    return GitHub(monkeypatch)


def _update(files: PublishSet, *extra: str) -> int:
    argv = ["--set", str(files.directory), "--ref", files.ref, "--commit", files.commit]
    return main([*argv, "--run-url", RUN_URL, *extra])


def test_the_first_publish_creates_the_tag_and_a_prerelease_that_is_never_latest(
    github: GitHub, tmp_path: Path, gh_summary: Path
) -> None:
    files = write_set(tmp_path / "set")

    assert _update(files) == 0

    assert github.tag == COMMIT
    assert github.release is not None
    assert github.release["prerelease"] is True
    assert github.release["make_latest"] == "false"
    assert github.names == sorted(ASSETS)
    assert github.log[:3] == ["create tag", "create release", "upload 12"]
    assert github.log[-1] == "write notes"
    assert published_commit(REPO) == COMMIT
    summary = gh_summary.read_text(encoding="utf-8")
    assert "✅ published" in summary
    assert f"https://github.com/{REPO}/releases/tag/daily" in summary


def test_an_update_swaps_the_assets_only_after_every_upload_and_the_tag_move(
    github: GitHub, tmp_path: Path
) -> None:
    github.publish(OLDER_COMMIT, ASSETS)
    old_ids = set(github.assets)
    files = write_set(tmp_path / "set")

    assert _update(files) == 0

    # The old files stay complete during the uploads; the swap is deletes and renames only.
    swap = [step for name in ASSETS for step in (f"delete {name}", f"name {name}")]
    assert github.log == ["upload 12", "move tag", *swap, "write notes"]
    assert github.tag == COMMIT
    assert github.names == sorted(ASSETS)
    assert not old_ids & set(github.assets)
    assert github.release is not None
    assert github.release["id"] == 300
    assert github.release["prerelease"] is True
    assert published_commit(REPO) == COMMIT


def test_an_older_commit_never_takes_the_release_back(
    github: GitHub, tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    github.publish(COMMIT, ASSETS)
    files = write_set(tmp_path / "set", commit=OLDER_COMMIT)

    assert _update(files) == 1

    assert github.log == []
    assert github.tag == COMMIT
    assert published_commit(REPO) == COMMIT
    assert "the tag only moves forward" in capsys.readouterr().out


@pytest.mark.parametrize(
    "refusal",
    [
        "Repository rule violations found (HTTP 422)",
        "Resource not accessible by integration (HTTP 403)",
    ],
)
def test_a_refused_tag_move_is_a_warning_and_the_release_is_updated_all_the_same(
    github: GitHub,
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
    gh_summary: Path,
    refusal: str,
) -> None:
    # A refusal that repeated on every publish would otherwise stop the daily release for good.
    github.publish(OLDER_COMMIT, ASSETS)
    old_ids = set(github.assets)
    github.tag_error = refusal
    files = write_set(tmp_path / "set")

    assert _update(files) == 0

    assert github.tag == OLDER_COMMIT
    assert github.names == sorted(ASSETS)
    assert not old_ids & set(github.assets)
    # The record follows the files, not the tag: it is what keeps an older commit out.
    assert published_commit(REPO) == COMMIT
    assert github.release is not None
    assert (
        f"The `daily` tag still points at [`{OLDER_COMMIT[:10]}`]"
        f"(https://github.com/{REPO}/commit/{OLDER_COMMIT})"
    ) in github.release["body"]
    assert f"[`{COMMIT[:10]}`](https://github.com/{REPO}/commit/{COMMIT})" in github.release["body"]
    out = capsys.readouterr().out
    assert "::warning title=Daily tag not moved::GitHub refused to move the daily tag" in out
    assert refusal in out
    summary = gh_summary.read_text(encoding="utf-8")
    assert (
        f"| Result | ⚠️ published, but the `daily` tag still points at `{OLDER_COMMIT}`" in summary
    )

    # The next publish tries the move again.
    github.tag_error = None
    assert _update(files) == 0
    assert github.tag == COMMIT
    assert "still points at" not in github.release["body"]


@pytest.mark.parametrize(
    "failure", ["Server Error (HTTP 502)", "connection reset by peer", "Not Found (HTTP 404)"]
)
def test_an_unexpected_failure_to_move_the_tag_still_fails_and_keeps_the_previous_files(
    github: GitHub,
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
    gh_summary: Path,
    failure: str,
) -> None:
    github.publish(OLDER_COMMIT, ASSETS)
    before = dict(github.assets)
    github.tag_error = failure
    files = write_set(tmp_path / "set")

    assert _update(files) == 1

    assert github.tag_requests == 3
    assert github.tag == OLDER_COMMIT
    assert {i: name for i, name in github.assets.items() if i in before} == before
    assert published_commit(REPO) == OLDER_COMMIT
    out = capsys.readouterr().out
    assert "::error title=Daily release not updated::the daily tag could not be moved" in out
    assert "❌ failed" in gh_summary.read_text(encoding="utf-8")


def test_the_first_publish_fails_when_the_tag_cannot_be_created(
    github: GitHub, tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    github.tag_error = "Repository rule violations found (HTTP 422)"

    assert _update(write_set(tmp_path / "set")) == 1

    assert (github.tag, github.release, github.log) == (None, None, [])
    assert "::error title=Daily release not updated::" in capsys.readouterr().out


def test_an_older_commit_does_not_take_the_release_back_while_the_tag_lags_behind_it(
    github: GitHub, tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    # The tag could not follow the release to COMMIT. MIDDLE_COMMIT is ahead of the tag, but
    # behind the files.
    github.publish(COMMIT, ASSETS)
    github.tag = OLDER_COMMIT
    files = write_set(tmp_path / "set", commit=MIDDLE_COMMIT)

    assert _update(files) == 1

    assert github.log == []
    assert published_commit(REPO) == COMMIT
    assert f"the release holds {COMMIT}" in capsys.readouterr().out


def test_a_rerun_finishes_what_a_failed_run_left(github: GitHub, tmp_path: Path) -> None:
    # The failed run uploaded some files and moved the tag, then stopped: no notes for COMMIT.
    github.publish(OLDER_COMMIT, ASSETS)
    github.tag = COMMIT
    github.add(f"{NAMES[0]}.next")
    github.add(f"{NAMES[0]}.sha256.next")
    files = write_set(tmp_path / "set")
    assert published_commit(REPO) == OLDER_COMMIT

    assert _update(files) == 0

    assert github.log[:3] == [
        f"delete {NAMES[0]}.next",
        f"delete {NAMES[0]}.sha256.next",
        "upload 12",
    ]
    assert "move tag" not in github.log
    assert github.names == sorted(ASSETS)
    assert published_commit(REPO) == COMMIT


def test_files_of_an_older_commit_do_not_stay_in_the_release(
    github: GitHub, tmp_path: Path
) -> None:
    github.publish(OLDER_COMMIT, [*ASSETS, "QGroundControl-installer-AMD64-ARM64.exe"])
    files = write_set(tmp_path / "set")

    assert _update(files) == 0

    assert github.names == sorted(ASSETS)
    assert github.log[-2:] == ["delete QGroundControl-installer-AMD64-ARM64.exe", "write notes"]


def test_the_release_holds_exactly_the_set_whatever_packages_it_has(
    github: GitHub, tmp_path: Path
) -> None:
    # master stopped building a package: its asset goes with the commit that had it.
    dropped = NAMES[0]
    github.publish(OLDER_COMMIT, ASSETS)
    files = write_set(tmp_path / "set", without=[dropped])

    assert _update(files) == 0

    assert github.names == sorted(
        name for name in ASSETS if name not in (dropped, f"{dropped}.sha256")
    )
    assert github.release is not None
    assert f"`{dropped}`" not in github.release["body"]
    assert all(f"`{name}`" in github.release["body"] for name in NAMES[1:])


def test_a_dry_run_changes_nothing(
    github: GitHub, tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_summary: Path
) -> None:
    github.publish(OLDER_COMMIT, ASSETS)
    files = write_set(tmp_path / "set")

    assert _update(files, "--dry-run") == 0
    monkeypatch.setenv("PUBLISH_DRY_RUN", "true")
    assert _update(files) == 0

    assert github.log == []
    assert github.tag == OLDER_COMMIT
    assert "dry run: the release was not changed" in gh_summary.read_text(encoding="utf-8")


def test_only_master_feeds_the_daily_release(
    github: GitHub, tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    files = write_set(tmp_path / "set", ref="refs/heads/Stable_V5.1")

    assert _update(files) == 1

    assert github.log == []
    assert "the daily release follows refs/heads/master" in capsys.readouterr().out


def test_the_notes_state_the_commit_its_date_the_builds_and_how_to_verify(tmp_path: Path) -> None:
    files = write_set(tmp_path / "set")

    notes = release_notes(
        files, committed_at="2026-11-11T06:05:20Z", published_at=NOW, run_url=RUN_URL
    )

    assert f"[`13faea0245`](https://github.com/{REPO}/commit/{COMMIT})" in notes
    assert "(committed 2026-11-11T06:05:20Z), published 2026-11-11 16:40 UTC" in notes
    assert f"[Publish Builds]({RUN_URL})" in notes
    for package in files.packages:
        url = f"https://github.com/{REPO}/releases/download/daily/{package.name}"
        assert f"| [`{package.name}`]({url}) | `{package.sha256}` |" in notes
    runs = f"https://github.com/{REPO}/actions/runs"
    assert (
        "Every file was built by a job that passed, in workflow runs that passed: "
        + ", ".join(f"[{name}]({runs}/{run_id}/attempts/2)" for name, run_id in RUN_IDS.items())
        + "."
    ) in notes
    assert f"sha256sum -c {NAMES[0]}.sha256" in notes
    assert f"gh attestation verify {NAMES[0]} --repo {REPO}" in notes
    assert recorded_commit({"body": notes}) == COMMIT


def test_the_notes_do_not_pass_a_build_for_one_whose_workflows_all_passed(tmp_path: Path) -> None:
    # master is published although a job that builds no package failed: here the emulator
    # boot test of the Android run.
    files = write_set(tmp_path / "set", conclusions={"Android": "failure"})

    notes = release_notes(
        files, committed_at="2026-11-11T06:05:20Z", published_at=NOW, run_url=RUN_URL
    )

    runs = f"https://github.com/{REPO}/actions/runs"
    assert (
        "Every file was built by a job that passed, but not every workflow run of this commit "
        f"did: [Android]({runs}/{RUN_IDS['Android']}/attempts/2) concluded `failure`. Another "
        "job of such a run failed, so this build passed fewer checks than one whose runs all "
        f"passed. These runs passed: [Linux]({runs}/{RUN_IDS['Linux']}/attempts/2), "
    ) in notes
    assert "in workflow runs that passed" not in notes
    assert "has built on every platform" not in notes


@pytest.mark.parametrize(
    "release",
    [None, {}, {"body": None}, {"body": "The first daily build is being published."}],
)
def test_a_release_without_finished_notes_holds_no_commit(release: dict | None) -> None:
    assert recorded_commit(release) is None


def test_plan_assets_replaces_by_name_and_clears_everything_else() -> None:
    assets = [
        {"id": 1, "name": "QGroundControl.dmg"},
        {"id": 2, "name": "QGroundControl.dmg.sha256"},
        {"id": 3, "name": "QGroundControl.apk.next"},
        {"id": 4, "name": "QGroundControl-installer.exe"},
    ]

    plan = plan_assets(assets, ["QGroundControl.apk", "QGroundControl.dmg"], owns_release=True)

    assert plan.stale == (3,)
    assert plan.replaced == (("QGroundControl.apk", None), ("QGroundControl.dmg", 1))
    assert plan.removed == (2, 4)


def test_plan_assets_keeps_to_its_own_names_on_a_release_that_others_add_to() -> None:
    assets = [
        {"id": 1, "name": "QGroundControl.dmg"},
        {"id": 2, "name": "QGroundControl.dmg.sha256"},
        {"id": 3, "name": "QGroundControl.apk.next"},
        {"id": 4, "name": "QGroundControl-installer.exe"},
        {"id": 5, "name": "release-notes.next"},
        {"id": 6, "name": "QGroundControl.dmg.sha256.next"},
        {"id": 7, "name": "QGroundControl.apk.next.next"},
    ]
    names = ["QGroundControl.apk", "QGroundControl.dmg"]

    plan = plan_assets(assets, names, owns_release=False, reserved=["QGroundControl.dmg.sha256"])

    # Only what a failed run left under the temporary names of its own files, whether those
    # files are uploaded now or are already in place.
    assert plan.stale == (3, 6)
    assert plan.replaced == (("QGroundControl.apk", None), ("QGroundControl.dmg", 1))
    assert plan.removed == ()
    # The daily release holds one commit's files and nothing else.
    assert plan_assets(assets, names, owns_release=True).stale == (3, 5, 6, 7)


@pytest.mark.parametrize(
    ("tag", "relation", "step"),
    [(None, None, "create"), (COMMIT, None, "keep"), (OLDER_COMMIT, "ahead", "move")],
)
def test_the_tag_is_created_kept_or_moved_forward(
    tag: str | None, relation: str | None, step: str
) -> None:
    assert tag_step(tag, COMMIT, lambda _base, _head: relation) == step


@pytest.mark.parametrize("relation", ["behind", "diverged", None])
def test_the_tag_never_moves_back_or_sideways(relation: str | None) -> None:
    with pytest.raises(DailyReleaseError, match="the tag only moves forward"):
        tag_step(OLDER_COMMIT, COMMIT, lambda _base, _head: relation)


def test_set_and_ref_come_from_the_decision(github: GitHub, tmp_path: Path) -> None:
    files = write_set(tmp_path / "set")

    assert main(["--set", str(files.directory), "--ref", MASTER, "--commit", OLDER_COMMIT]) == 1
    assert github.log == []
