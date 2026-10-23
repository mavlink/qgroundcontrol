"""Qt kit discovery for host tools such as ``qt-cmake`` and ``lupdate``."""

from __future__ import annotations

import os
import re
from glob import glob
from pathlib import Path

from .platform import host_arch, is_linux, is_macos, is_windows

__all__ = ["find_qt_tool", "qt_version_key"]

_VERSION_IN_PATH = re.compile(r"[\\/](\d+\.\d+\.\d+)[\\/]")


def qt_version_key(path: Path) -> tuple[int, ...]:
    """Return the Qt version in an install path such as ``~/Qt/6.10.1/gcc_64/bin`` (``(0,)`` if none).

    Stdlib-only: CI runs cmake_helper.py through `python3`, which on Windows is the runner's interpreter.
    """
    match = _VERSION_IN_PATH.search(f"{path}/")
    return tuple(int(part) for part in match.group(1).split(".")) if match else (0,)


def _kit_bin_patterns(version: str) -> list[Path]:
    """Return glob patterns for standard Qt installer kit ``bin`` directories, by priority."""
    if is_windows():
        kit = "msvc*_arm64" if host_arch() == "aarch64" else "msvc*_64"
        return [base / version / kit / "bin" for base in (Path("C:/Qt"), Path.home() / "Qt")]
    if is_macos():
        return [
            base / version / kit / "bin"
            for base in (Path.home() / "Qt", Path("/Applications/Qt"))
            for kit in ("macos", "clang_64")
        ]
    kit = "gcc_arm64" if host_arch() == "aarch64" else "gcc_64"
    return [base / version / kit / "bin" for base in (Path.home() / "Qt", Path("/opt/Qt"))]


def _executable_in(bin_dir: Path, name: str) -> Path | None:
    # On Windows qt-cmake ships as .bat beside an extensionless bash script.
    for suffix in (".exe", ".bat", "") if is_windows() else ("",):
        candidate = bin_dir / f"{name}{suffix}"
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return candidate
    return None


def find_qt_tool(name: str, *, version: str = "*", qt_root: Path | None = None) -> Path | None:
    """Find Qt host tool *name*, or None.

    Searches *qt_root*, then ``$QT_ROOT_DIR``, then standard installer kits (``~/Qt``, plus
    ``C:/Qt``, ``/Applications/Qt`` or ``/opt/Qt``) for *version*, newest first per base. An
    unpinned *version* (``"*"``) also accepts the Linux distribution kit in ``/usr/lib/qt6``.
    """
    env_root = os.environ.get("QT_ROOT_DIR")
    for root in (qt_root, Path(env_root) if env_root else None):
        if root is not None and (tool := _executable_in(root / "bin", name)):
            return tool

    bin_dirs: list[Path] = []
    for pattern in _kit_bin_patterns(version):
        bin_dirs.extend(sorted(map(Path, glob(str(pattern))), key=qt_version_key, reverse=True))
    if version == "*" and is_linux():
        bin_dirs.append(Path("/usr/lib/qt6/bin"))
    for bin_dir in bin_dirs:
        if tool := _executable_in(bin_dir, name):
            return tool
    return None
