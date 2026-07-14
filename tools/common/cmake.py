"""Dependency-light helpers for reading CMake cache metadata."""

from __future__ import annotations

import re
from pathlib import Path

__all__ = ["read_cache_dict", "read_cache_var"]

_CACHE_LINE_RE = re.compile(r"^([A-Za-z0-9_.\-]+):[^=]+=(.*)$")


def read_cache_dict(cache_path: Path | str) -> dict[str, str]:
    """Return typed ``CMakeCache.txt`` entries as a flat name-to-value mapping."""
    entries: dict[str, str] = {}
    try:
        with Path(cache_path).open(encoding="utf-8") as cache:
            for line in cache:
                match = _CACHE_LINE_RE.match(line.rstrip("\n"))
                if match:
                    entries[match.group(1)] = match.group(2)
    except FileNotFoundError:
        pass
    return entries


def read_cache_var(cache_path: Path | str, name: str) -> str | None:
    """Return one CMake cache value, or ``None`` when it is absent."""
    return read_cache_dict(cache_path).get(name)
