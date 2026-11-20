"""Dependency-free network helpers for QGC bootstrap and CI scripts."""

from __future__ import annotations

import contextlib
import hashlib
import io
import os
import re
import sys
import tempfile
import time
import urllib.request
from http.client import HTTPException
from pathlib import Path
from typing import Any
from urllib.error import HTTPError

from .io import extract_tar_data, extract_zip_safe, sha256_file

__all__ = ["download_file", "fetch_bytes", "fetch_release_member", "read_url_text"]

# Request Timeout and Too Many Requests clear up on their own; other 4xx responses never do.
_TRANSIENT_CLIENT_ERRORS = frozenset({408, 429})


class ChecksumMismatchError(RuntimeError):
    """Downloaded content does not match its expected digest."""


class DownloadTimeLimitError(RuntimeError):
    """A download outlasted its time limit; with a pinned digest its partial file is kept."""


def read_url_text(url: str, *, timeout: int = 60, encoding: str = "utf-8") -> str:
    """Fetch a small text response using the dependency-free CI HTTP boundary."""
    with urllib.request.urlopen(urllib.request.Request(url), timeout=timeout) as response:
        return response.read().decode(encoding)


def _is_transient(error: HTTPException | OSError) -> bool:
    """Connection errors, timeouts, truncated bodies, 5xx, 408 and 429 are worth retrying."""
    if isinstance(error, HTTPError):
        return error.code >= 500 or error.code in _TRANSIENT_CLIENT_ERRORS
    return True


def fetch_bytes(
    url: str, *, timeout: float, attempts: int = 3, backoff_seconds: float = 2.0
) -> bytes:
    """GET a small response into memory, retrying like download_file.

    A final HTTP error (e.g. a 404) propagates as ``HTTPError`` at once so callers can tell it
    apart; transient failures that outlast every attempt raise ``RuntimeError``.
    """
    if attempts < 1:
        raise ValueError("attempts must be positive")
    last: HTTPException | OSError | None = None
    for attempt in range(1, attempts + 1):
        try:
            with urllib.request.urlopen(urllib.request.Request(url), timeout=timeout) as response:
                return response.read()
        except (HTTPException, OSError) as error:
            if not _is_transient(error):
                raise
            last = error
            if attempt < attempts:
                time.sleep(backoff_seconds * attempt)
    raise RuntimeError(f"Failed to fetch {url} after {attempts} attempts: {last}") from last


def _content_range(headers: Any) -> tuple[int, int | None] | None:
    """Return a 206 response's first byte and total size (None when unknown)."""
    value = headers.get("Content-Range") if headers is not None else None
    match = re.fullmatch(r"bytes (\d+)-\d+/(\d+|\*)", str(value or "").strip())
    if match is None:
        return None
    return int(match[1]), None if match[2] == "*" else int(match[2])


def _fetch_into(
    url: str, staging: Path, offset: int, timeout: float, deadline: float | None
) -> None:
    """Write url to staging, continuing after its first offset bytes when the server allows."""
    request = urllib.request.Request(url)
    if offset:
        request.add_header("Range", f"bytes={offset}-")
    with urllib.request.urlopen(request, timeout=timeout) as response:
        headers = getattr(response, "headers", None)
        if offset and getattr(response, "status", None) == 206:
            resumed = _content_range(headers)
            if resumed is None or resumed[0] != offset:
                # Start over rather than splice bytes from an unexpected position.
                staging.unlink(missing_ok=True)
                raise OSError(f"server resumed at {resumed} instead of byte {offset}")
            expected_size = resumed[1]
        else:
            offset = 0
            content_length = headers.get("Content-Length") if headers is not None else None
            expected_size = int(content_length) if content_length is not None else None
        # read1 returns whatever has arrived, so a slow server still meets the deadline check;
        # read would wait for a whole chunk.
        read = response.read1 if isinstance(response, io.BufferedIOBase) else response.read
        received = offset
        with staging.open("ab" if offset else "wb") as output:
            while chunk := read(1024 * 1024):
                output.write(chunk)
                received += len(chunk)
                if deadline is not None and time.monotonic() > deadline:
                    raise DownloadTimeLimitError(f"reached its time limit after {received} bytes")
    if expected_size is not None and received != expected_size:
        raise OSError(f"incomplete download: expected {expected_size} bytes, received {received}")


def download_file(
    url: str,
    dest: Path,
    *,
    expected_sha256: str | None = None,
    attempts: int = 3,
    retry_backoff_seconds: float = 5.0,
    timeout: float = 120,
    time_limit: float | None = None,
) -> None:
    """Download and atomically publish a file after optional SHA-256 verification.

    Transient failures are retried with a linear backoff. With *expected_sha256*, an existing
    *dest* that matches it is kept, and an attempt cut off mid-transfer resumes where it stopped
    when the server honors HTTP Range requests; the digest check then catches a mismatched prefix
    and starts over. Each call downloads into its own staging file. With a digest, a failed call
    leaves the partial file beside *dest* under a name derived from the digest, and the next call
    (from a mirror of the same file, or after this process was killed) takes it over to resume, so
    concurrent calls never share one. Without a digest nothing proves a resumed file is one
    version, so every attempt starts over. *time_limit* bounds the whole call in seconds, so a
    slow but steady server cannot hold up a faster mirror; it raises ``DownloadTimeLimitError``.
    A final HTTP error (e.g. a 404) propagates as ``HTTPError`` at once, and failures that outlast
    every attempt raise ``RuntimeError``.
    """
    if attempts < 1:
        raise ValueError("attempts must be positive")
    if retry_backoff_seconds < 0:
        raise ValueError("retry_backoff_seconds must be non-negative")
    if timeout <= 0:
        raise ValueError("timeout must be positive")
    if time_limit is not None and time_limit <= 0:
        raise ValueError("time_limit must be positive")
    partial: Path | None = None
    if expected_sha256 is not None:
        expected_sha256 = expected_sha256.lower()
        try:
            valid_digest = len(bytes.fromhex(expected_sha256)) == hashlib.sha256().digest_size
        except ValueError:
            valid_digest = False
        if not valid_digest or len(expected_sha256) != 64:
            raise ValueError("expected_sha256 must be a 64-character hexadecimal digest")
        if dest.is_file() and sha256_file(dest) == expected_sha256:
            print(f"Using {dest}, which already matches its SHA-256")
            return
        partial = dest.parent / f".{dest.name}.{expected_sha256[:16]}.part"

    with tempfile.NamedTemporaryFile(
        dir=dest.parent, prefix=f".{dest.name}.", delete=False
    ) as handle:
        staging = Path(handle.name)
    if partial is not None:
        # Take over a failed call's partial file; a concurrent call then finds none.
        with contextlib.suppress(FileNotFoundError):
            os.replace(partial, staging)

    deadline = None if time_limit is None else time.monotonic() + time_limit
    last: Exception | None = None
    try:
        for attempt in range(1, attempts + 1):
            if deadline is not None and attempt > 1 and time.monotonic() > deadline:
                raise DownloadTimeLimitError(f"reached its time limit; last error: {last}")
            offset = staging.stat().st_size if partial is not None and staging.exists() else 0
            resuming = f", resuming at byte {offset}" if offset else ""
            print(f"Downloading {url} (attempt {attempt}/{attempts}{resuming})")
            try:
                _fetch_into(url, staging, offset, timeout, deadline)
                if expected_sha256 is not None:
                    actual_sha256 = sha256_file(staging)
                    if actual_sha256 != expected_sha256:
                        staging.unlink(missing_ok=True)
                        mismatch = ChecksumMismatchError(
                            f"SHA-256 mismatch for {dest.name}: "
                            f"expected {expected_sha256}, got {actual_sha256}"
                        )
                        if not offset or attempt == attempts:
                            raise mismatch
                        # The partial came from an earlier, different file; start over.
                        last = mismatch
                        print(f"Download failed: {mismatch}", file=sys.stderr)
                        continue
                os.replace(staging, dest)
                return
            except ChecksumMismatchError:
                raise
            except (HTTPException, OSError) as error:
                if isinstance(error, HTTPError) and error.code == 416 and offset:
                    # Range Not Satisfiable: the partial is not a prefix of this file.
                    staging.unlink(missing_ok=True)
                elif not _is_transient(error):
                    raise
                last = error
                print(f"Download failed: {error}", file=sys.stderr)
                if attempt < attempts:
                    delay = retry_backoff_seconds * attempt
                    if delay > 0:
                        time.sleep(delay)
    finally:
        if staging.exists():
            if partial is not None and staging.stat().st_size:
                # Leave the bytes for the next call to resume.
                os.replace(staging, partial)
            else:
                staging.unlink()
    raise RuntimeError(f"Failed to download {url}: {last}")


def fetch_release_member(url: str, sha256: str, member: str, staging: Path) -> Path:
    """Download a SHA-256-pinned release archive into *staging*, extract it, and return *member*.

    The member must be a regular file; a symlink could point the install outside the archive.
    """
    archive = staging / url.rsplit("/", 1)[-1]
    download_file(url, archive, expected_sha256=sha256)
    if archive.name.endswith(".zip"):
        extract_zip_safe(archive, staging)
    else:
        extract_tar_data(archive, staging)
    source = staging / member
    if source.is_symlink() or not source.is_file():
        raise RuntimeError(f"No regular file {member} in {archive.name}")
    return source
