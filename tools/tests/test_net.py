#!/usr/bin/env python3
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
