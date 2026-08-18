#!/usr/bin/env python3
"""Tests for tools/common/io.py."""

from __future__ import annotations

import io
import tarfile
import zipfile
from typing import TYPE_CHECKING

import pytest
from common.io import (
    atomic_write,
    extract_tar_data,
    extract_zip_safe,
    read_json,
    read_toml,
    sha256_file,
    write_json,
    write_text_if_changed,
)

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


def test_atomic_write_cleans_up_tmp_on_failure(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    target = tmp_path / "z.txt"

    def boom(*args: object, **kwargs: object) -> None:
        raise OSError("disk full")

    monkeypatch.setattr("os.replace", boom)
    with pytest.raises(OSError, match="disk full"):
        atomic_write(target, "x")
    assert not list(tmp_path.glob(".z.txt.*"))


def test_write_text_if_changed_and_sha256_file(tmp_path: Path) -> None:
    target = tmp_path / "generated" / "output.txt"
    assert write_text_if_changed(target, "first") is True
    assert write_text_if_changed(target, "first") is False
    assert write_text_if_changed(target, "second") is True
    assert target.read_text(encoding="utf-8") == "second"

    payload = tmp_path / "payload.bin"
    payload.write_bytes(b"abc")
    assert sha256_file(payload, chunk_size=1) == (
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
    )
    for invalid_size in (0, -1):
        with pytest.raises(ValueError, match="chunk_size must be a positive integer"):
            sha256_file(payload, chunk_size=invalid_size)


def test_extract_tar_data_rejects_traversal(tmp_path: Path) -> None:
    archive_path = tmp_path / "unsafe.tar"
    with tarfile.open(archive_path, "w") as archive:
        info = tarfile.TarInfo("../escape.txt")
        info.size = 1
        archive.addfile(info, io.BytesIO(b"x"))

    with pytest.raises(tarfile.FilterError):
        extract_tar_data(archive_path, tmp_path / "output")
    assert not (tmp_path / "escape.txt").exists()


def test_extract_zip_safe_allows_nested_files_and_rejects_traversal(tmp_path: Path) -> None:
    unsafe_archive = tmp_path / "unsafe.zip"
    with zipfile.ZipFile(unsafe_archive, "w") as archive:
        archive.writestr("../escape.txt", "x")
    with pytest.raises(ValueError, match="Unsafe zip member"):
        extract_zip_safe(unsafe_archive, tmp_path / "unsafe-output")
    assert not (tmp_path / "escape.txt").exists()

    safe_archive = tmp_path / "safe.zip"
    with zipfile.ZipFile(safe_archive, "w") as archive:
        archive.writestr("dir/file.txt", "content")
    extract_zip_safe(safe_archive, tmp_path / "safe-output")
    assert (tmp_path / "safe-output" / "dir" / "file.txt").read_text(encoding="utf-8") == "content"
