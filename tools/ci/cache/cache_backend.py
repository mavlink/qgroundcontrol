#!/usr/bin/env python3
"""Detect the Actions cache backend for setup-python and export it to $GITHUB_ENV.

RunsOn's magic cache points ACTIONS_RESULTS_URL at its S3 proxy and moves the runner's own
results URL to ZCTIONS_RESULTS_URL. The runner injects these URLs into JavaScript actions only,
where they can override the shell environment (RunsOn < 3.1 does on Windows), so setup-python
runs this script from actions/github-script with that environment.

Exports QGC_ACTIONS_CACHE_BACKEND (`s3` or `github`) and, for S3, UV_PYTHON_PREFERENCE=only-managed.
Standalone: it runs on the runner's interpreter before setup-python.
"""

from __future__ import annotations

import argparse
import os
import sys
from typing import TYPE_CHECKING
from urllib.parse import urlsplit

if TYPE_CHECKING:
    from collections.abc import Mapping

DEFAULT_PORTS = {"http": 80, "https": 443}
INACTIVE_WARNING = (
    "RunsOn S3 cache routing is inactive; using GitHub cache limits. "
    "Windows requires RunsOn >= 3.1."
)


def url_origin(url: str) -> tuple[str, str, int] | None:
    """Return an http(s) URL's origin (scheme, host, port) as WHATWG URL normalises it, else None."""
    try:
        parts = urlsplit(url.strip())
        port = parts.port
    except ValueError:
        return None
    if parts.scheme not in DEFAULT_PORTS or not parts.hostname:
        return None
    return parts.scheme, parts.hostname, DEFAULT_PORTS[parts.scheme] if port is None else port


def runs_on_s3_configured(env: Mapping[str, str]) -> bool:
    return bool(env.get("RUNS_ON_RUNNER_NAME") and env.get("RUNS_ON_S3_BUCKET_CACHE"))


def detect_backend(env: Mapping[str, str]) -> str:
    """Return `s3` when cache requests reach RunsOn's proxy instead of GitHub, else `github`."""
    if not runs_on_s3_configured(env):
        return "github"
    endpoint = url_origin(env.get("ACTIONS_RESULTS_URL", ""))
    original = url_origin(env.get("ZCTIONS_RESULTS_URL", ""))
    # Missing or invalid proxy metadata keeps GitHub's storage limits.
    if endpoint and original and original[0] == "https" and endpoint != original:
        return "s3"
    return "github"


def main(argv: list[str] | None = None) -> int:
    argparse.ArgumentParser(description=__doc__).parse_args(argv)
    backend = detect_backend(os.environ)
    if backend != "s3" and runs_on_s3_configured(os.environ):
        print(f"::warning::{INACTIVE_WARNING}", flush=True)
    exports = {"QGC_ACTIONS_CACHE_BACKEND": backend}
    if backend == "s3":
        # Full runner images ship a system Python that uv would otherwise prefer, leaving
        # cache-python without a managed install to save.
        exports["UV_PYTHON_PREFERENCE"] = "only-managed"
    github_env = os.environ.get("GITHUB_ENV")
    if github_env:
        with open(github_env, "a", encoding="utf-8") as file:
            file.writelines(f"{key}={value}\n" for key, value in exports.items())
    print(f"Actions cache backend: {backend}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
