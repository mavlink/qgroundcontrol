"""Tests for aws_upload.py."""

from __future__ import annotations

import json
import subprocess
from datetime import UTC, datetime
from email.message import Message
from typing import TYPE_CHECKING
from urllib.error import HTTPError

import common.aws
import common.gh_actions
import pytest
from ci.publish import aws_upload, publish_decide
from ci.publish.aws_upload import (
    PublishedStateError,
    Record,
    main,
    plan_uploads,
    public_record,
    published_state,
    sanitize_ref,
)
from qgc_tools import release_tag_check
from qgc_tools.build_artifacts import PUBLISHED_BUILDS

from .._helpers import completed, load_action
from ._publish_sets import (
    COMMIT,
    MASTER,
    NAMES,
    OLDER_COMMIT,
    REPO,
    RUN_IDS,
    TAG_COMMIT,
    write_set,
)

if TYPE_CHECKING:
    from collections.abc import Sequence
    from pathlib import Path

    from ci.publish.publish_set import PublishSet

DISTRIBUTION = "E123DIST"


def _http_error(status: int) -> HTTPError:
    return HTTPError(
        "https://qgroundcontrol.s3.us-west-2.amazonaws.com", status, "error", Message(), None
    )


STABLE = "refs/heads/Stable_V5.1"
TAG = "refs/tags/v5.1.5"
# Upstream's v5.1.5 is an annotated tag: this object names the commit.
TAG_OBJECT = "9428722253158ae7600e45a72f2951c8e60b88b1"
RUN_URL = "https://github.com/mavlink/qgroundcontrol/actions/runs/43100000000"
# The AWS CLI's last line for a read that the role's policy does not allow.
ACCESS_DENIED = (
    "An error occurred (AccessDenied) when calling the GetObject operation: User: "
    "arn:aws:sts::123456789012:assumed-role/qgc-publish/GitHubActions is not authorized to "
    "perform: s3:GetObject on resource"
)


class TestSanitizeRef:
    def test_simple_branch(self) -> None:
        assert sanitize_ref("main") == "main"

    def test_slashes_replaced(self) -> None:
        assert sanitize_ref("feature/foo") == "feature_foo"

    def test_dotdot_removed(self) -> None:
        assert sanitize_ref("../../../etc/passwd") == "___etc_passwd"

    def test_special_chars(self) -> None:
        assert sanitize_ref("v1.2.3-rc1") == "v1.2.3-rc1"

    def test_spaces_replaced(self) -> None:
        assert sanitize_ref("my branch") == "my_branch"


def _keys(plan: aws_upload.S3Plan) -> list[str]:
    return [key for _, key in plan.uploads]


@pytest.mark.parametrize(
    ("ref", "commit", "directory"),
    [
        (MASTER, COMMIT, "builds/master"),
        (STABLE, COMMIT, "builds/Stable_V5.1"),
        (TAG, TAG_COMMIT, "builds/v5.1.5"),
    ],
)
def test_builds_get_the_keys_each_build_job_used_to_write(
    tmp_path: Path, ref: str, commit: str, directory: str
) -> None:
    files = write_set(tmp_path / "set", ref=ref, commit=commit)

    plan = plan_uploads(files, aws_upload.builds_prefix(files.ref_name))

    # Every published build and its checksum, under the names the build jobs gave them.
    assert sorted(_keys(plan)) == sorted(
        f"{directory}/{build.name}{suffix}"
        for build in PUBLISHED_BUILDS
        for suffix in ("", ".sha256")
    )
    # A checksum follows its package, so the two differ only for the moment between them.
    assert _keys(plan)[:2] == [f"{directory}/{NAMES[0]}", f"{directory}/{NAMES[0]}.sha256"]
    assert all(path.name == key.rsplit("/", 1)[1] for path, key in plan.uploads)
    assert plan.state_key == f"{directory}/published.json"


def test_latest_gets_the_same_names(tmp_path: Path) -> None:
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT)

    plan = plan_uploads(files, aws_upload.LATEST_PREFIX)

    assert sorted(_keys(plan)) == sorted(
        f"latest/{build.name}{suffix}" for build in PUBLISHED_BUILDS for suffix in ("", ".sha256")
    )
    assert plan.state_key == "latest/published.json"


def test_the_record_names_the_commit_every_file_and_how_each_build_concluded(
    tmp_path: Path,
) -> None:
    # The emulator boot test failed after the APK was built, and master was published.
    files = write_set(tmp_path / "set", conclusions={"Android": "failure"})

    state = published_state(
        files, run_url=RUN_URL, published_at=datetime(2026, 11, 11, 16, 40, 5, tzinfo=UTC)
    )

    assert state == {
        "schema_version": 1,
        "repository": REPO,
        "ref": MASTER,
        "commit": COMMIT,
        "published_at": "2026-11-11T16:40:05Z",
        "run": RUN_URL,
        "files": {package.name: package.sha256 for package in files.packages},
        "builds": [
            {
                "workflow": workflow,
                "run": f"https://github.com/{REPO}/actions/runs/{run_id}/attempts/2",
                "attempt": 2,
                "conclusion": "failure" if workflow == "Android" else "success",
            }
            for workflow, run_id in RUN_IDS.items()
        ],
    }
    assert sorted(state["files"]) == NAMES


class Aws:
    """Stand-in for the AWS CLI, git, GitHub's compare API and the bucket's public endpoint."""

    def __init__(self, monkeypatch: pytest.MonkeyPatch, stable_branches: str = "") -> None:
        # Every command that writes, and every record read, with credentials (`reads`) or
        # from the public endpoint (`fetches`): its key and how many writes preceded it.
        self.commands: list[list[str]] = []
        self.reads: list[tuple[str, int]] = []
        self.fetches: list[tuple[str, int]] = []
        self.git: list[tuple[str, ...]] = []
        self.failing: set[str] = set()
        # The call that fails, counted from 1 over every `aws` command, what the CLI prints
        # for a failure, and whether the CLI cannot be started at all.
        self.calls = 0
        self.failing_call: int | None = None
        self.error = "boom"
        self.missing = False
        self.git_returncode = 0
        self.stable_branches = stable_branches
        # Whether the public endpoint serves the records, what it answers the next reads of
        # a key in place of its record, its status for a key without one, and why a read
        # with credentials fails.
        self.readable = True
        self.answers: dict[str, list[bytes | Exception]] = {}
        self.missing_status = 403
        self.read_error: str | None = None
        # Records by key: what `aws s3 cp` uploaded.
        self.records: dict[str, dict] = {}
        self.relations = {(OLDER_COMMIT, COMMIT): "ahead", (COMMIT, OLDER_COMMIT): "behind"}
        # What GitHub says the release tags name, and whether as annotated tags.
        self.tags: dict[str, str] = {"v5.1.5": TAG_COMMIT}
        self.annotated = False
        self.requests: list[str] = []
        monkeypatch.setattr(common.gh_actions, "gh", self._gh)
        monkeypatch.setattr(common.aws, "run_captured", self._aws)
        monkeypatch.setattr(aws_upload, "run_captured", self._aws)
        monkeypatch.setattr(aws_upload, "compare_commits", self._compare)
        monkeypatch.setattr(release_tag_check, "run_git", self._git)
        monkeypatch.setattr(aws_upload, "fetch_bytes", self._fetch)

    def _compare(self, repo: str, base: str, head: str) -> str | None:
        assert repo == REPO
        return self.relations.get((base, head))

    def _gh(self, *args: str, **_: object):
        """Answer the refs API: the only thing these commands ask GitHub."""
        assert args[0] == "api"
        path = args[1].removeprefix(f"repos/{REPO}/")
        self.requests.append(path)
        tag = path.removeprefix("git/ref/tags/")
        if tag in self.tags and tag != path:
            target = ("tag", TAG_OBJECT) if self.annotated else ("commit", self.tags[tag])
            return completed(
                json.dumps({"object": dict(zip(("type", "sha"), target, strict=True))})
            )
        if path == f"git/tags/{TAG_OBJECT}" and self.annotated:
            return completed(json.dumps({"object": {"type": "commit", "sha": self.tags["v5.1.5"]}}))
        return completed('{"message":"Not Found"}', 1, "gh: Not Found (HTTP 404)")

    def _read(self, command: list[str], error: str | None):
        assert command[3:5] == ["--bucket", "qgroundcontrol"] and command[5] == "--key"
        self.reads.append((command[6], len(self.commands)))
        if error is not None:
            return completed(returncode=254, stderr=f"\n{error}\n")
        if command[6] not in self.records:
            # What S3 answers a caller that may list the bucket.
            missing = "An error occurred (NoSuchKey) when calling the GetObject operation"
            return completed(
                returncode=254, stderr=f"\n{missing}: The specified key does not exist.\n"
            )
        with open(command[7], "w", encoding="utf-8") as record:
            json.dump(self.records[command[6]], record)
        return completed('{"ContentType": "application/json"}')

    def _aws(self, cmd: Sequence[str], *, check: bool = False, **_: object):
        command = list(cmd)
        self.calls += 1
        fails = self.calls == self.failing_call
        if fails and self.missing:
            raise FileNotFoundError(2, "No such file or directory", "aws")
        if command[:3] == ["aws", "s3api", "get-object"]:
            return self._read(command, self.error if fails else self.read_error)
        self.commands.append(command)
        returncode = 1 if fails or any(part in self.failing for part in command) else 0
        if check and returncode:
            raise subprocess.CalledProcessError(returncode, command, stderr=self.error)
        if not returncode and command[:3] == ["aws", "s3", "cp"] and command[4].endswith(".json"):
            key = command[4].removeprefix("s3://qgroundcontrol/")
            with open(command[3], encoding="utf-8") as record:
                self.records[key] = json.load(record)
        return completed(returncode=returncode, stderr=self.error if returncode else "")

    def _git(self, *args: str, **_: object):
        self.git.append(args)
        return completed(self.stable_branches, self.git_returncode)

    def _fetch(self, url: str, **_: object) -> bytes:
        key = url.removeprefix("https://qgroundcontrol.s3.us-west-2.amazonaws.com/")
        self.fetches.append((key, len(self.commands)))
        if self.answers.get(key):
            answer = self.answers[key].pop(0)
            if isinstance(answer, Exception):
                raise answer
            return answer
        if not self.readable:
            raise _http_error(403)
        if key not in self.records:
            raise _http_error(self.missing_status)
        return json.dumps(self.records[key]).encode()

    @property
    def keys(self) -> list[str]:
        """The keys uploaded, in order."""
        return [
            command[4].removeprefix("s3://qgroundcontrol/")
            for command in self.commands
            if command[:3] == ["aws", "s3", "cp"]
        ]

    @property
    def invalidations(self) -> list[list[str]]:
        return [command for command in self.commands if command[1] == "cloudfront"]


@pytest.fixture(params=aws_upload.RECORD_READS)
def record_read(request: pytest.FixtureRequest, monkeypatch: pytest.MonkeyPatch) -> str:
    """Run the test once with each way of reading the record that a directory holds."""
    monkeypatch.setenv("PUBLISH_RECORD_READ", request.param)
    return request.param


def _run(command: str, files: PublishSet, *extra: str) -> int:
    argv = [command, "--set", str(files.directory), "--ref", files.ref, "--commit", files.commit]
    return main([*argv, "--run-url", RUN_URL, *extra])


def _seed(aws: Aws, tag: str = "v5.1.4", commit: str = COMMIT) -> None:
    """Give latest/ the record an administrator writes for the release it already holds."""
    aws.records["latest/published.json"] = aws_upload.seed_record(
        tag, commit, repository=REPO, published_at=datetime(2026, 11, 12, 9, 0, tzinfo=UTC)
    )


def test_publish_uploads_the_whole_set_public_and_records_it_last(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    aws = Aws(monkeypatch)
    files = write_set(tmp_path / "set")

    assert _run("publish", files) == 0

    assert aws.keys == [
        *_keys(plan_uploads(files, "builds/master")),
        "builds/master/published.json",
    ]
    first = aws.commands[0]
    assert first == [
        "aws",
        "s3",
        "cp",
        str(files.directory / NAMES[0]),
        f"s3://qgroundcontrol/builds/master/{NAMES[0]}",
        "--acl",
        "public-read",
    ]
    assert all(command[-2:] == ["--acl", "public-read"] for command in aws.commands)
    # Invalidating builds/ is a command of its own, which the job runs after this one.
    assert aws.invalidations == []
    assert "CloudFront" not in capsys.readouterr().out
    assert aws.git == []
    record = aws.records["builds/master/published.json"]
    assert (record["commit"], record["ref"], record["run"]) == (COMMIT, MASTER, RUN_URL)
    assert public_record("builds/master") == Record(COMMIT, MASTER, tuple(NAMES), REPO)
    assert [build["conclusion"] for build in record["builds"]] == ["success"] * 4
    summary = gh_summary.read_text(encoding="utf-8")
    assert "### S3 builds/master/" in summary
    assert "✅ published" in summary
    assert (
        "| Android build | packages built; workflow passed ([run 42969993919, attempt 2]"
        f"(https://github.com/{REPO}/actions/runs/42969993919/attempts/2)) |"
    ) in summary


def _invalidate(ref: str, *extra: str) -> list[str]:
    return ["invalidate", "--ref", ref, "--distribution-id", DISTRIBUTION, *extra]


@pytest.mark.parametrize(
    ("command", "ref", "commit", "extra", "path"),
    [
        ("publish", MASTER, COMMIT, (), "/builds/master/*"),
        ("publish", STABLE, COMMIT, (), "/builds/Stable_V5.1/*"),
        ("publish", TAG, TAG_COMMIT, (), "/builds/v5.1.5/*"),
        ("promote", TAG, TAG_COMMIT, ("--latest",), "/latest/*"),
    ],
)
def test_a_directory_is_invalidated_with_one_wildcard_path_after_every_upload(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    gh_summary: Path,
    command: str,
    ref: str,
    commit: str,
    extra: tuple[str, ...],
    path: str,
) -> None:
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    _seed(aws)
    files = write_set(tmp_path / "set", ref=ref, commit=commit)

    assert _run(command, files) == 0
    # Invalidating is a command of its own, which the job runs after this one.
    assert aws.invalidations == []
    assert main(_invalidate(ref, *extra)) == 0

    # CloudFront counts a wildcard path as one path, whatever it covers.
    invalidation = ["aws", "cloudfront", "create-invalidation", "--distribution-id", DISTRIBUTION]
    assert aws.invalidations == [[*invalidation, "--paths", path]]
    # It follows the record, which is the last upload.
    assert aws.commands[-2][4] == f"s3://qgroundcontrol{path[:-1]}published.json"
    assert aws.commands[-1] == aws.invalidations[0]
    summary = gh_summary.read_text(encoding="utf-8")
    assert f"### CloudFront {path[1:-1]}" in summary
    assert f"| Result | ✅ invalidated `{path}` |" in summary


@pytest.mark.parametrize(
    ("command", "target", "ref", "commit", "extra", "directory"),
    [
        ("publish", "s3", MASTER, COMMIT, (), "builds/master"),
        ("promote", "latest", TAG, TAG_COMMIT, ("--latest",), "latest"),
    ],
)
def test_a_failed_invalidation_fails_the_job_and_leaves_the_record_as_uploaded(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    command: str,
    target: str,
    ref: str,
    commit: str,
    extra: tuple[str, ...],
    directory: str,
) -> None:
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    _seed(aws)
    aws.failing.add("create-invalidation")
    files = write_set(tmp_path / "set", ref=ref, commit=commit)
    argv = _invalidate(ref, *extra)

    assert _run(command, files) == 0
    assert main(argv) == 1

    assert public_record(directory) == Record(commit, ref, tuple(NAMES), REPO)
    assert (
        f"::error title=CloudFront {directory}/ not invalidated::The files and the record of "
        f"{directory}/ are in S3 and are correct. Only the CloudFront invalidation of "
        f"/{directory}/* failed, so CloudFront may serve the previous files until its copies "
        "expire: boom. Re-run this job to send the invalidation again. Only a run that was "
        "started by hand, like a new dispatch for the ref, uploads the files again first."
    ) in capsys.readouterr().out

    # The re-run job's `confirm` step finds the directory up to date, so the job skips the
    # upload, and this command is all that it repeats. A run that was started by hand
    # publishes again whatever the directory holds, on a re-run too.
    published = publish_decide.publishing_ref(files.ref_name)
    assert published is not None
    for by_hand in (False, True):
        proceed, _ = publish_decide.verdict(
            REPO, published, target, commit, lambda *_: None, republish=by_hand
        )
        assert proceed is by_hand
    aws.failing.clear()
    uploaded = list(aws.keys)
    assert main(argv) == 0
    assert aws.keys == uploaded
    assert len(aws.invalidations) == 2


@pytest.mark.parametrize(("extra", "directory"), [((), "builds/master"), (("--latest",), "latest")])
def test_nothing_is_invalidated_in_a_dry_run_or_without_a_distribution_or_credentials(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    gh_summary: Path,
    extra: tuple[str, ...],
    directory: str,
) -> None:
    aws = Aws(monkeypatch)
    argv = ["invalidate", "--ref", MASTER, *extra]

    assert main([*argv, "--distribution-id", DISTRIBUTION, "--dry-run"]) == 0
    out = capsys.readouterr().out
    assert f"one CloudFront invalidation: /{directory}/*" in out
    assert "::warning" not in out
    assert "dry run: nothing was invalidated" in gh_summary.read_text(encoding="utf-8")

    assert main(argv) == 0
    out = capsys.readouterr().out
    assert out.count("::warning::No CloudFront distribution is configured") == 1
    assert f"cached copies of {directory}/ are served until they expire" in out

    # The upload has warned of the missing credentials already.
    monkeypatch.setenv("HAS_AWS_CREDENTIALS", "false")
    assert main([*argv, "--distribution-id", DISTRIBUTION]) == 0
    assert "::warning" not in capsys.readouterr().out
    assert aws.commands == []


@pytest.mark.parametrize("ref", ["master", "refs/pull/15100/merge", "refs/heads/", ""])
def test_only_a_branch_or_tag_s_directory_is_invalidated(
    monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str], ref: str
) -> None:
    aws = Aws(monkeypatch)
    monkeypatch.delenv("PUBLISH_REF", raising=False)

    assert main(["invalidate", "--ref", ref, "--distribution-id", DISTRIBUTION]) == 1

    assert aws.commands == []
    assert "::error::" in capsys.readouterr().out


def test_promote_publishes_a_stable_release_tag_to_latest_and_records_it_last(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    aws = Aws(monkeypatch, stable_branches="  origin/Stable_V5.1\n")
    _seed(aws)
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT)

    assert _run("promote", files) == 0

    # The checkout is the default branch, so the check names the tag's commit, not HEAD.
    assert aws.git == [("branch", "-r", "--contains", TAG_COMMIT, "--list", "origin/Stable*")]
    assert aws.keys == [*_keys(plan_uploads(files, "latest")), "latest/published.json"]
    assert aws.invalidations == []
    assert aws.records["latest/published.json"]["commit"] == TAG_COMMIT


@pytest.mark.parametrize(
    ("ref", "commit", "stable_branches", "returncode", "message"),
    [
        (TAG, TAG_COMMIT, "", 0, "is not a release tag on a Stable* branch"),
        (TAG, TAG_COMMIT, "", 128, "Cannot determine whether tag is on a Stable* branch"),
        ("refs/tags/v5.2.0-dev", TAG_COMMIT, "origin/Stable_V5.1\n", 0, "not a release tag"),
        (MASTER, COMMIT, "origin/Stable_V5.1\n", 0, "is not a tag"),
        (STABLE, COMMIT, "origin/Stable_V5.1\n", 0, "is not a tag"),
    ],
)
def test_latest_takes_only_release_tags_on_a_stable_branch(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    ref: str,
    commit: str,
    stable_branches: str,
    returncode: int,
    message: str,
) -> None:
    aws = Aws(monkeypatch, stable_branches=stable_branches)
    aws.git_returncode = returncode
    files = write_set(tmp_path / "set", ref=ref, commit=commit)

    assert _run("promote", files) == 1

    assert aws.commands == []
    assert message in capsys.readouterr().out


@pytest.mark.parametrize(
    ("command", "prefix"), [("publish", "builds/v5.1.5"), ("promote", "latest")]
)
@pytest.mark.parametrize(
    ("now", "problem"),
    [
        (COMMIT, f"the tag v5.1.5 now names {COMMIT}, not {TAG_COMMIT}"),
        (None, "the tag v5.1.5 no longer exists"),
    ],
)
def test_a_tag_s_packages_are_not_uploaded_once_the_tag_names_another_commit(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    command: str,
    prefix: str,
    now: str | None,
    problem: str,
) -> None:
    # The tag was moved or deleted while this job waited its turn: the set is another commit's.
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    _seed(aws)
    aws.tags = {} if now is None else {"v5.1.5": now}
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT)

    assert _run(command, files) == 1

    assert aws.commands == []
    assert "builds/v5.1.5/published.json" not in aws.records
    assert aws.records["latest/published.json"]["ref"] == "refs/tags/v5.1.4"
    out = capsys.readouterr().out
    assert f"::error title=S3 {prefix}/ not published::Nothing is uploaded: {problem}" in out


@pytest.mark.parametrize("command", ["publish", "promote"])
def test_an_annotated_tag_is_followed_to_its_commit_before_the_upload(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, command: str
) -> None:
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    _seed(aws)
    aws.annotated = True
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT)

    assert _run(command, files) == 0

    assert aws.requests == ["git/ref/tags/v5.1.5", f"git/tags/{TAG_OBJECT}"]
    # The tag is read after the record and before the first upload.
    assert aws.keys


def test_a_branch_s_packages_ask_nothing_about_tags(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    aws = Aws(monkeypatch)

    assert _run("publish", write_set(tmp_path / "set")) == 0

    assert aws.requests == []


# Every command as a job runs it: the two that read a record, with each way to read it.
COMMANDS = [
    *(
        pytest.param(command, ("--record-read", read), id=f"{command} {read}")
        for command in ("publish", "promote")
        for read in aws_upload.RECORD_READS
    ),
    pytest.param("invalidate", ("--distribution-id", DISTRIBUTION), id="invalidate"),
    pytest.param(
        "invalidate", ("--latest", "--distribution-id", DISTRIBUTION), id="invalidate latest"
    ),
]


def _command(command: str, options: tuple[str, ...], files: PublishSet) -> int:
    if command == "invalidate":
        return main([command, "--ref", files.ref, *options])
    return _run(command, files, *options)


@pytest.mark.parametrize(("command", "options"), COMMANDS)
@pytest.mark.parametrize("asked", ["flag", "environment"])
def test_a_dry_run_uploads_nothing_and_makes_no_call_that_needs_credentials(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    gh_summary: Path,
    command: str,
    options: tuple[str, ...],
    asked: str,
) -> None:
    # A dry run configures no credentials, so the `aws` CLI must not be started at all,
    # however the record is read. A read of the record's public URL is no such call.
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    _seed(aws)
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT)
    monkeypatch.setenv("PUBLISH_DRY_RUN", str(asked == "environment").lower())

    assert _command(command, (*options, *(["--dry-run"] if asked == "flag" else [])), files) == 0

    assert (aws.calls, aws.commands, aws.reads) == (0, [], [])
    assert not (files.directory / "published.json").exists()
    assert "dry run: nothing was" in gh_summary.read_text(encoding="utf-8")


def test_without_credentials_nothing_is_uploaded_and_the_job_passes_with_a_warning(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    aws = Aws(monkeypatch)
    monkeypatch.setenv("HAS_AWS_CREDENTIALS", "false")
    files = write_set(tmp_path / "set")

    assert _run("publish", files) == 0

    assert (aws.commands, aws.reads) == ([], [])
    assert "::warning::No AWS credentials are configured" in capsys.readouterr().out


def test_a_failed_upload_stops_before_the_record(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    aws = Aws(monkeypatch)
    files = write_set(tmp_path / "set")
    aws.failing.add(f"s3://qgroundcontrol/builds/master/{NAMES[1]}")

    assert _run("publish", files) == 1

    assert aws.keys == [
        f"builds/master/{NAMES[0]}",
        f"builds/master/{NAMES[0]}.sha256",
        f"builds/master/{NAMES[1]}",
    ]
    assert public_record("builds/master") is None


def test_a_record_that_cannot_be_read_back_fails_the_publish(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    # The endpoint fails after the upload: nothing says that the record can be read.
    aws = Aws(monkeypatch)
    aws.answers["builds/master/published.json"] = [_http_error(403), _http_error(503)]
    files = write_set(tmp_path / "set")

    assert _run("publish", files) == 1

    out = capsys.readouterr().out
    url = "https://qgroundcontrol.s3.us-west-2.amazonaws.com/builds/master/published.json"
    assert (
        "::error title=S3 builds/master/ not published::s3://qgroundcontrol/builds/master/"
        f"published.json was uploaded but cannot be read back from {url} (cannot read {url}: "
        "HTTP 503). A run that reads the record there"
    ) in out


@pytest.mark.parametrize(
    ("command", "prefix"), [("publish", "builds/v5.1.5"), ("promote", "latest")]
)
def test_the_record_is_read_before_the_first_upload(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, record_read: str, command: str, prefix: str
) -> None:
    # builds/v5.1.5/ has no record: the tag's first publication. latest/ holds v5.1.4.
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    _seed(aws)
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT)

    assert _run(command, files) == 0

    record = f"{prefix}/published.json"
    # The public endpoint is read once more after the record's upload: the read-back.
    read_back = (record, 2 * len(NAMES) + 1)
    if record_read == "credentialed":
        assert (aws.reads, aws.fetches) == ([(record, 0)], [read_back])
    else:
        assert (aws.reads, aws.fetches) == ([], [(record, 0), read_back])
    assert aws.records[record]["commit"] == TAG_COMMIT


@pytest.mark.parametrize(
    ("option", "variable", "read"),
    [
        # Nothing set, and a variable that a workflow passes on empty: the public read.
        ((), None, "public"),
        ((), "", "public"),
        ((), "public", "public"),
        ((), "credentialed", "credentialed"),
        (("--record-read", "credentialed"), None, "credentialed"),
        # The option wins over the variable.
        (("--record-read", "public"), "credentialed", "public"),
        (("--record-read", "credentialed"), "public", "credentialed"),
    ],
)
@pytest.mark.parametrize("command", ["publish", "promote"])
def test_the_record_is_read_as_the_option_or_the_variable_says_and_publicly_by_default(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    command: str,
    option: tuple[str, ...],
    variable: str | None,
    read: str,
) -> None:
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    _seed(aws)
    if variable is None:
        monkeypatch.delenv("PUBLISH_RECORD_READ", raising=False)
    else:
        monkeypatch.setenv("PUBLISH_RECORD_READ", variable)
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT)

    assert _run(command, files, *option) == 0

    # Only the credentialed read starts `aws s3api get-object`.
    assert bool(aws.reads) is (read == "credentialed")
    assert aws.calls == 2 * len(NAMES) + 1 + len(aws.reads)


@pytest.mark.parametrize(
    ("option", "variable"),
    [
        (("--record-read", "private"), None),
        (("--record-read", ""), None),
        ((), "Credentialed"),
        ((), "true"),
        # The variable does not stand in for an option that is wrong.
        (("--record-read", "nonsense"), "credentialed"),
    ],
)
@pytest.mark.parametrize("command", ["publish", "promote"])
def test_an_unknown_way_to_read_the_record_is_refused_before_anything_is_read(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    command: str,
    option: tuple[str, ...],
    variable: str | None,
) -> None:
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    _seed(aws)
    if variable is None:
        monkeypatch.delenv("PUBLISH_RECORD_READ", raising=False)
    else:
        monkeypatch.setenv("PUBLISH_RECORD_READ", variable)
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT)

    assert _run(command, files, *option) == 1

    assert (aws.calls, aws.fetches) == (0, [])
    value = option[1] if option else variable
    assert (
        f"::error::The record read must be public or credentialed, not {value!r} "
        "(--record-read, PUBLISH_RECORD_READ)"
    ) in capsys.readouterr().out


@pytest.mark.parametrize("status", [403, 404])
def test_a_directory_whose_public_record_is_missing_holds_nothing(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, status: int
) -> None:
    # The bucket cannot be listed, so S3 answers 403 for a missing key to an anonymous
    # caller, and 404 if it ever can be.
    aws = Aws(monkeypatch)
    aws.missing_status = status
    files = write_set(tmp_path / "set")

    assert _run("publish", files) == 0

    assert aws.reads == []
    assert aws.fetches[0] == ("builds/master/published.json", 0)
    assert public_record("builds/master") == Record(COMMIT, MASTER, tuple(NAMES), REPO)


@pytest.mark.parametrize(
    ("command", "prefix"), [("publish", "builds/v5.1.5"), ("promote", "latest")]
)
@pytest.mark.parametrize(
    ("answer", "problem"),
    [
        (_http_error(500), "published.json: HTTP 500"),
        (_http_error(400), "published.json: HTTP 400"),
        # What `fetch_bytes` raises once its attempts at a connection are used up.
        (RuntimeError("Failed to fetch after 3 attempts: timed out"), "after 3 attempts"),
        (OSError("Network is unreachable"), "Network is unreachable"),
        (b"<html>Service Unavailable</html>", "Expecting value"),
        (b'{"ref": "refs/tags/v5.1.4"}', "published.json does not record a commit"),
    ],
    ids=["500", "400", "no connection", "no network", "not json", "no commit"],
)
def test_a_public_record_that_cannot_be_read_stops_the_job_before_it_uploads_anything(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    command: str,
    prefix: str,
    answer: bytes | Exception,
    problem: str,
) -> None:
    # Only "no such object" means that nothing is published: any other answer leaves open
    # that the directory holds a newer build.
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    _seed(aws)
    aws.answers[f"{prefix}/published.json"] = [answer]
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT)

    assert _run(command, files) == 1

    assert (aws.calls, aws.commands) == (0, [])
    out = capsys.readouterr().out
    url = f"https://qgroundcontrol.s3.us-west-2.amazonaws.com/{prefix}/published.json"
    assert out.count("::error") == 1
    assert f"::error title=S3 {prefix}/ not published::" in out
    assert url in out and problem in out
    assert "so nothing is uploaded: what the directory holds is unknown" in out


def test_a_record_that_the_public_endpoint_hides_is_found_by_the_read_back_of_the_next_write(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    # builds/master/ holds COMMIT, but anonymous readers get 403, as they do for a missing
    # record. The public read cannot tell the two apart, so the older commit is uploaded: the
    # read-back of its record is what fails the job and says why that matters.
    aws = Aws(monkeypatch)
    aws.records["builds/master/published.json"] = {"ref": MASTER, "commit": COMMIT}
    aws.readable = False
    files = write_set(tmp_path / "set", commit=OLDER_COMMIT)

    assert _run("publish", files) == 1

    assert aws.keys[-1] == "builds/master/published.json"
    assert (
        "::error title=S3 builds/master/ not published::s3://qgroundcontrol/builds/master/"
        "published.json was uploaded but cannot be read back from https://qgroundcontrol.s3."
        "us-west-2.amazonaws.com/builds/master/published.json. A run that reads the record "
        "there (the default) takes a record it cannot read for none, which switches the "
        "never-regress check off: an older commit can then replace a newer one in a builds/ "
        "directory, and latest/ refuses every release. Do not ignore this failure: make the "
        "object publicly readable (the bucket's public-access and ACL settings), then re-run "
        "this job."
    ) in capsys.readouterr().out


def test_a_newer_commit_in_a_branch_s_directory_is_not_replaced_by_an_older_one(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    record_read: str,
) -> None:
    aws = Aws(monkeypatch)
    aws.records["builds/master/published.json"] = {"ref": MASTER, "commit": COMMIT}
    files = write_set(tmp_path / "set", commit=OLDER_COMMIT)

    assert _run("publish", files) == 1

    assert aws.commands == []
    assert aws.records["builds/master/published.json"]["commit"] == COMMIT
    assert (
        "::error title=S3 builds/master/ not published::Nothing is uploaded: builds/master/ "
        f"holds {COMMIT}, and {OLDER_COMMIT} is behind relative to it"
    ) in capsys.readouterr().out


def test_latest_without_a_record_is_not_taken_for_empty(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    record_read: str,
) -> None:
    # latest/ holds the release that the build jobs uploaded for themselves, and no record:
    # nothing says that it is older than v5.1.5, whose builds have just completed.
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT)

    assert _run("publish", files) == 0
    assert _run("promote", files) == 1

    assert aws.records["builds/v5.1.5/published.json"]["commit"] == TAG_COMMIT
    assert not [key for key in aws.keys if key.startswith("latest/")]
    assert aws.invalidations == []
    assert "latest/published.json" not in aws.records
    assert (
        "::error title=S3 latest/ not published::Nothing is uploaded: latest/ has no "
        "publication record (published.json), so the release it holds cannot be compared with "
        "v5.1.5. Write the record of the release that latest/ holds (`aws_upload.py seed`, "
        ".github/ci-notes/platforms.md#seeding-the-latest-record) and re-run this job, or "
        "publish v5.1.5 by hand with force_latest."
    ) in capsys.readouterr().out


def test_a_request_by_hand_with_force_latest_promotes_without_a_record(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    monkeypatch.setenv("PUBLISH_FORCE_LATEST", "true")
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT)

    assert _run("promote", files) == 0

    assert aws.keys[-1] == "latest/published.json"
    # The record every later release is compared with.
    assert public_record("latest") == Record(TAG_COMMIT, TAG, tuple(NAMES), REPO)


@pytest.mark.parametrize(
    ("held", "promoted"), [("v5.1.4", True), ("v5.1.5", True), ("v5.1.6", False), ("v5.2.0", False)]
)
def test_a_seeded_record_orders_the_first_release_that_is_promoted(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    record_read: str,
    held: str,
    promoted: bool,
) -> None:
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    _seed(aws, held)
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT)

    assert _run("promote", files) == (0 if promoted else 1)

    record = public_record("latest")
    assert record is not None
    assert record.release == ("v5.1.5" if promoted else held)
    assert bool(aws.commands) is promoted
    refused = f"Nothing is uploaded: latest/ holds the newer release {held}"
    assert (refused in capsys.readouterr().out) is not promoted


def test_seed_prints_the_record_of_a_release_without_touching_aws(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    aws = Aws(monkeypatch)
    monkeypatch.delenv("PUBLISH_REF", raising=False)
    monkeypatch.delenv("PUBLISH_COMMIT", raising=False)

    assert main(["seed", "--tag", "v5.1.4", "--commit", COMMIT]) == 0

    out = capsys.readouterr().out
    seeded = json.loads(out)
    assert seeded == {
        "schema_version": 1,
        "repository": REPO,
        "ref": "refs/tags/v5.1.4",
        "commit": COMMIT,
        "published_at": seeded["published_at"],
        "run": "",
        "files": {},
    }
    # The fields, their order and the layout of a record that a publication writes.
    written = published_state(
        write_set(tmp_path / "set"), run_url=RUN_URL, published_at=datetime.now(UTC)
    )
    assert list(written) == [*seeded, "builds"]
    assert out == json.dumps(seeded, indent=2) + "\n"
    assert aws_upload._record("seed", out) == Record(COMMIT, "refs/tags/v5.1.4", (), REPO)
    assert (aws.commands, aws.reads) == ([], [])


@pytest.mark.parametrize(
    ("tag", "commit"),
    [("v5.1", COMMIT), ("v5.2.0-dev", COMMIT), ("refs/tags/v5.1.4", COMMIT), ("v5.1.4", "13faea0")],
)
def test_seed_refuses_what_is_not_a_release_and_its_commit(
    capsys: pytest.CaptureFixture[str], tag: str, commit: str
) -> None:
    assert main(["seed", "--tag", tag, "--commit", commit]) == 1

    captured = capsys.readouterr()
    assert "::error::" in captured.out
    assert "schema_version" not in captured.out


@pytest.mark.parametrize("command", ["publish", "promote"])
@pytest.mark.parametrize(
    "error",
    [
        ACCESS_DENIED,
        "An error occurred (ExpiredToken) when calling the GetObject operation: expired",
        'Could not connect to the endpoint URL: "https://qgroundcontrol.s3.us-west-2.amazonaws.com/"',
        "",
    ],
    ids=["access denied", "expired token", "no connection", "no message"],
)
def test_a_record_the_role_cannot_read_stops_the_job_before_it_uploads_anything(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    command: str,
    error: str,
) -> None:
    # Denied is what S3 answers for a record that exists, and for a missing one when the role
    # may not list the bucket: either way the directory may hold a newer build.
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    aws.read_error = error
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT)

    assert _run(command, files, "--record-read", "credentialed") == 1

    assert (aws.commands, aws.fetches) == ([], [])
    out = capsys.readouterr().out
    assert "published.json, so nothing is uploaded" in out
    for permission in (
        "s3:GetObject on builds/*/published.json and latest/published.json",
        "s3:ListBucket on the bucket",
        "s3:PutObject and s3:PutObjectAcl on builds/* and latest/*",
        "cloudfront:CreateInvalidation",
    ):
        assert permission in out


def test_a_newer_build_whose_record_the_public_endpoint_hides_is_not_overwritten(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    # builds/master/ holds COMMIT, but anonymous readers get 403, as they do for a missing
    # record, so the decision took the directory for empty and sent an older commit. The
    # read with credentials sees the record.
    aws = Aws(monkeypatch)
    aws.records["builds/master/published.json"] = {"ref": MASTER, "commit": COMMIT}
    aws.readable = False
    files = write_set(tmp_path / "set", commit=OLDER_COMMIT)

    assert _run("publish", files, "--record-read", "credentialed") == 1

    assert aws.commands == []
    assert aws.records["builds/master/published.json"]["commit"] == COMMIT
    assert f"builds/master/ holds {COMMIT}, and {OLDER_COMMIT} is behind" in capsys.readouterr().out


def test_a_record_that_names_no_commit_stops_the_job_too(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    record_read: str,
) -> None:
    aws = Aws(monkeypatch)
    aws.records["builds/master/published.json"] = {"ref": MASTER}

    assert _run("publish", write_set(tmp_path / "set")) == 1

    assert aws.commands == []
    assert "builds/master/published.json does not record a commit" in capsys.readouterr().out


def test_a_branch_s_directory_moves_forward_and_takes_its_commit_again(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    aws = Aws(monkeypatch)
    aws.records["builds/master/published.json"] = {"ref": MASTER, "commit": OLDER_COMMIT}
    files = write_set(tmp_path / "set")

    assert _run("publish", files) == 0
    assert aws.records["builds/master/published.json"]["commit"] == COMMIT
    # A request by hand publishes packages rebuilt for the same commit.
    assert _run("publish", files) == 0
    assert len(aws.keys) == 2 * (2 * len(NAMES) + 1)


@pytest.mark.parametrize(
    ("held", "force", "promoted"),
    [
        ("refs/tags/v5.1.4", "false", True),
        ("refs/tags/v5.1.5", "false", True),
        ("refs/tags/v5.1.6", "false", False),
        ("refs/tags/v5.10.0", "false", False),
        ("refs/tags/v5.1.6", "true", True),
        # A record that names no release cannot be compared with one.
        ("", "false", False),
        ("", "true", True),
    ],
)
def test_latest_goes_back_to_an_older_release_only_when_forced(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    record_read: str,
    held: str,
    force: str,
    promoted: bool,
) -> None:
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    aws.records["latest/published.json"] = {"ref": held, "commit": COMMIT}
    monkeypatch.setenv("PUBLISH_FORCE_LATEST", force)
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT)

    assert _run("promote", files) == (0 if promoted else 1)

    assert bool(aws.commands) is promoted
    assert aws.records["latest/published.json"]["commit"] == (TAG_COMMIT if promoted else COMMIT)


def test_latest_takes_the_release_it_holds_again_when_its_tag_names_another_commit(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    # v5.1.5 was promoted from COMMIT; the tag was then moved to TAG_COMMIT and built again.
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    _seed(aws, "v5.1.5", COMMIT)
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT)

    assert _run("promote", files) == 0

    assert public_record("latest") == Record(TAG_COMMIT, TAG, tuple(NAMES), REPO)
    # At the same commit it is uploaded again: only a request by hand gets this far.
    uploads = len(aws.keys)
    assert _run("promote", files) == 0
    assert len(aws.keys) == 2 * uploads


@pytest.mark.parametrize(
    ("command", "prefix", "ref", "commit"),
    [("publish", "builds/master", MASTER, COMMIT), ("promote", "latest", TAG, TAG_COMMIT)],
)
def test_another_repository_s_build_is_neither_compared_with_nor_replaced(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    record_read: str,
    command: str,
    prefix: str,
    ref: str,
    commit: str,
) -> None:
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    foreign = {
        "schema_version": 1,
        "repository": "someone/qgroundcontrol",
        "ref": "refs/tags/v5.1.4" if prefix == "latest" else MASTER,
        "commit": OLDER_COMMIT,
    }
    aws.records[f"{prefix}/published.json"] = dict(foreign)
    files = write_set(tmp_path / "set", ref=ref, commit=commit)

    assert _run(command, files) == 1

    assert (aws.commands, aws.records[f"{prefix}/published.json"]) == ([], foreign)
    assert (
        f"Nothing is uploaded: {prefix}/ holds a build of someone/qgroundcontrol, not of "
        f"{REPO}: its record must be removed before this repository publishes there"
    ) in capsys.readouterr().out


@pytest.mark.parametrize(
    ("command", "prefix"), [("publish", "builds/v5.1.5"), ("promote", "latest")]
)
def test_a_set_with_fewer_packages_is_published_as_it_is_and_nothing_is_deleted(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
    command: str,
    prefix: str,
) -> None:
    # A Stable branch cut before the Arm installer existed releases after a line that has it.
    arm = "QGroundControl-installer-ARM64.exe"
    aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
    previous = {
        "ref": "refs/tags/v5.1.4",
        "commit": COMMIT,
        "files": dict.fromkeys(NAMES, "0" * 64),
    }
    aws.records[f"{prefix}/published.json"] = previous
    files = write_set(tmp_path / "set", ref=TAG, commit=TAG_COMMIT, without=[arm])
    names = [name for name in NAMES if name != arm]

    assert _run(command, files) == 0

    # Exactly the five packages and their checksums, then the record that lists them.
    uploaded = [f"{prefix}/{name}{suffix}" for name in names for suffix in ("", ".sha256")]
    assert aws.keys == [*uploaded, f"{prefix}/published.json"]
    assert sorted(aws.records[f"{prefix}/published.json"]["files"]) == names
    # The installer of the earlier release stays in the bucket: nothing deletes, the job says so.
    assert all(sent[:3] == ["aws", "s3", "cp"] for sent in aws.commands)
    out = capsys.readouterr().out
    assert f"{prefix}/ still holds {arm} from an earlier publication" in out
    summary = gh_summary.read_text(encoding="utf-8")
    assert f"| Left from an earlier publication | `{arm}` |" in summary
    assert f"`{arm}` |" not in summary.split("| File |")[1]
    assert all(f"| `{name}` |" in summary for name in names)

    # Published again, nothing is left that the previous record lists.
    assert _run(command, files) == 0
    assert "still holds" not in capsys.readouterr().out


def test_only_the_set_the_job_was_given_is_uploaded(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    aws = Aws(monkeypatch)
    files = write_set(tmp_path / "set")
    argv = ["publish", "--set", str(files.directory), "--ref", MASTER]

    assert main([*argv, "--commit", TAG_COMMIT]) == 1
    (files.directory / NAMES[0]).write_bytes(b"swapped")
    assert main([*argv, "--commit", COMMIT]) == 1

    assert aws.commands == []
    assert "SHA-256 checksum mismatch" in capsys.readouterr().out


def _serve(monkeypatch: pytest.MonkeyPatch, answer: bytes | Exception) -> list[str]:
    urls: list[str] = []

    def fetch(url: str, **_: object) -> bytes:
        urls.append(url)
        if isinstance(answer, Exception):
            raise answer
        return answer

    monkeypatch.setattr(aws_upload, "fetch_bytes", fetch)
    return urls


# What the AWS CLI prints when it has no credentials, when a call is denied or throttled, and
# when it cannot reach AWS. None stands for a CLI that cannot be started.
AWS_FAILURES = (
    'Unable to locate credentials. You can configure credentials by running "aws configure".',
    "An error occurred (AccessDenied) when calling the PutObject operation: Access Denied",
    "An error occurred (Throttling) when calling the CreateInvalidation operation (reached max "
    "retries: 2): Rate exceeded",
    'Could not connect to the endpoint URL: "https://cloudfront.amazonaws.com/2020-05-31/"',
    None,
)


@pytest.mark.parametrize(("command", "options"), COMMANDS)
def test_whichever_aws_call_fails_the_command_reports_an_error_and_fails(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    command: str,
    options: tuple[str, ...],
) -> None:
    def run(directory: str, failing_call: int | None, failure: str | None) -> tuple[Aws, int]:
        aws = Aws(monkeypatch, stable_branches="origin/Stable_V5.1\n")
        _seed(aws)
        aws.failing_call, aws.missing = failing_call, failure is None
        aws.error = failure or ""
        files = write_set(tmp_path / directory, ref=TAG, commit=TAG_COMMIT)
        # An exception that escapes here is a traceback in the job's log.
        return aws, _command(command, options, files)

    # An upload writes each package with its checksum, then the record, after the read of
    # the record that the directory holds, which only the credentialed read asks of AWS.
    aws, status = run("passes", None, "")
    calls = 1 if command == "invalidate" else 2 * len(NAMES) + 1 + len(aws.reads)
    assert bool(aws.reads) is ("credentialed" in options)
    assert (status, aws.calls) == (0, calls)
    for call in range(1, calls + 1):
        for number, failure in enumerate(AWS_FAILURES):
            capsys.readouterr()
            aws, status = run(f"{call}-{number}", call, failure)
            out = capsys.readouterr().out
            assert status == 1, (call, failure)
            # Nothing follows the call that failed, and the annotation says what the CLI said.
            assert aws.calls == call, (call, failure)
            assert out.count("::error") == 1, (call, failure, out)
            assert (failure or "No such file or directory") in out, (call, failure, out)


def test_the_record_is_read_from_the_bucket_not_the_cdn(monkeypatch: pytest.MonkeyPatch) -> None:
    urls = _serve(monkeypatch, json.dumps({"commit": COMMIT, "ref": MASTER}).encode())

    assert public_record("builds/master") == Record(COMMIT, MASTER)
    assert urls == [
        "https://qgroundcontrol.s3.us-west-2.amazonaws.com/builds/master/published.json"
    ]


@pytest.mark.parametrize("status", [403, 404])
def test_a_directory_without_a_public_record_seems_to_hold_nothing(
    monkeypatch: pytest.MonkeyPatch, status: int
) -> None:
    # S3 answers 403 for a missing object of a bucket that cannot be listed, and for a private one.
    _serve(monkeypatch, _http_error(status))

    assert public_record("builds/master") is None


def test_only_a_release_tag_s_record_names_a_release() -> None:
    assert Record(TAG_COMMIT, TAG).release == "v5.1.5"
    for ref in ("", MASTER, "refs/heads/v5.1.5", "refs/tags/v5.2.0-dev", "v5.1.5"):
        assert Record(TAG_COMMIT, ref).release is None


@pytest.mark.parametrize(
    "answer",
    [
        _http_error(500),
        RuntimeError("Failed to fetch after 3 attempts"),
        b"<html>not json</html>",
        b"[]",
        b'{"commit": "master"}',
        b'{"ref": "refs/heads/master"}',
    ],
)
def test_an_unreadable_record_is_an_error_not_an_empty_directory(
    monkeypatch: pytest.MonkeyPatch, answer: bytes | Exception
) -> None:
    _serve(monkeypatch, answer)

    with pytest.raises(PublishedStateError):
        public_record("builds/master")


def test_aws_credentials_selects_oidc_or_static_keys_in_one_step() -> None:
    action = load_action("aws-credentials")
    assert "outputs" not in action
    (step,) = action["runs"]["steps"]
    assert step["uses"].startswith("aws-actions/configure-aws-credentials@")
    assert step["if"] == "inputs.aws-role-arn != '' || inputs.aws-key-id != ''"
    assert step["with"] == {
        "role-to-assume": "${{ inputs.aws-role-arn }}",
        "aws-access-key-id": "${{ inputs.aws-role-arn == '' && inputs.aws-key-id || '' }}",
        "aws-secret-access-key": (
            "${{ inputs.aws-role-arn == '' && inputs.aws-secret-access-key || '' }}"
        ),
        "aws-region": "${{ inputs.aws-region }}",
    }
    assert action["inputs"]["aws-region"]["default"] == "us-west-2"
