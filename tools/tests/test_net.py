"""Tests for tools/common/net.py."""

from __future__ import annotations

import hashlib
import io
from http.client import IncompleteRead
from unittest.mock import MagicMock, patch
from urllib.error import URLError

import pytest
from common.net import download_file, read_url_text


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
