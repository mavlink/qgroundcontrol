"""Tests for tools/ci/publish/release_notes.py and tools/configs/cliff.toml."""

from __future__ import annotations

import subprocess
from typing import TYPE_CHECKING

import pytest
from ci.publish import release_notes

from .._helpers import git

if TYPE_CHECKING:
    from pathlib import Path

GIT_CLIFF = release_notes.find_git_cliff()
needs_git_cliff = pytest.mark.skipif(
    GIT_CLIFF is None, reason="git-cliff is not installed (tools/pyproject.toml `release` group)"
)


def _commit(repo: Path, *message: str) -> None:
    args = [part for line in message for part in ("-m", line)]
    git(repo, "commit", "-q", "--allow-empty", *args)


@pytest.fixture(scope="module")
def history(tmp_path_factory: pytest.TempPathFactory) -> Path:
    """v1.0.0 on master, v1.0.1 and v1.0.2 on Stable_V1.0, and the v1.1.0-dev marker on master."""
    repo = tmp_path_factory.mktemp("history")
    git(repo, "init", "-q", "-b", "master")
    _commit(repo, "chore: initial import")
    _commit(repo, "feat(Core): first feature")
    git(repo, "tag", "-a", "v1.0.0", "-m", "v1.0.0")

    git(repo, "checkout", "-q", "-b", "Stable_V1.0")
    for message in (
        ("fix(Comms): repair link (#45)",),
        ("Fix lua script list",),
        ("feat(Vehicle): add thing (#123) (#130)",),
        ("perf(Map): faster tiles",),
        ('Revert "feat(UI): bad idea"',),
        ("docs: words",),
        ("style: format",),
        ("chore: tidy",),
        ("refactor: rename",),
        ("test(Vehicle): cover it",),
        ("build(cmake): flags",),
        ("ci: cache",),
        ("deps: bump foo",),
        ("Merge branch 'topic' into Stable_V1.0",),
        ("refactor(API)!: rename settings keys",),
        ("feat(UI): new palette", "BREAKING CHANGE: colors moved"),
        ("fix: second fix",),
    ):
        _commit(repo, *message)
        if message[0] == "perf(Map): faster tiles":
            git(repo, "tag", "-a", "v1.0.1-rc1", "-m", "v1.0.1-rc1")
    git(repo, "tag", "-a", "v1.0.1", "-m", "v1.0.1")
    _commit(repo, "docs: patch notes")
    git(repo, "tag", "-a", "v1.0.2", "-m", "v1.0.2")

    git(repo, "checkout", "-q", "master")
    _commit(repo, "feat(Map): master feature")
    git(repo, "tag", "-a", "v1.1.0-dev", "-m", "v1.1.0 daily version base")
    _commit(repo, "fix(Map): after the marker")
    return repo


@pytest.fixture
def in_history(history: Path, isolated_git: None, monkeypatch: pytest.MonkeyPatch) -> Path:
    monkeypatch.chdir(history)
    return history


@needs_git_cliff
def test_release_notes_group_full_subjects_since_the_previous_release(
    in_history: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    assert release_notes.main(["v1.0.1"]) == 0
    assert capsys.readouterr().out == (
        "## Changes in v1.0.1\n"
        "\n"
        "### Features\n"
        "\n"
        "- **BREAKING** feat(UI): new palette\n"
        "- feat(Vehicle): add thing\n"
        "\n"
        "### Bug Fixes\n"
        "\n"
        "- fix: second fix\n"
        "- fix(Comms): repair link\n"
        "\n"
        "### Performance\n"
        "\n"
        "- perf(Map): faster tiles\n"
        "\n"
        "### Reverts\n"
        "\n"
        '- Revert "feat(UI): bad idea"\n'
        "\n"
        "### Other Changes\n"
        "\n"
        "- **BREAKING** refactor(API)!: rename settings keys\n"
        "- Fix lua script list\n"
    )


@needs_git_cliff
def test_unreleased_notes_skip_version_markers(
    in_history: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    assert release_notes.main(["v1.1.0"]) == 0
    assert capsys.readouterr().out == (
        "## Changes in v1.1.0\n"
        "\n"
        "### Features\n"
        "\n"
        "- feat(Map): master feature\n"
        "\n"
        "### Bug Fixes\n"
        "\n"
        "- fix(Map): after the marker\n"
    )


@needs_git_cliff
def test_config_tag_pattern_ignores_dev_and_rc_tags(in_history: Path) -> None:
    def cliff(*args: str) -> str:
        assert GIT_CLIFF is not None
        config = str(release_notes.CLIFF_CONFIG)
        command = [GIT_CLIFF, "--config", config, *args]
        return subprocess.run(command, capture_output=True, text=True, check=True).stdout

    master = cliff("--tag", "v1.1.0", "v1.0.0..master")
    assert "## Changes in v1.1.0\n" in master
    assert "- feat(Map): master feature\n" in master
    assert "-dev" not in master
    stable = cliff("v1.0.0..Stable_V1.0")
    assert stable.count("## Changes in") == 1
    assert "## Changes in v1.0.1\n" in stable
    assert "-rc1" not in stable


@needs_git_cliff
@pytest.mark.parametrize(
    ("tag", "notes"),
    [
        ("v1.0.0", "### Features\n\n- feat(Core): first feature\n"),
        ("v1.0.2", "No notable changes.\n"),
    ],
)
def test_first_and_empty_releases(
    in_history: Path, capsys: pytest.CaptureFixture[str], tag: str, notes: str
) -> None:
    assert release_notes.main([tag]) == 0
    assert capsys.readouterr().out == f"## Changes in {tag}\n\n{notes}"


def test_previous_release_tag_compares_versions_numerically(
    git_repo: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    for tag in ("v1.9.0", "v1.10.0", "v1.10.1-rc1", "v2.0.0-dev", "v1.11.0"):
        _commit(git_repo, tag)
        git(git_repo, "tag", tag)
    _commit(git_repo, "fix: head")
    monkeypatch.chdir(git_repo)
    assert release_notes.previous_release_tag("v1.11.0", "v1.11.0") == "v1.10.0"
    assert release_notes.previous_release_tag("v1.12.0", "HEAD") == "v1.11.0"
    assert release_notes.previous_release_tag("v1.9.0", "HEAD") is None


@pytest.mark.parametrize(
    ("previous", "released", "tail"),
    [
        ("v1.0.0", True, ["v1.0.0..v1.0.1"]),
        (None, True, ["--latest"]),
        ("v1.0.0", False, ["--unreleased", "--tag", "v1.0.1", "v1.0.0..HEAD"]),
        (None, False, ["--unreleased", "--tag", "v1.0.1"]),
    ],
)
def test_cliff_command_renders_exactly_one_release(
    previous: str | None, released: bool, tail: list[str]
) -> None:
    command = release_notes.cliff_command("git-cliff", "v1.0.1", previous, released=released)
    config = str(release_notes.CLIFF_CONFIG)
    assert command == ["git-cliff", "--config", config, "--tag-pattern", r"^v1\.0\.1$", *tail]


class FakeGh:
    def __init__(self, view: subprocess.CompletedProcess[str], create_code: int = 0) -> None:
        self.view = view
        self.create_code = create_code
        self.calls: list[tuple[str, ...]] = []

    def __call__(self, *args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
        assert not check
        self.calls.append(args)
        if args[:2] == ("release", "view"):
            return self.view
        return subprocess.CompletedProcess(
            ["gh", *args], self.create_code, "https://example.invalid/v1.0.1\n", "boom"
        )


def _view(code: int, stdout: str = "", stderr: str = "") -> subprocess.CompletedProcess[str]:
    return subprocess.CompletedProcess(["gh"], code, stdout, stderr)


@pytest.fixture
def fake_cliff(monkeypatch: pytest.MonkeyPatch) -> list[list[str]]:
    commands: list[list[str]] = []

    def run(command: list[str]) -> subprocess.CompletedProcess[str]:
        commands.append(command)
        return subprocess.CompletedProcess(command, 0, "\n## Changes in v1.0.1\n\n- fix: x\n\n", "")

    monkeypatch.setattr(release_notes, "find_git_cliff", lambda: "git-cliff")
    monkeypatch.setattr(release_notes, "run_captured", run)
    return commands


def test_draft_creates_a_release_when_none_exists(
    in_history: Path,
    fake_cliff: list[list[str]],
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    gh_summary: Path,
) -> None:
    fake_gh = FakeGh(_view(1, stderr="release not found\n"))
    monkeypatch.setattr(release_notes, "gh", fake_gh)
    notes = tmp_path / "notes.md"

    assert release_notes.main(["v1.0.1", "--output", str(notes), "--draft"]) == 0

    assert fake_cliff[0][-1] == "v1.0.0..v1.0.1"
    assert notes.read_text(encoding="utf-8") == "## Changes in v1.0.1\n\n- fix: x\n"
    assert gh_summary.read_text(encoding="utf-8") == (
        "### Release notes\n\n| Item | Value |\n| --- | --- |\n"
        "| Tag | `v1.0.1` |\n| Tag pushed | yes |\n\n#### Changes in v1.0.1\n\n- fix: x\n\n"
    )
    assert fake_gh.calls == [
        ("release", "view", "v1.0.1", "--json", "isDraft,url"),
        (
            "release",
            "create",
            "v1.0.1",
            "--draft",
            "--verify-tag",
            "--title",
            "v1.0.1",
            "--notes-file",
            str(notes),
        ),
    ]
    assert capsys.readouterr().out.endswith(
        "Created draft release v1.0.1: https://example.invalid/v1.0.1\n"
    )


@pytest.mark.parametrize("draft", [True, False])
def test_draft_leaves_an_existing_release_unchanged(
    in_history: Path,
    fake_cliff: list[list[str]],
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    gh_summary: Path,
    draft: bool,
) -> None:
    view = _view(0, stdout=f'{{"isDraft": {str(draft).lower()}, "url": "https://x/v1.0.1"}}')
    fake_gh = FakeGh(view)
    monkeypatch.setattr(release_notes, "gh", fake_gh)

    assert release_notes.main(["v1.0.1", "--output", str(tmp_path / "n.md"), "--draft"]) == 0

    assert [call[1] for call in fake_gh.calls] == ["view"]
    state = "draft" if draft else "published"
    assert f"::notice::A {state} v1.0.1 release already exists; left unchanged" in (
        capsys.readouterr().out
    )
    assert "\n#### Changes in v1.0.1\n" in gh_summary.read_text(encoding="utf-8")


@pytest.mark.parametrize(
    ("view", "create_code", "error"),
    [
        (_view(1, stderr="HTTP 502"), 0, "Cannot check for an existing v1.0.1 release: HTTP 502"),
        (_view(1, stderr="release not found"), 1, "Cannot create the draft v1.0.1 release: boom"),
    ],
)
def test_draft_fails_when_gh_fails(
    in_history: Path,
    fake_cliff: list[list[str]],
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    view: subprocess.CompletedProcess[str],
    create_code: int,
    error: str,
) -> None:
    monkeypatch.setattr(release_notes, "gh", FakeGh(view, create_code))
    assert release_notes.main(["v1.0.1", "--output", str(tmp_path / "n.md"), "--draft"]) == 1
    assert f"::error::{error}" in capsys.readouterr().out


@pytest.mark.parametrize(
    ("argv", "error"),
    [
        (["v1.1.0-dev"], "v1.1.0-dev is not a vX.Y.Z release tag"),
        (["v9.9.9", "--output", "n.md", "--draft"], "Tag v9.9.9 does not exist"),
    ],
)
def test_rejects_tags_it_cannot_draft(
    in_history: Path,
    fake_cliff: list[list[str]],
    capsys: pytest.CaptureFixture[str],
    argv: list[str],
    error: str,
) -> None:
    assert release_notes.main(argv) == 1
    assert capsys.readouterr().out.startswith(f"::error::{error}")
    assert fake_cliff == []


def test_draft_requires_an_output_file(in_history: Path) -> None:
    with pytest.raises(SystemExit) as exc:
        release_notes.main(["v1.0.1", "--draft"])
    assert exc.value.code == 2


def test_reports_missing_or_failing_git_cliff(
    in_history: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    monkeypatch.setattr(release_notes, "find_git_cliff", lambda: None)
    assert release_notes.main(["v1.0.1"]) == 1
    assert "install_python.py release" in capsys.readouterr().out

    monkeypatch.setattr(release_notes, "find_git_cliff", lambda: "git-cliff")
    monkeypatch.setattr(
        release_notes,
        "run_captured",
        lambda command: subprocess.CompletedProcess(command, 1, "", "bad template"),
    )
    assert release_notes.main(["v1.0.1"]) == 1
    assert capsys.readouterr().out == (
        "::error::Cannot generate release notes for v1.0.1: bad template\n"
    )
