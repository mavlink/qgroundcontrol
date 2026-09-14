"""Tests for tools/common/net.py."""

from __future__ import annotations

from unittest.mock import MagicMock, patch

from common.net import read_url_text


def test_read_url_text_decodes_response() -> None:
    response = MagicMock()
    response.__enter__.return_value.read.return_value = b"hello\n"
    with patch("common.net.urllib.request.urlopen", return_value=response) as urlopen:
        assert read_url_text("https://example.test/value", timeout=7) == "hello\n"
    request = urlopen.call_args.args[0]
    assert request.full_url == "https://example.test/value"
    assert urlopen.call_args.kwargs["timeout"] == 7


def test_failed_download_preserves_existing_destination(tmp_path):
    import pytest
    from common.net import download_file

    destination = tmp_path / "sdk.zip"
    destination.write_bytes(b"previous complete download")
    response = MagicMock()
    response.__enter__.return_value.read.side_effect = [b"partial", OSError("connection reset")]
    with patch("common.net.urllib.request.urlopen", return_value=response), pytest.raises(OSError):
        download_file("https://example.test/sdk.zip", destination)
    assert destination.read_bytes() == b"previous complete download"
    assert list(tmp_path.iterdir()) == [destination]


def test_download_retries_timeout_then_publishes_complete_file(tmp_path):
    import io

    from common.net import download_with_retry

    destination = tmp_path / "sdk.zip"
    with (
        patch(
            "common.net.urllib.request.urlopen",
            side_effect=[TimeoutError(), io.BytesIO(b"complete")],
        ),
        patch("common.net.time.sleep"),
    ):
        download_with_retry("https://example.test/sdk.zip", destination)
    assert destination.read_bytes() == b"complete"
    assert list(tmp_path.iterdir()) == [destination]
