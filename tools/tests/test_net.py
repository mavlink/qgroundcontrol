"""Tests for tools/common/net.py."""

from __future__ import annotations

import functools
import hashlib
import io
import itertools
from email.message import Message
from http.client import IncompleteRead
from typing import ClassVar
from unittest.mock import MagicMock, patch
from urllib.error import HTTPError, URLError

import pytest
from common.net import DownloadTimeLimitError, download_file, fetch_bytes, read_url_text

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


def _resumed_response(content: bytes, *, start: int, total: int) -> MagicMock:
    response = _response(content)
    response.status = 206
    response.headers = {"Content-Range": f"bytes {start}-{total - 1}/{total}"}
    return response


def _ranges(urlopen: MagicMock) -> list[str | None]:
    return [call.args[0].get_header("Range") for call in urlopen.call_args_list]


@pytest.mark.parametrize("pinned", [True, False])
def test_interrupted_download_resumes_only_when_its_digest_is_pinned(
    tmp_path, pinned: bool
) -> None:
    destination = tmp_path / "sdk.zip"
    digest = hashlib.sha256(b"helloworld").hexdigest() if pinned else None
    # Without a digest nothing proves a resumed file is one version, so it starts over.
    rest = _resumed_response(b"world", start=5, total=10) if pinned else io.BytesIO(b"helloworld")
    with (
        patch(
            "common.net.urllib.request.urlopen",
            side_effect=[_response(b"hello", content_length=10), rest],
        ) as urlopen,
        patch("common.net.time.sleep"),
    ):
        download_file("https://example.test/sdk.zip", destination, expected_sha256=digest)
    assert _ranges(urlopen) == [None, "bytes=5-" if pinned else None]
    assert destination.read_bytes() == b"helloworld"
    assert list(tmp_path.iterdir()) == [destination]


def test_a_destination_matching_its_digest_is_not_downloaded_again(tmp_path) -> None:
    destination = tmp_path / "sdk.zip"
    destination.write_bytes(b"complete")
    with patch("common.net.urllib.request.urlopen") as urlopen:
        download_file(
            "https://example.test/sdk.zip",
            destination,
            expected_sha256=hashlib.sha256(b"complete").hexdigest(),
        )
    urlopen.assert_not_called()


def test_a_call_takes_over_the_partial_so_a_concurrent_call_cannot_share_it(tmp_path) -> None:
    destination = tmp_path / "sdk.zip"
    digest = hashlib.sha256(b"helloworld").hexdigest()
    with (
        patch(
            "common.net.urllib.request.urlopen",
            return_value=_response(b"hello", content_length=10),
        ),
        pytest.raises(RuntimeError, match="incomplete download"),
    ):
        download_file(
            "https://example.test/sdk.zip", destination, expected_sha256=digest, attempts=1
        )
    (partial,) = tmp_path.iterdir()

    def resume(request, timeout):
        # While this call downloads, the shared partial name is free for no one else to append to.
        assert not partial.exists()
        return _resumed_response(b"world", start=5, total=10)

    with patch("common.net.urllib.request.urlopen", side_effect=resume):
        download_file("https://example.test/sdk.zip", destination, expected_sha256=digest)
    assert destination.read_bytes() == b"helloworld"


def test_time_limit_holds_against_a_server_that_trickles_data(tmp_path) -> None:
    class Trickle(io.BufferedIOBase):
        status = 200
        headers: ClassVar[dict[str, str]] = {"Content-Length": "100"}

        def read1(self, size: int = -1) -> bytes:
            return b"x"

        def read(self, size: int | None = -1) -> bytes:
            raise AssertionError("read waits for a whole chunk; read1 returns what arrived")

    with (
        patch("common.net.urllib.request.urlopen", return_value=Trickle()),
        patch("common.net.time.monotonic", side_effect=itertools.count(0, 1)),
        pytest.raises(DownloadTimeLimitError),
    ):
        download_file("https://slow.test/sdk.zip", tmp_path / "sdk.zip", time_limit=5)


def test_verified_partial_outlives_the_call_for_another_source_to_resume(tmp_path) -> None:
    destination = tmp_path / "sdk.zip"
    digest = hashlib.sha256(b"helloworld").hexdigest()
    with (
        patch(
            "common.net.urllib.request.urlopen",
            return_value=_response(b"hello", content_length=10),
        ),
        pytest.raises(RuntimeError, match="incomplete download"),
    ):
        download_file(
            "https://upstream.test/sdk.zip", destination, expected_sha256=digest, attempts=1
        )
    assert not destination.exists()

    with patch(
        "common.net.urllib.request.urlopen",
        return_value=_resumed_response(b"world", start=5, total=10),
    ) as urlopen:
        download_file("https://mirror.test/sdk.zip", destination, expected_sha256=digest)
    assert _ranges(urlopen) == ["bytes=5-"]
    assert destination.read_bytes() == b"helloworld"
    assert list(tmp_path.iterdir()) == [destination]


def test_time_limit_stops_retrying_and_keeps_the_verified_partial(tmp_path) -> None:
    destination = tmp_path / "sdk.zip"
    digest = hashlib.sha256(b"helloworld").hexdigest()
    with (
        patch(
            "common.net.urllib.request.urlopen",
            return_value=_response(b"hello", content_length=10),
        ) as urlopen,
        patch("common.net.time.sleep"),
        # Each clock reading is 100 s after the last, so the 150 s limit passes after one attempt.
        patch("common.net.time.monotonic", side_effect=itertools.count(0, 100)),
        pytest.raises(DownloadTimeLimitError, match="time limit"),
    ):
        download_file(
            "https://slow.test/sdk.zip", destination, expected_sha256=digest, time_limit=150
        )
    urlopen.assert_called_once()
    (partial,) = tmp_path.iterdir()
    assert partial.read_bytes() == b"hello"


@pytest.mark.parametrize("resumed", ["stale", "complete"])
def test_unusable_partial_is_downloaded_again(tmp_path, resumed: str) -> None:
    destination = tmp_path / "sdk.zip"
    content = b"helloworld"
    digest = hashlib.sha256(content).hexdigest()
    # A stale partial from different bytes, or a complete one whose publish was interrupted.
    partial = b"HELLO" if resumed == "stale" else content
    with (
        patch(
            "common.net.urllib.request.urlopen",
            return_value=_response(partial, content_length=len(partial) + 1),
        ),
        pytest.raises(RuntimeError, match="incomplete download"),
    ):
        download_file(
            "https://example.test/sdk.zip", destination, expected_sha256=digest, attempts=1
        )

    reply = (
        _resumed_response(b"world", start=5, total=10) if resumed == "stale" else _http_error(416)
    )
    with (
        patch(
            "common.net.urllib.request.urlopen", side_effect=[reply, io.BytesIO(content)]
        ) as urlopen,
        patch("common.net.time.sleep"),
    ):
        download_file("https://example.test/sdk.zip", destination, expected_sha256=digest)
    assert _ranges(urlopen) == [f"bytes={len(partial)}-", None]
    assert destination.read_bytes() == content
    assert list(tmp_path.iterdir()) == [destination]


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
