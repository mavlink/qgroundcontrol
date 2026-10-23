"""Actions cache backend detection for setup-python."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from ci.cache import cache_backend

if TYPE_CHECKING:
    from pathlib import Path

pytestmark = pytest.mark.windows

GITHUB_ENDPOINT = "https://results-receiver.actions.githubusercontent.com/"
GITHUB_ENDPOINT_EXPLICIT = "HTTPS://Results-Receiver.actions.githubusercontent.com:443/x"
GITHUB_ENDPOINT_HTTP = "http://results-receiver.actions.githubusercontent.com/"
PROXY_ENDPOINT = "http://10.1.2.3:6871/"
ENV_NAMES: tuple[str, ...] = (
    "RUNS_ON_RUNNER_NAME",
    "RUNS_ON_S3_BUCKET_CACHE",
    "ACTIONS_RESULTS_URL",
    "ZCTIONS_RESULTS_URL",
)


@pytest.mark.parametrize(
    ("runner", "bucket", "endpoint", "original", "expected"),
    [
        ("", "", GITHUB_ENDPOINT, "", "github"),
        ("windows", "bucket", GITHUB_ENDPOINT, "", "github"),
        ("linux", "bucket", PROXY_ENDPOINT, GITHUB_ENDPOINT, "s3"),
        ("windows", "bucket", PROXY_ENDPOINT, GITHUB_ENDPOINT, "s3"),
        ("windows", "bucket", GITHUB_ENDPOINT, GITHUB_ENDPOINT, "github"),
        ("windows", "bucket", PROXY_ENDPOINT, "", "github"),
        ("linux", "bucket", "invalid", GITHUB_ENDPOINT, "github"),
        ("linux", "bucket", "file:///cache", GITHUB_ENDPOINT, "github"),
        ("linux", "", PROXY_ENDPOINT, GITHUB_ENDPOINT, "github"),
        ("", "bucket", PROXY_ENDPOINT, GITHUB_ENDPOINT, "github"),
        # Origins compare normalised: an explicit default port and host case do not differ.
        ("linux", "bucket", GITHUB_ENDPOINT_EXPLICIT, GITHUB_ENDPOINT, "github"),
        ("linux", "bucket", "https://proxy.internal/", GITHUB_ENDPOINT, "s3"),
        ("linux", "bucket", PROXY_ENDPOINT, GITHUB_ENDPOINT_HTTP, "github"),
        ("linux", "bucket", "http://10.1.2.3:99999/", GITHUB_ENDPOINT, "github"),
        ("linux", "bucket", "http://", GITHUB_ENDPOINT, "github"),
    ],
)
def test_backend_requires_the_runs_on_proxy_and_its_original_endpoint(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    gh_env: Path,
    runner: str,
    bucket: str,
    endpoint: str,
    original: str,
    expected: str,
) -> None:
    env = dict(zip(ENV_NAMES, (runner, bucket, endpoint, original), strict=True))
    assert cache_backend.detect_backend(env) == expected

    for name, value in env.items():
        monkeypatch.setenv(name, value)
    assert cache_backend.main([]) == 0

    exports = [f"QGC_ACTIONS_CACHE_BACKEND={expected}"]
    if expected == "s3":
        exports.append("UV_PYTHON_PREFERENCE=only-managed")
    assert gh_env.read_text(encoding="utf-8").splitlines() == exports
    out = capsys.readouterr().out
    assert out.endswith(f"Actions cache backend: {expected}\n")
    warned = bool(runner and bucket and expected == "github")
    assert (f"::warning::{cache_backend.INACTIVE_WARNING}\n" in out) == warned


def test_unset_metadata_selects_github_without_a_warning(
    monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    for name in (*ENV_NAMES, "GITHUB_ENV"):
        monkeypatch.delenv(name, raising=False)
    assert cache_backend.main([]) == 0
    assert capsys.readouterr().out == "Actions cache backend: github\n"


@pytest.mark.parametrize(
    ("url", "origin"),
    [
        (PROXY_ENDPOINT, ("http", "10.1.2.3", 6871)),
        ("HTTP://Proxy:80/path", ("http", "proxy", 80)),
        (" https://[::1]/ ", ("https", "::1", 443)),
        ("https://user:secret@host:8443/", ("https", "host", 8443)),
        ("", None),
        ("invalid", None),
        ("file:///cache", None),
        ("ftp://host/", None),
        ("http://host:port/", None),
    ],
)
def test_url_origin_matches_javascript_url_origin(
    url: str, origin: tuple[str, str, int] | None
) -> None:
    assert cache_backend.url_origin(url) == origin
