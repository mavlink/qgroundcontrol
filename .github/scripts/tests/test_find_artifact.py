"""Tests for find_artifact.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from find_artifact import main

if TYPE_CHECKING:
    from pathlib import Path


def test_missing_dir(tmp_path: Path, monkeypatch) -> None:
    monkeypatch.setattr(
        "sys.argv", ["prog", "--build-dir", str(tmp_path / "nope"), "--pattern", "*.apk"]
    )
    assert main() == 0


def test_no_match(tmp_path: Path, monkeypatch) -> None:
    (tmp_path / "other.txt").write_text("x")
    monkeypatch.setattr("sys.argv", ["prog", "--build-dir", str(tmp_path), "--pattern", "*.apk"])
    assert main() == 0


def test_finds_artifact(tmp_path: Path, monkeypatch) -> None:
    apk = tmp_path / "sub" / "app.apk"
    apk.parent.mkdir()
    apk.write_text("x")
    output_file = tmp_path / "gh_output"
    output_file.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    monkeypatch.setattr("sys.argv", ["prog", "--build-dir", str(tmp_path), "--pattern", "*.apk"])
    assert main() == 0
    output = output_file.read_text()
    assert "found=true" in output
    assert str(apk) in output


def test_match_finds_subset(tmp_path: Path, monkeypatch) -> None:
    apk = tmp_path / "app.apk"
    apk.write_text("x")
    dmg = tmp_path / "sub" / "qgc.dmg"
    dmg.parent.mkdir()
    dmg.write_text("x")
    output_file = tmp_path / "gh_output"
    output_file.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    monkeypatch.setattr(
        "sys.argv",
        [
            "prog",
            "--build-dir",
            str(tmp_path),
            "--match",
            "apk=*.apk",
            "--match",
            "dmg=*.dmg",
            "--match",
            "exe=*.exe",
        ],
    )
    assert main() == 0
    output = output_file.read_text()
    assert f"apk={apk}" in output
    assert f"dmg={dmg}" in output
    assert "exe=\n" in output


def test_match_missing_dir(tmp_path: Path, monkeypatch) -> None:
    output_file = tmp_path / "gh_output"
    output_file.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
    monkeypatch.setattr(
        "sys.argv",
        ["prog", "--build-dir", str(tmp_path / "nope"), "--match", "apk=*.apk"],
    )
    assert main() == 0
    assert "apk=\n" in output_file.read_text()


def test_match_rejects_bad_spec(tmp_path: Path, monkeypatch) -> None:
    monkeypatch.setattr(
        "sys.argv",
        ["prog", "--build-dir", str(tmp_path), "--match", "no-equals-sign"],
    )
    with pytest.raises(SystemExit):
        main()


def test_pattern_and_match_mutually_exclusive(tmp_path: Path, monkeypatch) -> None:
    monkeypatch.setattr(
        "sys.argv",
        ["prog", "--build-dir", str(tmp_path), "--pattern", "*.apk", "--match", "apk=*.apk"],
    )
    with pytest.raises(SystemExit):
        main()
