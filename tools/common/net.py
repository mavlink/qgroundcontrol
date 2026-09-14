"""Network helpers for QGC CI scripts: retrying subprocess runs and file downloads.

Consolidates the retry-with-backoff logic that android_sdk_helper, mold_helper,
and ccache_helper each reimplemented.
"""

from __future__ import annotations

import shutil
import sys
import tempfile
import time
import urllib.request
from typing import TYPE_CHECKING
from urllib.error import URLError

from .proc import run_checked_with_retry

if TYPE_CHECKING:
    from collections.abc import Sequence
    from pathlib import Path

__all__ = ["download_file", "download_with_retry", "read_url_text", "run_with_retries"]


def read_url_text(url: str, *, timeout: int = 60, encoding: str = "utf-8") -> str:
    """Fetch a small text response using the dependency-free CI HTTP boundary."""
    with urllib.request.urlopen(urllib.request.Request(url), timeout=timeout) as response:
        return response.read().decode(encoding)


def run_with_retries(
    cmd: Sequence[str], *, attempts: int = 3, backoff: float = 15.0, timeout: float = 300
) -> None:
    """Compatibility adapter for bootstrap commands using the shared retry policy."""
    run_checked_with_retry(
        cmd, max_attempts=attempts, retry_backoff_seconds=backoff, timeout=timeout
    )


def download_file(url: str, dest: Path, *, timeout: float = 120) -> None:
    """Publish a complete download atomically, preserving an existing file on failure."""
    from pathlib import Path

    staging: Path | None = None
    try:
        with (
            urllib.request.urlopen(urllib.request.Request(url), timeout=timeout) as resp,
            tempfile.NamedTemporaryFile(
                dir=dest.parent, prefix=f".{dest.name}.", delete=False
            ) as f,
        ):
            staging = Path(f.name)
            shutil.copyfileobj(resp, f)
        staging.replace(dest)
    finally:
        if staging is not None:
            staging.unlink(missing_ok=True)


def download_with_retry(
    url: str, dest: Path, *, attempts: int = 3, delay: float = 5.0, timeout: float = 120
) -> None:
    """Download *url* to *dest*, retrying transient network failures."""
    if attempts < 1 or delay < 0 or timeout <= 0:
        raise ValueError("attempts and timeout must be positive; delay must be non-negative")
    last: Exception | None = None
    for attempt in range(1, attempts + 1):
        try:
            print(f"Downloading {url} (attempt {attempt}/{attempts})")
            download_file(url, dest, timeout=timeout)
            return
        except (URLError, OSError) as e:
            last = e
            print(f"Download failed: {e}", file=sys.stderr)
            if attempt < attempts:
                time.sleep(delay)
    raise RuntimeError(f"Failed to download {url}: {last}")
