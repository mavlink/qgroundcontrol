"""Tests for tools/release.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
import release

if TYPE_CHECKING:
    from pathlib import Path


@pytest.fixture
def version_file(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    path = tmp_path / ".release-version"
    monkeypatch.setattr(release, "VERSION_FILE", path)
    monkeypatch.setattr(release, "check_node", lambda: 24)
    monkeypatch.setenv("GITHUB_TOKEN", "token")
    return path


def test_release_outputs_report_no_release_without_version_file(
    tmp_path: Path, gh_output: Path
) -> None:
    assert release.write_release_outputs(tmp_path / "missing") == 0
    assert gh_output.read_text(encoding="utf-8") == "new_release=false\n"


def test_release_outputs_strip_all_whitespace_from_version(tmp_path: Path, gh_output: Path) -> None:
    version_file = tmp_path / ".release-version"
    version_file.write_text(" 5.1.0\r\n", encoding="utf-8")

    assert release.write_release_outputs(version_file) == 0
    assert gh_output.read_text(encoding="utf-8") == "new_release=true\nversion=5.1.0\n"


def test_release_outputs_reject_empty_version_file(
    tmp_path: Path, gh_output: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    version_file = tmp_path / ".release-version"
    version_file.write_text(" \n", encoding="utf-8")

    assert release.write_release_outputs(version_file) == 1
    assert gh_output.read_text(encoding="utf-8") == ""
    assert "::error::semantic-release wrote an empty version file" in capsys.readouterr().out


def test_github_output_clears_stale_version_before_running(
    version_file: Path, gh_output: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    version_file.write_text("1.0.0\n", encoding="utf-8")
    seen: list[bool] = []

    def run(*, dry_run: bool) -> int:
        assert not dry_run
        seen.append(version_file.exists())
        return 0

    monkeypatch.setattr(release, "run_semantic_release", run)

    assert release.main(["--run", "--github-output"]) == 0
    assert seen == [False]
    assert gh_output.read_text(encoding="utf-8") == "new_release=false\n"


def test_github_output_reports_published_version(
    version_file: Path, gh_output: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    def run(*, dry_run: bool) -> int:
        version_file.write_text("5.1.0\n", encoding="utf-8")
        return 0

    monkeypatch.setattr(release, "run_semantic_release", run)

    assert release.main(["--run", "--github-output"]) == 0
    assert gh_output.read_text(encoding="utf-8") == "new_release=true\nversion=5.1.0\n"


def test_failed_release_writes_no_outputs(
    version_file: Path, gh_output: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    def run(*, dry_run: bool) -> int:
        version_file.write_text("5.1.0\n", encoding="utf-8")
        return 2

    monkeypatch.setattr(release, "run_semantic_release", run)

    assert release.main(["--run", "--github-output"]) == 2
    assert gh_output.read_text(encoding="utf-8") == ""


def test_github_output_requires_github_output_env(
    version_file: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.delenv("GITHUB_OUTPUT", raising=False)
    monkeypatch.setattr(
        release, "run_semantic_release", lambda **_: pytest.fail("semantic-release ran")
    )

    assert release.main(["--run", "--github-output"]) == 1
