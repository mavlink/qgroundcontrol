"""Small file I/O helpers for QGC dev tools.

Centralizes the encoding and atomic-write patterns that get repeated in
``configure.py``, ``gh_actions.py``, ``build_config.py`` and others.
"""

from __future__ import annotations

import contextlib
import hashlib
import json
import os
import tempfile
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from pathlib import Path
    from typing import Literal

__all__ = [
    "atomic_write",
    "chdir",
    "extract_tar_data",
    "extract_zip_safe",
    "read_json",
    "read_toml",
    "require_tar_data_filter",
    "sha256_file",
    "write_json",
    "write_text_if_changed",
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


def extract_tar_data(
    archive: Path,
    destination: Path,
    *,
    mode: Literal["r", "r:*", "r:gz", "r:bz2", "r:xz"] = "r:*",
) -> None:
    """Extract a tar archive using Python's path-safe PEP 706 data filter."""
    import tarfile

    require_tar_data_filter()
    with tarfile.open(archive, mode) as tar:
        tar.extractall(destination, filter="data")


def extract_zip_safe(archive: Path, destination: Path) -> None:
    """Extract a zip archive, rejecting members that resolve outside *destination*."""
    import zipfile

    dest = destination.resolve()
    with zipfile.ZipFile(archive) as zf:
        for name in zf.namelist():
            if not (dest / name).resolve().is_relative_to(dest):
                raise ValueError(f"Unsafe zip member path: {name!r}")
        zf.extractall(dest)


def sha256_file(path: Path, *, chunk_size: int = 1024 * 1024) -> str:
    """Return the SHA-256 digest of *path* without loading it all into memory."""
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        while chunk := fh.read(chunk_size):
            digest.update(chunk)
    return digest.hexdigest()


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


def write_text_if_changed(path: Path, content: str, *, encoding: str = "utf-8") -> bool:
    """Atomically write *content* when it differs; return whether the file changed."""
    try:
        if path.read_text(encoding=encoding) == content:
            return False
    except FileNotFoundError:
        pass
    atomic_write(path, content, encoding=encoding)
    return True
