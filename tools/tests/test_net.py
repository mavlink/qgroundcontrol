"""Tests for tools/common/net.py."""

from __future__ import annotations

import functools
import hashlib
import io
from email.message import Message
from http.client import IncompleteRead
from unittest.mock import MagicMock, patch
from urllib.error import HTTPError, URLError

import pytest
from common.net import download_file, fetch_bytes, read_url_text

pytestmark = pytest.mark.windows


def _response(content: bytes, *, content_length: int | None = None) -> MagicMock:
    response = MagicMock()
    response.__enter__.return_value = response
    response.read.side_effect = [content, b""]
    response.headers = {"Content-Length": str(content_length)} if content_length is not None else {}
    return response


def test_read_url_text_decodes_response() -> None:
    response = MagicMock()
    response.__enter__.return_value.read.return_value = b"hello\n"
    with patch("common.net.urllib.request.urlopen", return_value=response) as urlopen:
        assert read_url_text("https://example.test/value", timeout=7) == "hello\n"
    request = urlopen.call_args.args[0]
    assert request.full_url == "https://example.test/value"
    assert urlopen.call_args.kwargs["timeout"] == 7


def test_failed_download_preserves_existing_destination(tmp_path) -> None:
    destination = tmp_path / "sdk.zip"
    destination.write_bytes(b"previous complete download")
    response = MagicMock()
    response.__enter__.return_value.read.side_effect = [b"partial", OSError("connection reset")]
    with (
        patch("common.net.urllib.request.urlopen", return_value=response),
        pytest.raises(RuntimeError, match="connection reset"),
    ):
        download_file("https://example.test/sdk.zip", destination, attempts=1)
    assert destination.read_bytes() == b"previous complete download"
    assert list(tmp_path.iterdir()) == [destination]


def test_download_retries_timeout_then_publishes_complete_file(tmp_path) -> None:
    destination = tmp_path / "sdk.zip"
    with (
        patch(
            "common.net.urllib.request.urlopen",
            side_effect=[URLError("timed out"), io.BytesIO(b"complete")],
        ),
        patch("common.net.time.sleep") as sleep,
    ):
        download_file(
            "https://example.test/sdk.zip",
            destination,
            retry_backoff_seconds=2,
        )
    assert destination.read_bytes() == b"complete"
    assert list(tmp_path.iterdir()) == [destination]
    sleep.assert_called_once_with(2)


def test_download_retries_truncated_fixed_length_response(tmp_path) -> None:
    destination = tmp_path / "sdk.zip"
    with (
        patch(
            "common.net.urllib.request.urlopen",
            side_effect=[_response(b"short", content_length=10), io.BytesIO(b"complete")],
        ) as urlopen,
        patch("common.net.time.sleep"),
    ):
        download_file("https://example.test/sdk.zip", destination)

    assert urlopen.call_count == 2
    assert destination.read_bytes() == b"complete"


def test_download_retries_incomplete_chunked_response(tmp_path) -> None:
    destination = tmp_path / "sdk.zip"
    incomplete = MagicMock()
    incomplete.__enter__.return_value = incomplete
    incomplete.headers = {}
    incomplete.read.side_effect = IncompleteRead(b"partial", 10)
    with (
        patch(
            "common.net.urllib.request.urlopen",
            side_effect=[incomplete, io.BytesIO(b"complete")],
        ) as urlopen,
        patch("common.net.time.sleep"),
    ):
        download_file("https://example.test/sdk.zip", destination)

    assert urlopen.call_count == 2
    assert destination.read_bytes() == b"complete"


def test_download_exhaustion_preserves_destination_and_cleans_staging(tmp_path) -> None:
    destination = tmp_path / "sdk.zip"
    destination.write_bytes(b"old")
    with (
        patch(
            "common.net.urllib.request.urlopen",
            side_effect=URLError("unavailable"),
        ) as urlopen,
        patch("common.net.time.sleep") as sleep,
        pytest.raises(RuntimeError, match="Failed to download"),
    ):
        download_file(
            "https://example.test/sdk.zip",
            destination,
            attempts=3,
            retry_backoff_seconds=1,
        )
    assert urlopen.call_count == 3
    assert [call.args[0] for call in sleep.call_args_list] == [1, 2]
    assert destination.read_bytes() == b"old"
    assert list(tmp_path.iterdir()) == [destination]


def test_checksum_mismatch_never_replaces_destination(tmp_path) -> None:
    destination = tmp_path / "sdk.zip"
    destination.write_bytes(b"trusted")
    expected = hashlib.sha256(b"expected").hexdigest()
    with (
        patch(
            "common.net.urllib.request.urlopen",
            return_value=io.BytesIO(b"corrupt"),
        ) as urlopen,
        patch("common.net.time.sleep") as sleep,
        pytest.raises(RuntimeError, match="SHA-256 mismatch"),
    ):
        download_file(
            "https://example.test/sdk.zip",
            destination,
            expected_sha256=expected,
            attempts=2,
            retry_backoff_seconds=0,
        )
    assert destination.read_bytes() == b"trusted"
    assert list(tmp_path.iterdir()) == [destination]
    urlopen.assert_called_once()
    sleep.assert_not_called()


def test_verified_download_publishes_matching_content(tmp_path) -> None:
    destination = tmp_path / "sdk.zip"
    content = b"verified"
    with patch("common.net.urllib.request.urlopen", return_value=io.BytesIO(content)):
        download_file(
            "https://example.test/sdk.zip",
            destination,
            expected_sha256=hashlib.sha256(content).hexdigest().upper(),
        )
    assert destination.read_bytes() == content


def test_failed_atomic_replace_preserves_existing_destination(tmp_path) -> None:
    destination = tmp_path / "sdk.zip"
    destination.write_bytes(b"old")
    with (
        patch("common.net.urllib.request.urlopen", return_value=io.BytesIO(b"new")),
        patch("common.net.os.replace", side_effect=OSError("replace failed")),
        pytest.raises(RuntimeError, match="replace failed"),
    ):
        download_file("https://example.test/sdk.zip", destination, attempts=1)
    assert destination.read_bytes() == b"old"
    assert list(tmp_path.iterdir()) == [destination]


def test_download_rejects_invalid_checksum_before_network(tmp_path) -> None:
    with (
        patch("common.net.urllib.request.urlopen") as urlopen,
        pytest.raises(ValueError, match="expected_sha256"),
    ):
        download_file("https://example.test/sdk.zip", tmp_path / "sdk.zip", expected_sha256="bad")
    urlopen.assert_not_called()


def _http_error(code: int) -> HTTPError:
    return HTTPError("https://example.test", code, "error", Message(), None)


@pytest.mark.parametrize("code", [403, 404])
def test_download_fails_at_once_on_a_final_http_error(tmp_path, code: int) -> None:
    with (
        patch("common.net.urllib.request.urlopen", side_effect=_http_error(code)) as urlopen,
        patch("common.net.time.sleep") as sleep,
        pytest.raises(HTTPError) as raised,
    ):
        download_file("https://example.test/sdk.zip", tmp_path / "sdk.zip", attempts=4)
    assert raised.value.code == code
    urlopen.assert_called_once()
    sleep.assert_not_called()
    assert list(tmp_path.iterdir()) == []


@pytest.mark.parametrize("code", [408, 429, 503])
def test_download_retries_transient_http_errors(tmp_path, code: int) -> None:
    destination = tmp_path / "sdk.zip"
    with (
        patch(
            "common.net.urllib.request.urlopen",
            side_effect=[_http_error(code), io.BytesIO(b"complete")],
        ) as urlopen,
        patch("common.net.time.sleep"),
    ):
        download_file("https://example.test/sdk.zip", destination)
    assert urlopen.call_count == 2
    assert destination.read_bytes() == b"complete"


@pytest.mark.parametrize(
    ("outcomes", "calls", "raises"),
    [
        ([URLError("reset"), b"tile"], 2, None),
        ([_http_error(503), TimeoutError(), b"tile"], 3, None),
        ([_http_error(429), IncompleteRead(b"", 4), b"tile"], 3, None),
        ([_http_error(404)], 1, HTTPError),
        ([_http_error(503)] * 3, 3, RuntimeError),
    ],
    ids=["reset", "5xx-then-timeout", "429-then-truncated", "404", "exhausted"],
)
def test_fetch_bytes_retries_only_transient_failures(
    outcomes: list[object], calls: int, raises: type[Exception] | None
) -> None:
    responses = [io.BytesIO(item) if isinstance(item, bytes) else item for item in outcomes]
    get = functools.partial(fetch_bytes, "https://example.test/tile", timeout=1)
    with (
        patch("common.net.urllib.request.urlopen", side_effect=responses) as urlopen,
        patch("common.net.time.sleep") as sleep,
    ):
        if raises is None:
            assert get() == b"tile"
        else:
            with pytest.raises(raises):
                get()
    assert urlopen.call_count == calls
    assert [call.args[0] for call in sleep.call_args_list] == [2.0 * n for n in range(1, calls)]


def test_fetch_bytes_rejects_zero_attempts() -> None:
    with pytest.raises(ValueError, match="attempts"):
        fetch_bytes("https://example.test/tile", timeout=1, attempts=0)
