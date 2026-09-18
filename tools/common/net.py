"""Dependency-free network helpers for QGC bootstrap and CI scripts."""

from __future__ import annotations

import hashlib
import os
import sys
import tempfile
import time
import urllib.request
from http.client import IncompleteRead
from pathlib import Path
from urllib.error import URLError

__all__ = ["download_file", "read_url_text"]


class ChecksumMismatchError(RuntimeError):
    """Downloaded content does not match its expected digest."""


def read_url_text(url: str, *, timeout: int = 60, encoding: str = "utf-8") -> str:
    """Fetch a small text response using the dependency-free CI HTTP boundary."""
    with urllib.request.urlopen(urllib.request.Request(url), timeout=timeout) as response:
        return response.read().decode(encoding)


def download_file(
    url: str,
    dest: Path,
    *,
    expected_sha256: str | None = None,
    attempts: int = 3,
    retry_backoff_seconds: float = 5.0,
    timeout: float = 120,
) -> None:
    """Download and atomically publish a file after optional SHA-256 verification."""
    if attempts < 1:
        raise ValueError("attempts must be positive")
    if retry_backoff_seconds < 0:
        raise ValueError("retry_backoff_seconds must be non-negative")
    if timeout <= 0:
        raise ValueError("timeout must be positive")
    if expected_sha256 is not None:
        expected_sha256 = expected_sha256.lower()
        try:
            valid_digest = len(bytes.fromhex(expected_sha256)) == hashlib.sha256().digest_size
        except ValueError:
            valid_digest = False
        if not valid_digest or len(expected_sha256) != 64:
            raise ValueError("expected_sha256 must be a 64-character hexadecimal digest")

    last: Exception | None = None
    for attempt in range(1, attempts + 1):
        staging: Path | None = None
        try:
            print(f"Downloading {url} (attempt {attempt}/{attempts})")
            digest = hashlib.sha256()
            with (
                urllib.request.urlopen(urllib.request.Request(url), timeout=timeout) as response,
                tempfile.NamedTemporaryFile(
                    dir=dest.parent, prefix=f".{dest.name}.", delete=False
                ) as output,
            ):
                staging = Path(output.name)
                headers = getattr(response, "headers", None)
                content_length = headers.get("Content-Length") if headers is not None else None
                expected_size = int(content_length) if content_length is not None else None
                downloaded_size = 0
                while chunk := response.read(1024 * 1024):
                    output.write(chunk)
                    digest.update(chunk)
                    downloaded_size += len(chunk)
                if expected_size is not None and downloaded_size != expected_size:
                    raise OSError(
                        f"incomplete download: expected {expected_size} bytes, "
                        f"received {downloaded_size}"
                    )
            actual_sha256 = digest.hexdigest()
            if expected_sha256 is not None and actual_sha256 != expected_sha256:
                raise ChecksumMismatchError(
                    f"SHA-256 mismatch for {dest.name}: "
                    f"expected {expected_sha256}, got {actual_sha256}"
                )
            os.replace(staging, dest)
            return
        except ChecksumMismatchError:
            raise
        except (IncompleteRead, URLError, OSError) as error:
            last = error
            print(f"Download failed: {error}", file=sys.stderr)
            if attempt < attempts:
                delay = retry_backoff_seconds * attempt
                if delay > 0:
                    time.sleep(delay)
        finally:
            if staging is not None:
                staging.unlink(missing_ok=True)
    raise RuntimeError(f"Failed to download {url}: {last}")
