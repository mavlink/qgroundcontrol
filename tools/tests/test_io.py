"""Filesystem, serialization, hashing, and safe-extraction contracts."""

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


def test_json_and_toml_serialization(tmp_path: Path) -> None:
    target = tmp_path / "data.json"
    write_json(target, {"b": [2, 3], "a": 1}, indent=4, sort_keys=True)
    assert read_json(target) == {"a": 1, "b": [2, 3]}
    text = target.read_text()
    assert text.endswith("\n") and '    "a": 1' in text
    with pytest.raises(FileNotFoundError):
        read_json(tmp_path / "missing.json")

    toml = tmp_path / "data.toml"
    toml.write_text('name = "qgc"\nversion = 1\n')
    assert read_toml(toml) == {"name": "qgc", "version": 1}


def test_atomic_write_creates_overwrites_and_cleans_failed_temporary_files(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    target = tmp_path / "sub" / "file.txt"
    atomic_write(target, "first")
    atomic_write(target, "second")
    assert target.read_text() == "second"

    failed = tmp_path / "failed.txt"

    def fail_replace(*_args: object, **_kwargs: object) -> None:
        raise OSError("disk full")

    monkeypatch.setattr("os.replace", fail_replace)
    with pytest.raises(OSError, match="disk full"):
        atomic_write(failed, "content")
    assert not list(tmp_path.glob(".failed.txt.*"))


def test_write_if_changed_and_streaming_hash(tmp_path: Path) -> None:
    target = tmp_path / "generated" / "output.txt"
    assert write_text_if_changed(target, "first") is True
    assert write_text_if_changed(target, "first") is False
    assert write_text_if_changed(target, "second") is True
    assert target.read_text() == "second"

    payload = tmp_path / "payload.bin"
    payload.write_bytes(b"abc")
    assert sha256_file(payload, chunk_size=1) == (
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
    )


def test_archive_extraction_allows_nested_files_and_rejects_traversal(tmp_path: Path) -> None:
    unsafe_tar = tmp_path / "unsafe.tar"
    with tarfile.open(unsafe_tar, "w") as archive:
        info = tarfile.TarInfo("../escape.txt")
        info.size = 1
        archive.addfile(info, io.BytesIO(b"x"))
    with pytest.raises(tarfile.FilterError):
        extract_tar_data(unsafe_tar, tmp_path / "tar-output")

    unsafe_zip = tmp_path / "unsafe.zip"
    with zipfile.ZipFile(unsafe_zip, "w") as archive:
        archive.writestr("../escape.txt", "x")
    with pytest.raises(ValueError, match="Unsafe zip member"):
        extract_zip_safe(unsafe_zip, tmp_path / "zip-output")
    assert not (tmp_path / "escape.txt").exists()

    safe_zip = tmp_path / "safe.zip"
    with zipfile.ZipFile(safe_zip, "w") as archive:
        archive.writestr("dir/file.txt", "content")
    extract_zip_safe(safe_zip, tmp_path / "safe-output")
    assert (tmp_path / "safe-output" / "dir" / "file.txt").read_text() == "content"
