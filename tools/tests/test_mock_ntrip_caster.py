"""Tests for tools/simulation/mock_ntrip_caster.py."""

from __future__ import annotations

import socket

import pytest
from simulation import mock_ntrip_caster as caster

# RTCM 1005 frame from a real receiver, shared with QuectelProtocolTest.cc.
RECEIVER_1005_FRAME = bytes.fromhex("d300133ed122033a2266a8ee8b4a8c4d3507a1bddfbfbb0a12")
TIMEOUT_SECONDS = 5


def _payload_length(frame: bytes) -> int:
    return ((frame[1] & 0x03) << 8) | frame[2]


def _message_id(frame: bytes) -> int:
    return (frame[3] << 4) | (frame[4] >> 4)


def _has_valid_crc(frame: bytes) -> bool:
    return caster.crc24q(frame[:-3]) == int.from_bytes(frame[-3:], "big")


@pytest.mark.parametrize(
    ("data", "expected"),
    [
        (b"", 0),
        (b"\x00", 0),
        (b"\x01", 0x864CFB),
        (b"123456789", 0xCDE703),
        (RECEIVER_1005_FRAME[:-3], 0xBB0A12),
        (RECEIVER_1005_FRAME, 0),
    ],
    ids=["empty", "zero", "one", "standard-check", "receiver-frame", "receiver-frame-residue"],
)
def test_crc24q_matches_rtcm_vectors(data: bytes, expected: int) -> None:
    assert caster.crc24q(data) == expected


@pytest.mark.parametrize(
    ("frame", "message_id"),
    [(caster.make_rtcm_1005(), 1005), (caster.make_rtcm_1077(), 1077)],
    ids=["1005", "1077"],
)
def test_rtcm_frame_layout(frame: bytes, message_id: int) -> None:
    assert frame[0] == 0xD3
    assert frame[1] & 0xFC == 0
    assert _payload_length(frame) == len(frame) - 6
    assert _message_id(frame) == message_id
    assert _has_valid_crc(frame)


def test_bad_crc_frame_keeps_layout_but_fails_crc() -> None:
    frame = caster.make_bad_crc_frame()
    assert frame[0] == 0xD3
    assert _payload_length(frame) == len(frame) - 6
    assert not _has_valid_crc(frame)


def test_rtcm_frame_rejects_oversized_payload() -> None:
    with pytest.raises(ValueError, match="too large"):
        caster.make_rtcm_frame(1005, bytes(1023))


def test_default_bind_address_is_loopback() -> None:
    args = caster.parse_args([])
    assert (args.host, args.port) == ("127.0.0.1", 2101)


def _recv_exact(sock: socket.socket, size: int) -> bytes:
    data = b""
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        assert chunk, f"connection closed after {len(data)} of {size} bytes"
        data += chunk
    return data


def _recv_all(sock: socket.socket) -> bytes:
    data = b""
    while chunk := sock.recv(4096):
        data += chunk
    return data


class _Session:
    """A ClientHandler serving one end of a socket pair."""

    def __init__(self, *argv: str) -> None:
        self.client, server = socket.socketpair()
        self.client.settimeout(TIMEOUT_SECONDS)
        self.handler = caster.ClientHandler(server, ("test", 0), caster.parse_args(list(argv)))
        self.handler.start()

    def request(self, path: str, *headers: str) -> None:
        lines = [f"GET {path} HTTP/1.1", "Host: 127.0.0.1", *headers]
        self.client.sendall(("\r\n".join(lines) + "\r\n\r\n").encode())

    def close(self) -> None:
        self.handler.running = False
        self.client.close()
        self.handler.join(TIMEOUT_SECONDS)
        assert not self.handler.is_alive()


@pytest.fixture
def session_factory():
    sessions: list[_Session] = []

    def make(*argv: str) -> _Session:
        sessions.append(_Session(*argv))
        return sessions[-1]

    yield make
    for session in sessions:
        session.close()


def test_root_request_returns_source_table(session_factory) -> None:
    session = session_factory()
    session.request("/")

    head, _, body = _recv_all(session.client).partition(b"\r\n\r\n")

    assert head.startswith(b"HTTP/1.1 200 OK\r\n")
    assert f"Content-Length: {len(body)}".encode() in head
    assert body.startswith(b"STR;MOCK1;")
    assert b"STR;MOCK_AUTH;" in body
    assert body.endswith(b"ENDSOURCETABLE\r\n")


def test_mountpoint_request_streams_valid_rtcm(session_factory) -> None:
    session = session_factory("--rate", "50")
    session.request("/MOCK1", "Ntrip-Version: Ntrip/2.0")

    assert _recv_exact(session.client, 14) == b"ICY 200 OK\r\n\r\n"
    header = _recv_exact(session.client, 3)
    frame = header + _recv_exact(session.client, _payload_length(header) + 3)
    assert frame[0] == 0xD3
    assert _message_id(frame) == 1005
    assert _has_valid_crc(frame)


@pytest.mark.parametrize(
    ("argv", "path", "headers", "status"),
    [
        (("--error", "http_401"), "/MOCK1", (), b"401 Unauthorized"),
        (("--error", "http_500"), "/MOCK1", (), b"500 Internal Server Error"),
        (("--auth", "user:pass"), "/MOCK1", (), b"401 Unauthorized"),
        (
            ("--auth", "user:pass"),
            "/MOCK1",
            ("Authorization: Basic dXNlcjp3cm9uZw==",),
            b"401 Unauthorized",
        ),
        ((), "/NOPE", (), b"404 Not Found"),
    ],
    ids=["http_401", "http_500", "auth-missing", "auth-wrong", "unknown-mount"],
)
def test_error_responses(session_factory, argv, path, headers, status) -> None:
    session = session_factory(*argv)
    session.request(path, *headers)

    assert _recv_all(session.client) == b"HTTP/1.1 " + status + b"\r\n\r\n"


def test_valid_basic_auth_streams(session_factory) -> None:
    session = session_factory("--auth", "user:pass", "--rate", "50")
    session.request("/MOCK_AUTH", "Authorization: Basic dXNlcjpwYXNz")

    assert _recv_exact(session.client, 14) == b"ICY 200 OK\r\n\r\n"
