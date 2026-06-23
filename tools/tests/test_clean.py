#!/usr/bin/env python3
"""Tests for tools/clean.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from clean import (
    clean_build,
    clean_cache,
    clean_generated,
    main,
    parse_args,
    remove_path,
)

if TYPE_CHECKING:
    from pathlib import Path


def test_parse_args_defaults() -> None:
    args = parse_args([])
    assert args.all is False
    assert args.cache is False
    assert args.dry_run is False


def test_parse_args_flags() -> None:
    args = parse_args(["--all", "--dry-run"])
    assert args.all is True
    assert args.dry_run is True


def test_remove_path_dry_run_keeps_file(tmp_path: Path) -> None:
    target = tmp_path / "x"
    target.write_text("data", encoding="utf-8")
    remove_path(target, "x", dry_run=True)
    assert target.exists()


def test_remove_path_deletes_file(tmp_path: Path) -> None:
    target = tmp_path / "x"
    target.write_text("data", encoding="utf-8")
    remove_path(target, "x", dry_run=False)
    assert not target.exists()


def test_remove_path_deletes_directory(tmp_path: Path) -> None:
    target = tmp_path / "dir"
    target.mkdir()
    (target / "child").write_text("y", encoding="utf-8")
    remove_path(target, "dir", dry_run=False)
    assert not target.exists()


def test_remove_path_missing_is_noop(tmp_path: Path) -> None:
    remove_path(tmp_path / "missing", "missing", dry_run=False)


def test_clean_build_removes_targets(tmp_path: Path) -> None:
    (tmp_path / "build").mkdir()
    (tmp_path / "build" / "f").write_text("x", encoding="utf-8")
    (tmp_path / "CMakeUserPresets.json").write_text("{}", encoding="utf-8")
    (tmp_path / "qtcreator.user").write_text("", encoding="utf-8")

    clean_build(tmp_path, dry_run=False)

    assert not (tmp_path / "build").exists()
    assert not (tmp_path / "CMakeUserPresets.json").exists()
    assert not (tmp_path / "qtcreator.user").exists()


def test_clean_cache_removes_caches(tmp_path: Path) -> None:
    (tmp_path / ".cache").mkdir()
    (tmp_path / ".ccache").mkdir()
    clean_cache(tmp_path, dry_run=False)
    assert not (tmp_path / ".cache").exists()
    assert not (tmp_path / ".ccache").exists()


def test_clean_generated_removes_pycache(tmp_path: Path) -> None:
    pycache = tmp_path / "sub" / "__pycache__"
    pycache.mkdir(parents=True)
    (pycache / "x.pyc").write_text("", encoding="utf-8")
    pyc = tmp_path / "y.pyc"
    pyc.write_text("", encoding="utf-8")

    clean_generated(tmp_path, dry_run=False)

    assert not pycache.exists()
    assert not pyc.exists()


@pytest.mark.parametrize("flag", ["--dry-run", "--all --dry-run", "--cache --dry-run"])
def test_main_dry_run_exits_zero(flag: str, tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    # repo_root() is hardcoded to tools/..; redirect by chdir-ing into a fake tree.
    fake_tools = tmp_path / "tools"
    fake_tools.mkdir()
    (tmp_path / "build").mkdir()
    monkeypatch.setattr("clean.repo_root", lambda: tmp_path)
    assert main(flag.split()) == 0
    assert (tmp_path / "build").exists()  # dry-run preserves
