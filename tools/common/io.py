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

__all__ = [
    "atomic_write",
    "chdir",
    "read_json",
    "read_toml",
    "require_tar_data_filter",
    "write_json",
]


@contextlib.contextmanager
def chdir(path: Path):
    """Temporarily change the working directory (3.10-safe ``contextlib.chdir``)."""
    prev = os.getcwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(prev)


def require_tar_data_filter() -> None:
    """Ensure tarfile supports the PEP 706 ``data`` extraction filter.

    The ``filter`` argument landed in 3.12 and was backported to 3.10.12 /
    3.11.4. Raising here turns the cryptic ``TypeError`` on older 3.10 patch
    releases into actionable guidance.
    """
    import tarfile

    if not hasattr(tarfile, "data_filter"):
        raise RuntimeError(
            "Safe tar extraction requires the PEP 706 'data' filter "
            "(Python 3.10.12+, 3.11.4+, or 3.12+). Update Python "
            "(Ubuntu 22.04 ships 3.10.12+) to extract this archive."
        )


def read_json(path: Path) -> Any:
    """Read JSON from *path* (UTF-8). Raises on parse error or missing file."""
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, data: Any, *, indent: int = 2, sort_keys: bool = False) -> None:
    """Write *data* as JSON to *path* (UTF-8, trailing newline)."""
    text = json.dumps(data, indent=indent, sort_keys=sort_keys, ensure_ascii=False)
    path.write_text(text + "\n", encoding="utf-8")


def read_toml(path: Path) -> dict[str, Any]:
    """Read TOML from *path*, using stdlib ``tomllib`` (3.11+) or ``tomli`` on 3.10.

    Import is deferred so JSON-only callers (build-config composite action runs
    under runner system python which is 3.10 on some images) don't blow up
    transitively.
    """
    try:
        import tomllib
    except ModuleNotFoundError:  # stdlib tomllib is 3.11+; Ubuntu 22 ships 3.10
        try:
            import tomli as tomllib  # type: ignore[import-not-found]
        except ModuleNotFoundError as exc:
            raise ModuleNotFoundError(
                f"Reading {path} needs Python 3.11+ (stdlib tomllib) or the 'tomli' package "
                "on 3.10. Install uv (recommended) so bootstrap uses 'uv sync', or run "
                "'pip install tomli'."
            ) from exc

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
