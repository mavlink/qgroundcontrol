"""Artifact discovery CLI contracts."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from ci.packaging.find_artifact import main

if TYPE_CHECKING:
    from pathlib import Path


def _run(build_dir: Path, *arguments: str) -> int:
    return main(["--build-dir", str(build_dir), *arguments])


def test_pattern_emits_the_unique_recursive_match(tmp_path: Path, gh_output: Path) -> None:
    apk = tmp_path / "sub" / "app.apk"
    apk.parent.mkdir()
    apk.write_text("x")
    (tmp_path / "other.txt").write_text("x")

    assert _run(tmp_path, "--pattern", "*.apk") == 0
    assert gh_output.read_text() == f"path={apk}\n"


def test_missing_directory_or_match_fails(tmp_path: Path, gh_output: Path) -> None:
    assert _run(tmp_path / "missing", "--pattern", "*.deb") == 1
    (tmp_path / "other.txt").write_text("x")
    assert _run(tmp_path, "--pattern", "*.deb") == 1
    assert gh_output.read_text() == ""


def test_ambiguous_pattern_fails_until_narrowed(tmp_path: Path, gh_output: Path) -> None:
    linux = tmp_path / "QGroundControl-linux" / "QGroundControl.apk"
    windows = tmp_path / "QGroundControl-windows" / "QGroundControl.apk"
    linux.parent.mkdir()
    windows.parent.mkdir()
    linux.touch()
    windows.touch()

    assert _run(tmp_path, "--pattern", "*.apk") == 1
    assert gh_output.read_text() == ""

    assert _run(tmp_path, "--pattern", "QGroundControl-linux/*.apk") == 0
    assert gh_output.read_text() == f"path={linux}\n"


def test_top_level_ignores_staging_copies(tmp_path: Path, gh_output: Path) -> None:
    package = tmp_path / "QGroundControl.deb"
    staging = tmp_path / "_CPack_Packages" / "QGroundControl.deb"
    package.touch()
    staging.parent.mkdir()
    staging.touch()

    assert _run(tmp_path, "--pattern", "*.deb", "--top-level") == 0
    assert gh_output.read_text() == f"path={package}\n"


def test_copy_to_copies_the_single_top_level_match(tmp_path: Path, gh_output: Path) -> None:
    apk_dir = tmp_path / "android-build"
    nested = apk_dir / "build" / "outputs" / "intermediate.apk"
    nested.parent.mkdir(parents=True)
    nested.write_text("intermediate")
    apk = apk_dir / "android-build-release-signed.apk"
    apk.write_text("signed")
    destination = tmp_path / "QGroundControl.apk"
    arguments = ("--pattern", "*.apk", "--top-level", "--copy-to", str(destination))

    assert _run(apk_dir, *arguments) == 0
    assert destination.read_text() == "signed"
    assert f"path={apk}" in gh_output.read_text()

    destination.unlink()
    (apk_dir / "android-build-debug.apk").write_text("debug")
    assert _run(apk_dir, *arguments) == 1
    assert not destination.exists()

    assert _run(tmp_path / "missing", *arguments) == 1
    assert not destination.exists()


def test_pattern_is_required(tmp_path: Path) -> None:
    with pytest.raises(SystemExit):
        _run(tmp_path)
