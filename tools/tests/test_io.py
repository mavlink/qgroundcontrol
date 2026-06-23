#!/usr/bin/env python3
"""Tests for tools/common/io.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from common.io import atomic_write, read_json, read_toml, write_json

if TYPE_CHECKING:
    from pathlib import Path


def test_read_json_roundtrip(tmp_path: Path) -> None:
    target = tmp_path / "x.json"
    write_json(target, {"a": 1, "b": [2, 3]})
    assert read_json(target) == {"a": 1, "b": [2, 3]}


def test_write_json_indent_and_newline(tmp_path: Path) -> None:
    target = tmp_path / "x.json"
    write_json(target, {"k": "v"}, indent=4)
    text = target.read_text(encoding="utf-8")
    assert text.endswith("\n")
    assert '    "k": "v"' in text


def test_write_json_sort_keys(tmp_path: Path) -> None:
    target = tmp_path / "x.json"
    write_json(target, {"b": 2, "a": 1}, sort_keys=True)
    assert list(read_json(target).keys()) == ["a", "b"]


def test_read_json_missing_file_raises(tmp_path: Path) -> None:
    with pytest.raises(FileNotFoundError):
        read_json(tmp_path / "missing.json")


def test_read_toml(tmp_path: Path) -> None:
    target = tmp_path / "x.toml"
    target.write_text('name = "qgc"\nversion = 1\n', encoding="utf-8")
    assert read_toml(target) == {"name": "qgc", "version": 1}


def test_atomic_write_creates_file(tmp_path: Path) -> None:
    target = tmp_path / "sub" / "y.txt"
    atomic_write(target, "hello\n")
    assert target.read_text(encoding="utf-8") == "hello\n"


def test_atomic_write_overwrites(tmp_path: Path) -> None:
    target = tmp_path / "y.txt"
    target.write_text("old", encoding="utf-8")
    atomic_write(target, "new")
    assert target.read_text(encoding="utf-8") == "new"


def test_atomic_write_cleans_up_tmp_on_failure(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    target = tmp_path / "z.txt"

    def boom(*args: object, **kwargs: object) -> None:
        raise OSError("disk full")

    monkeypatch.setattr("os.replace", boom)
    with pytest.raises(OSError, match="disk full"):
        atomic_write(target, "x")
    assert not list(tmp_path.glob(".z.txt.*"))
