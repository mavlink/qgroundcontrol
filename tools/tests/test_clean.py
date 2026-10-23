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


def test_parse_args_rejects_cache_with_all() -> None:
    with pytest.raises(SystemExit):
        parse_args(["--cache", "--all"])


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


def test_clean_build_preserves_user_presets(tmp_path: Path) -> None:
    (tmp_path / "build").mkdir()
    (tmp_path / "build" / "f").write_text("x", encoding="utf-8")
    (tmp_path / "CMakeUserPresets.json").write_text("{}", encoding="utf-8")
    (tmp_path / "qtcreator.user").write_text("", encoding="utf-8")

    clean_build(tmp_path, dry_run=False)

    assert not (tmp_path / "build").exists()
    assert (tmp_path / "CMakeUserPresets.json").exists()
    assert not (tmp_path / "qtcreator.user").exists()


def test_clean_cache_removes_caches_and_keeps_clangd_config(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    (tmp_path / ".cache" / "clangd" / "index").mkdir(parents=True)
    (tmp_path / ".cache" / "ccache").mkdir()
    (tmp_path / ".ccache").mkdir()
    (tmp_path / ".clangd").write_text("CompileFlags: {}\n", encoding="utf-8")

    def unexpected_run(*_args: object, **_kwargs: object) -> None:
        raise AssertionError("clean_cache must not reset the user's global ccache statistics")

    monkeypatch.setattr("clean.subprocess.run", unexpected_run)
    clean_cache(tmp_path, dry_run=False)
    assert not (tmp_path / ".cache").exists()
    assert not (tmp_path / ".ccache").exists()
    assert (tmp_path / ".clangd").is_file()


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
def test_main_dry_run_exits_zero(
    flag: str, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    # repo_root() is hardcoded to tools/..; redirect by chdir-ing into a fake tree.
    fake_tools = tmp_path / "tools"
    fake_tools.mkdir()
    (tmp_path / "build").mkdir()
    monkeypatch.setattr("clean.repo_root", lambda: tmp_path)
    assert main(flag.split()) == 0
    assert (tmp_path / "build").exists()  # dry-run preserves


def test_main_all_removes_node_modules(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    for name in ("build", ".cache", "docs/node_modules/pkg", "node_modules/pkg/__pycache__"):
        (tmp_path / name).mkdir(parents=True)
    monkeypatch.setattr("clean.repo_root", lambda: tmp_path)
    monkeypatch.setattr("clean.report_disk_usage", lambda _root: None)

    assert main(["--all"]) == 0

    assert list(tmp_path.iterdir()) == [tmp_path / "docs"]
    assert not any((tmp_path / "docs").iterdir())


def test_clean_generated_skips_vcs_installed_packages_and_venvs(tmp_path: Path) -> None:
    owned = tmp_path / "tools" / "__pycache__"
    owned.mkdir(parents=True)
    protected = [
        tmp_path / ".git" / "hooks" / "__pycache__",
        tmp_path / "docs" / "node_modules" / "pkg" / "__pycache__",
        tmp_path / "tools" / ".venv" / "lib" / "site" / "__pycache__",
        tmp_path / "custom-env" / "lib" / "__pycache__",
    ]
    for path in protected:
        path.mkdir(parents=True)
        (path / "m.pyc").write_text("", encoding="utf-8")
    for venv in (tmp_path / "tools" / ".venv", tmp_path / "custom-env"):
        (venv / "pyvenv.cfg").write_text("home = /usr/bin\n", encoding="utf-8")

    clean_generated(tmp_path, dry_run=False)

    assert not owned.exists()
    assert all(path.exists() for path in protected)
