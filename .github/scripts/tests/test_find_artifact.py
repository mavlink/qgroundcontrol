"""Tests for find_artifact.py."""

from __future__ import annotations

from pathlib import Path

from find_artifact import main


def test_missing_dir(tmp_path: Path, monkeypatch) -> None:
    monkeypatch.setattr("sys.argv", ["prog", "--build-dir", str(tmp_path / "nope"), "--pattern", "*.apk"])
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
