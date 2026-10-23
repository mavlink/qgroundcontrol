"""Fetch Updates.xml and archives, verified against SHA-256 sums from download.qt.io itself.

Archives may come from any mirror: download.qt.io redirects to one that MirrorBrain picks, and the
fallbacks serve the same tree. A mirror can only fail a checksum, never substitute content.
"""

from __future__ import annotations

import hashlib
import re
from typing import TYPE_CHECKING
from urllib.error import HTTPError

from common.net import download_file, fetch_bytes

if TYPE_CHECKING:
    from pathlib import Path

TRUSTED_URL = "https://download.qt.io"
MIRRORS = (TRUSTED_URL, "https://master.qt.io", "https://qt.mirror.constant.com")
_SHA256 = re.compile(r"[0-9a-f]{64}")
_TIMEOUT = 60


def trusted_sha256(path: str) -> str:
    """Return the SHA-256 that download.qt.io publishes for *path* (``<path>.sha256``)."""
    try:
        text = fetch_bytes(f"{TRUSTED_URL}/{path}.sha256", timeout=_TIMEOUT).decode(
            "ascii", "replace"
        )
    except HTTPError as error:
        raise RuntimeError(
            f"download.qt.io has no SHA-256 for {path} (HTTP {error.code})"
        ) from error
    digest = text.split()[0].lower() if text.split() else ""
    if not _SHA256.fullmatch(digest):
        raise RuntimeError(f"Malformed SHA-256 for {path}: {text[:80]!r}")
    return digest


def fetch_updates_xml(repo_dir: str) -> bytes:
    """Return ``<repo_dir>/Updates.xml`` after checking it against its published SHA-256."""
    path = f"{repo_dir}/Updates.xml"
    try:
        data = fetch_bytes(f"{TRUSTED_URL}/{path}", timeout=_TIMEOUT)
    except HTTPError as error:
        raise RuntimeError(
            f"Cannot fetch {TRUSTED_URL}/{path} (HTTP {error.code}); check the Qt version, host, "
            "target and arch"
        ) from error
    expected, actual = trusted_sha256(path), hashlib.sha256(data).hexdigest()
    if actual != expected:
        raise RuntimeError(f"SHA-256 mismatch for {path}: expected {expected}, got {actual}")
    return data


def download_archive(url_path: str, dest: Path, sha256: str) -> None:
    """Download *url_path* from the first mirror whose content matches *sha256*."""
    errors = []
    for mirror in MIRRORS:
        try:
            download_file(f"{mirror}/{url_path}", dest, expected_sha256=sha256)
            return
        except (RuntimeError, OSError) as error:  # checksum mismatch, HTTP error, exhausted retries
            errors.append(f"{mirror}: {error}")
    raise RuntimeError(f"No mirror served {url_path}:\n  " + "\n  ".join(errors))
