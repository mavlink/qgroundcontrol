"""Filesystem-cleanup contracts."""

from __future__ import annotations

from typing import TYPE_CHECKING

from clean import clean_build, clean_cache, clean_generated, main, parse_args, remove_path

if TYPE_CHECKING:
    from pathlib import Path

    import pytest


def test_arguments_and_dry_run_policy() -> None:
    defaults = parse_args([])
    assert (defaults.all, defaults.cache, defaults.dry_run) == (False, False, False)
    explicit = parse_args(["--all", "--dry-run"])
    assert (explicit.all, explicit.dry_run) == (True, True)


def test_remove_path_handles_files_directories_missing_and_dry_run(tmp_path: Path) -> None:
    preserved = tmp_path / "preserved"
    preserved.write_text("data")
    remove_path(preserved, "preserved", dry_run=True)
    assert preserved.exists()

    file = tmp_path / "file"
    directory = tmp_path / "dir"
    file.write_text("data")
    directory.mkdir()
    (directory / "child").write_text("data")
    for path in (file, directory, tmp_path / "missing"):
        remove_path(path, path.name, dry_run=False)
        assert not path.exists()


def test_cleanup_groups_remove_their_owned_artifacts(tmp_path: Path) -> None:
    for path in (tmp_path / "build", tmp_path / ".cache", tmp_path / ".ccache"):
        path.mkdir()
    for path in (tmp_path / "CMakeUserPresets.json", tmp_path / "qtcreator.user"):
        path.write_text("")
    pycache = tmp_path / "sub" / "__pycache__"
    pycache.mkdir(parents=True)
    (pycache / "x.pyc").write_text("")
    (tmp_path / "y.pyc").write_text("")

    clean_build(tmp_path, dry_run=False)
    clean_cache(tmp_path, dry_run=False)
    clean_generated(tmp_path, dry_run=False)
    for path in (
        tmp_path / "build",
        tmp_path / ".cache",
        tmp_path / ".ccache",
        tmp_path / "qtcreator.user",
        pycache,
        tmp_path / "y.pyc",
    ):
        assert not path.exists()
    assert (tmp_path / "CMakeUserPresets.json").exists()


def test_main_dry_run_variants_preserve_build(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    (tmp_path / "build").mkdir()
    monkeypatch.setattr("clean.find_repo_root", lambda _path: tmp_path)
    for args in (["--dry-run"], ["--all", "--dry-run"], ["--cache", "--dry-run"]):
        assert main(args) == 0
        assert (tmp_path / "build").exists()
