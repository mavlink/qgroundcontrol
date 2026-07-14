"""Artifact discovery CLI contracts."""

from __future__ import annotations

import sys
from typing import TYPE_CHECKING

import pytest
from find_artifact import main

if TYPE_CHECKING:
    from pathlib import Path


def _run(monkeypatch: pytest.MonkeyPatch, build_dir: Path, *arguments: str) -> int:
    monkeypatch.setattr(sys, "argv", ["prog", "--build-dir", str(build_dir), *arguments])
    return main()


def test_single_pattern_reports_found_and_handles_empty_or_missing_directories(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_output: Path
) -> None:
    assert _run(monkeypatch, tmp_path / "missing", "--pattern", "*.apk") == 0
    (tmp_path / "other.txt").write_text("x")
    assert _run(monkeypatch, tmp_path, "--pattern", "*.apk") == 0

    apk = tmp_path / "sub" / "app.apk"
    apk.parent.mkdir()
    apk.write_text("x")
    gh_output.write_text("")
    assert _run(monkeypatch, tmp_path, "--pattern", "*.apk") == 0
    output = gh_output.read_text()
    assert "found=true" in output and str(apk) in output


def test_named_matches_emit_each_found_or_empty_path(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_output: Path
) -> None:
    apk = tmp_path / "app.apk"
    dmg = tmp_path / "sub" / "qgc.dmg"
    apk.write_text("x")
    dmg.parent.mkdir()
    dmg.write_text("x")
    assert (
        _run(
            monkeypatch,
            tmp_path,
            "--match",
            "apk=*.apk",
            "--match",
            "dmg=*.dmg",
            "--match",
            "exe=*.exe",
        )
        == 0
    )
    output = gh_output.read_text()
    assert f"apk={apk}" in output and f"dmg={dmg}" in output and "exe=\n" in output

    gh_output.write_text("")
    assert _run(monkeypatch, tmp_path / "missing", "--match", "apk=*.apk") == 0
    assert "apk=\n" in gh_output.read_text()


def test_cli_rejects_invalid_or_conflicting_match_options(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    for arguments in (
        ("--match", "no-equals-sign"),
        ("--pattern", "*.apk", "--match", "apk=*.apk"),
    ):
        with pytest.raises(SystemExit):
            _run(monkeypatch, tmp_path, *arguments)


def test_ambiguous_patterns_fail_and_scoped_patterns_select_one(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_output: Path
) -> None:
    linux = tmp_path / "QGroundControl-linux" / "QGroundControl.apk"
    windows = tmp_path / "QGroundControl-windows" / "QGroundControl.apk"
    linux.parent.mkdir()
    windows.parent.mkdir()
    linux.touch()
    windows.touch()

    assert _run(monkeypatch, tmp_path, "--match", "apk=*.apk") == 1
    gh_output.write_text("")
    assert (
        _run(
            monkeypatch,
            tmp_path,
            "--match",
            "apk=QGroundControl-linux/*.apk",
        )
        == 0
    )
    assert f"apk={linux}" in gh_output.read_text()


def test_top_level_ignores_staging_copies(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_output: Path
) -> None:
    package = tmp_path / "QGroundControl.deb"
    staging = tmp_path / "_CPack_Packages" / "QGroundControl.deb"
    package.touch()
    staging.parent.mkdir()
    staging.touch()

    assert _run(monkeypatch, tmp_path, "--pattern", "*.deb", "--top-level") == 0
    output = gh_output.read_text()
    assert f"path={package}" in output
    assert str(staging) not in output


def test_required_artifacts_fail_when_directory_or_match_is_missing(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_output: Path
) -> None:
    assert _run(monkeypatch, tmp_path / "missing", "--pattern", "*.deb", "--required") == 1
    assert "found=false" in gh_output.read_text()

    gh_output.write_text("")
    assert _run(monkeypatch, tmp_path, "--pattern", "*.deb", "--required") == 1
    assert "found=false" in gh_output.read_text()

    package = tmp_path / "QGroundControl.deb"
    package.touch()
    gh_output.write_text("")
    assert _run(monkeypatch, tmp_path, "--pattern", "*.deb", "--required") == 0
    assert "found=true" in gh_output.read_text()


def test_required_named_matches_fail_if_any_artifact_is_missing(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_output: Path
) -> None:
    apk = tmp_path / "QGroundControl.apk"
    apk.touch()

    assert (
        _run(
            monkeypatch,
            tmp_path,
            "--match",
            "apk=*.apk",
            "--match",
            "dmg=*.dmg",
            "--required",
        )
        == 1
    )
    output = gh_output.read_text()
    assert f"apk={apk}" in output and "dmg=\n" in output
