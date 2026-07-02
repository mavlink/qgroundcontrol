"""Network helpers for QGC CI scripts: retrying subprocess runs and file downloads.

Consolidates the retry-with-backoff logic that android_sdk_helper, mold_helper,
and ccache_helper each reimplemented.
"""

from __future__ import annotations

import shutil
import subprocess
import sys
import time
import urllib.request
from typing import TYPE_CHECKING
from urllib.error import URLError

from .gh_actions import gh_warning

if TYPE_CHECKING:
    from collections.abc import Sequence
    from pathlib import Path

__all__ = ["download_file", "download_with_retry", "run_with_retries"]


def run_with_retries(
    cmd: Sequence[str], *, attempts: int = 3, backoff: float = 15.0, check: bool = True
) -> None:
    """Run *cmd*, retrying transient failures with exponential backoff."""
    for attempt in range(1, attempts + 1):
        try:
            subprocess.run(list(cmd), check=check)
            return
        except subprocess.CalledProcessError:
            if attempt == attempts:
                raise
            gh_warning(f"{cmd[0]} failed (attempt {attempt}/{attempts}); retrying in {backoff:g}s")
            time.sleep(backoff)
            backoff *= 2


def download_file(url: str, dest: Path, *, timeout: int = 120) -> None:
    """Download *url* to *dest* in a single attempt (stdlib urllib)."""
    with (
        urllib.request.urlopen(urllib.request.Request(url), timeout=timeout) as resp,
        open(dest, "wb") as f,
    ):
        shutil.copyfileobj(resp, f)


def download_with_retry(
    url: str, dest: Path, *, attempts: int = 3, delay: float = 5.0, timeout: int = 120
) -> None:
    """Download *url* to *dest*, retrying transient network failures."""
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
