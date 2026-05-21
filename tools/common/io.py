"""Small file I/O helpers for QGC dev tools.

Centralizes the encoding and atomic-write patterns that get repeated in
``configure.py``, ``gh_actions.py``, ``build_config.py`` and others.
"""

from __future__ import annotations

import contextlib
import json
import os
import tempfile
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from pathlib import Path

__all__ = ["atomic_write", "read_json", "read_toml", "write_json"]


def read_json(path: Path) -> Any:
    """Read JSON from *path* (UTF-8). Raises on parse error or missing file."""
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, data: Any, *, indent: int = 2, sort_keys: bool = False) -> None:
    """Write *data* as JSON to *path* (UTF-8, trailing newline)."""
    text = json.dumps(data, indent=indent, sort_keys=sort_keys, ensure_ascii=False)
    path.write_text(text + "\n", encoding="utf-8")


def read_toml(path: Path) -> dict[str, Any]:
    """Read TOML from *path* using stdlib ``tomllib`` (3.11+).

    Import is deferred so JSON-only callers (build-config composite action runs
    under runner system python which is 3.10 on some images) don't blow up
    transitively.
    """
    import tomllib  # noqa: PLC0415

    with path.open("rb") as fh:
        return tomllib.load(fh)


def atomic_write(path: Path, content: str, *, encoding: str = "utf-8") -> None:
    """Write *content* to *path* atomically (write tmp + rename).

    Survives crashes mid-write and is safe against partial reads by other
    processes. Same-directory tmpfile keeps the rename atomic on POSIX.
    """
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp_name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(fd, "w", encoding=encoding) as fh:
            fh.write(content)
        os.replace(tmp_name, path)
    except Exception:
        with contextlib.suppress(FileNotFoundError):
            os.unlink(tmp_name)
        raise
