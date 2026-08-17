"""Tests for verify_package_contents.py."""

from __future__ import annotations

import subprocess
from typing import TYPE_CHECKING

from _helpers import completed
from verify_package_contents import find_violations, list_deb_paths, main

if TYPE_CHECKING:
    from pathlib import Path

_CLEAN_LISTING = """\
drwxr-xr-x root/root         0 2026-01-01 00:00 ./
drwxr-xr-x root/root         0 2026-01-01 00:00 ./usr/
drwxr-xr-x root/root         0 2026-01-01 00:00 ./usr/bin/
-rwxr-xr-x root/root   1234567 2026-01-01 00:00 ./usr/bin/QGroundControl
drwxr-xr-x root/root         0 2026-01-01 00:00 ./usr/lib/
-rw-r--r-- root/root      4321 2026-01-01 00:00 ./usr/lib/libgstreamer.so.1
"""

_DIRTY_LISTING = """\
drwxr-xr-x root/root         0 2026-01-01 00:00 ./usr/include/
-rw-r--r-- root/root       100 2026-01-01 00:00 ./usr/include/expat.h
-rw-r--r-- root/root      2000 2026-01-01 00:00 ./usr/lib/libexpat.a
-rw-r--r-- root/root       200 2026-01-01 00:00 ./usr/lib/pkgconfig/expat.pc
-rw-r--r-- root/root       300 2026-01-01 00:00 ./usr/lib/cmake/expat-2.7.5/expat-config.cmake
lrwxrwxrwx root/root         0 2026-01-01 00:00 ./usr/lib/libexpat.so -> libexpat.so.1
"""


def test_list_deb_paths_strips_leading_dot_slash(monkeypatch) -> None:
    monkeypatch.setattr(subprocess, "run", lambda *a, **kw: completed(_CLEAN_LISTING))
    paths = list_deb_paths(__file__)  # path itself unused by the stub
    assert "usr/bin/QGroundControl" in paths
    assert all(not p.startswith("./") for p in paths)


def test_list_deb_paths_resolves_symlink_target(monkeypatch) -> None:
    monkeypatch.setattr(subprocess, "run", lambda *a, **kw: completed(_DIRTY_LISTING))
    paths = list_deb_paths(__file__)
    assert "usr/lib/libexpat.so" in paths
    assert "usr/lib/libexpat.so -> libexpat.so.1" not in paths


def test_find_violations_clean_package() -> None:
    paths = ["usr/bin/QGroundControl", "usr/lib/libgstreamer.so.1", "usr/share/doc/copyright"]
    assert find_violations(paths) == []


def test_find_violations_flags_dev_payload() -> None:
    paths = [
        "usr/include",
        "usr/include/expat.h",
        "usr/lib/libexpat.a",
        "usr/lib/libfoo.la",
        "usr/lib/pkgconfig/expat.pc",
        "usr/lib/cmake/expat-2.7.5/expat-config.cmake",
        "usr/bin/QGroundControl",
    ]
    violations = find_violations(paths)
    flagged = {path for path, _ in violations}
    assert flagged == {
        "usr/include",
        "usr/include/expat.h",
        "usr/lib/libexpat.a",
        "usr/lib/libfoo.la",
        "usr/lib/pkgconfig/expat.pc",
        "usr/lib/cmake/expat-2.7.5/expat-config.cmake",
    }


def test_main_fails_on_missing_package(tmp_path: Path, monkeypatch) -> None:
    monkeypatch.setattr("sys.argv", ["prog", "--package-path", str(tmp_path / "nope.deb")])
    assert main() == 1


def test_main_passes_clean_package(tmp_path: Path, monkeypatch) -> None:
    deb = tmp_path / "qgc.deb"
    deb.write_bytes(b"")
    monkeypatch.setattr(subprocess, "run", lambda *a, **kw: completed(_CLEAN_LISTING))
    monkeypatch.setattr("sys.argv", ["prog", "--package-path", str(deb)])
    assert main() == 0


def test_main_fails_on_dirty_package(tmp_path: Path, monkeypatch, capsys) -> None:
    deb = tmp_path / "qgc.deb"
    deb.write_bytes(b"")
    monkeypatch.setattr(subprocess, "run", lambda *a, **kw: completed(_DIRTY_LISTING))
    monkeypatch.setattr("sys.argv", ["prog", "--package-path", str(deb)])
    assert main() == 1
    out = capsys.readouterr().out
    assert "usr/include/expat.h" in out
    assert "usr/lib/libexpat.a" in out
