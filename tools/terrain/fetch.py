"""HTTP GET with retries for transient failures (stdlib only)."""

from __future__ import annotations

import http.client
import time
import urllib.error
import urllib.request


def fetch_bytes(
    url: str, *, timeout: float, attempts: int = 3, backoff_seconds: float = 2.0
) -> bytes:
    """GET *url*; retries connection errors, timeouts and 5xx; other HTTP errors raise at once.

    4xx (e.g. a 404 for a cell a server doesn't have) propagate as ``HTTPError`` so callers can
    tell them apart; transient failures that outlast every attempt raise ``RuntimeError``.
    """
    if attempts < 1:
        raise ValueError("attempts must be positive")
    last_error: Exception | None = None
    for attempt in range(attempts):
        if attempt:
            time.sleep(backoff_seconds * attempt)
        try:
            with urllib.request.urlopen(url, timeout=timeout) as response:
                return response.read()
        except urllib.error.HTTPError as error:
            if error.code < 500:
                raise
            last_error = error
        except (OSError, http.client.HTTPException) as error:
            last_error = error
    raise RuntimeError(f"Failed to fetch {url} after {attempts} attempts: {last_error}") from (
        last_error
    )
