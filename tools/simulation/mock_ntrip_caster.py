#!/usr/bin/env python3
"""
Mock NTRIP Caster for QGroundControl Testing

Simulates an NTRIP caster that serves a source table and streams RTCM3
corrections. Useful for testing the QGC NTRIP client without a real caster
or internet connection.

Usage:
    ./mock_ntrip_caster.py                          # Default: localhost:2101
    ./mock_ntrip_caster.py --port 2102              # Custom port
    ./mock_ntrip_caster.py --auth user:pass          # Require basic auth
    ./mock_ntrip_caster.py --rate 5                  # 5 RTCM messages/sec
    ./mock_ntrip_caster.py --error bad_crc           # Inject bad CRC every 10th msg
    ./mock_ntrip_caster.py --error drop              # Drop connection after 10 msgs
    ./mock_ntrip_caster.py --error http_401          # Always return 401
    ./mock_ntrip_caster.py --error http_500          # Always return 500

Real caster for comparison:
    Host: rtk2go.com  Port: 2101  Password: ntrip@  (or your email)
"""

import argparse
import base64
import logging
import select
import socket
import struct
import threading
import time

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger("mock-ntrip")

# ---------------------------------------------------------------------------
# RTCM3 frame builder
# ---------------------------------------------------------------------------

# CRC-24Q lookup table (same algorithm as RTCMParser::crc24q)
_CRC24Q_TABLE = [0] * 256


def _init_crc_table():
    for i in range(256):
        crc = i << 16
        for _ in range(8):
            crc <<= 1
            if crc & 0x1000000:
                crc ^= 0x1864CFB
        _CRC24Q_TABLE[i] = crc & 0xFFFFFF


_init_crc_table()


def crc24q(data: bytes) -> int:
    crc = 0
    for b in data:
        crc = ((crc << 8) & 0xFFFFFF) ^ _CRC24Q_TABLE[((crc >> 16) ^ b) & 0xFF]
    return crc


def make_rtcm_frame(message_id: int, payload: bytes = b"") -> bytes:
    """Build a complete RTCM3 frame with preamble, length, message ID, payload, and CRC."""
    # Message body = 12-bit message ID (big-endian in first 2 bytes) + payload
    id_high = (message_id >> 4) & 0xFF
    id_low = (message_id & 0x0F) << 4
    if payload:
        # Merge low nibble of ID with first nibble of payload
        body = bytes([id_high, id_low | (payload[0] >> 4)]) + payload[1:]
    else:
        body = bytes([id_high, id_low])

    length = len(body)
    if length > 1023:
        raise ValueError(f"RTCM payload too large: {length}")

    # Header: preamble (0xD3) + 6 reserved bits (0) + 10-bit length
    header = bytes([0xD3, (length >> 8) & 0x03, length & 0xFF])

    frame_without_crc = header + body
    crc = crc24q(frame_without_crc)
    crc_bytes = struct.pack(">I", crc)[1:]  # 3 bytes big-endian

    return frame_without_crc + crc_bytes


def make_rtcm_1005() -> bytes:
    """RTCM 1005 - Stationary RTK Reference Station ARP (compact, 19 bytes payload).
    Uses a fixed position near 0,0,0 for testing."""
    # Reference Station ID (12 bits) = 1
    # ITRF Realization Year (6 bits) = 0
    # GPS indicator (1) = 1, GLONASS (1) = 0, Galileo (1) = 0, Reference-Station (1) = 0
    # ECEF-X (38 bits signed, 0.0001m) = 0
    # Single Receiver Oscillator (1) = 0, Reserved (1) = 0
    # ECEF-Y (38 bits signed) = 0
    # Quarter Cycle Indicator (2) = 0
    # ECEF-Z (38 bits signed) = 0
    # Total payload: 152 bits = 19 bytes
    payload = bytearray(19)
    # Station ID = 1 in bits 0..11 -> byte 0 high nibble will be merged with msg ID
    payload[0] = 0x00  # station ID high 4 bits (will merge with msg id low nibble)
    payload[1] = 0x11  # station ID low 8 bits = 0x01, shifted: 0001 + GPS=1,GLO=0,GAL=0,REF=0
    # Rest is zeros (ECEF 0,0,0)
    return make_rtcm_frame(1005, bytes(payload))


def make_rtcm_1077() -> bytes:
    """RTCM 1077 - GPS MSM7 (minimal/empty observation for testing)."""
    # Minimal MSM header: station ID + epoch + flags, rest zeros
    payload = bytearray(25)  # minimum MSM7 header size
    return make_rtcm_frame(1077, bytes(payload))


def make_bad_crc_frame() -> bytes:
    """Build a valid-looking RTCM3 frame with intentionally bad CRC."""
    frame = bytearray(make_rtcm_1005())
    # Corrupt the last CRC byte
    frame[-1] ^= 0xFF
    return bytes(frame)


# ---------------------------------------------------------------------------
# Source table
# ---------------------------------------------------------------------------

SOURCE_TABLE = """\
STR;MOCK1;MOCK1;RTCM 3.3;1005(1),1077(1),1087(1);2;GPS+GLO;MOCK;USA;0.00;0.00;0;0;sNTRIP;none;N;N;0;
STR;MOCK2;MOCK2;RTCM 3.3;1005(1),1077(1);2;GPS;MOCK;USA;37.77;-122.42;0;0;sNTRIP;none;N;N;0;
STR;MOCK_AUTH;MOCK_AUTH;RTCM 3.3;1005(1);2;GPS;MOCK;USA;0.00;0.00;0;0;sNTRIP;none;B;N;0;
ENDSOURCETABLE\r\n"""


# ---------------------------------------------------------------------------
# Client handler
# ---------------------------------------------------------------------------


class ClientHandler(threading.Thread):
    def __init__(self, conn, addr, args):
        super().__init__(daemon=True)
        self.conn = conn
        self.addr = addr
        self.args = args
        self.running = True

    def run(self):
        try:
            self._handle()
        except (ConnectionResetError, BrokenPipeError):
            log.info("%s disconnected", self.addr)
        except Exception as e:
            log.error("%s error: %s", self.addr, e)
        finally:
            self.conn.close()

    def _handle(self):
        # Read HTTP request
        buf = b""
        while b"\r\n\r\n" not in buf:
            chunk = self.conn.recv(4096)
            if not chunk:
                return
            buf += chunk
            if len(buf) > 16384:
                log.warning("%s request too large", self.addr)
                return

        header_end = buf.index(b"\r\n\r\n")
        header = buf[:header_end].decode("utf-8", errors="replace")
        lines = header.split("\r\n")
        request_line = lines[0] if lines else ""
        log.info("%s >> %s", self.addr, request_line)

        # Parse headers
        headers = {}
        for line in lines[1:]:
            if ":" in line:
                k, v = line.split(":", 1)
                headers[k.strip().lower()] = v.strip()

        # Error injection: HTTP errors
        if self.args.error == "http_401":
            self._send(b"HTTP/1.1 401 Unauthorized\r\n\r\n")
            return
        if self.args.error == "http_500":
            self._send(b"HTTP/1.1 500 Internal Server Error\r\n\r\n")
            return

        # Auth check
        if self.args.auth:
            expected = base64.b64encode(self.args.auth.encode()).decode()
            auth = headers.get("authorization", "")
            if not auth or auth.split()[-1] != expected:
                log.warning("%s auth failed", self.addr)
                self._send(b"HTTP/1.1 401 Unauthorized\r\n\r\n")
                return

        # Parse request path
        parts = request_line.split()
        if len(parts) < 2:
            self._send(b"HTTP/1.1 400 Bad Request\r\n\r\n")
            return

        path = parts[1]

        if path == "/":
            # Source table request
            log.info("%s serving source table", self.addr)
            body = SOURCE_TABLE.encode()
            resp = (
                f"HTTP/1.1 200 OK\r\n"
                f"Content-Type: text/plain\r\n"
                f"Content-Length: {len(body)}\r\n"
                f"\r\n"
            ).encode() + body
            self._send(resp)
            return

        # Mountpoint stream
        mountpoint = path.lstrip("/")
        valid_mounts = {"MOCK1", "MOCK2", "MOCK_AUTH"}
        if mountpoint not in valid_mounts:
            log.warning("%s unknown mount: %s", self.addr, mountpoint)
            self._send(b"HTTP/1.1 404 Not Found\r\n\r\n")
            return

        log.info("%s streaming %s @ %s msg/sec", self.addr, mountpoint, self.args.rate)
        # ICY 200 OK (like real NTRIP casters)
        self._send(b"ICY 200 OK\r\n\r\n")

        self._stream_rtcm(mountpoint)

    def _stream_rtcm(self, mountpoint):
        interval = 1.0 / self.args.rate
        msg_count = 0

        while self.running:
            msg_count += 1

            # Error injection: bad CRC every 10th message
            if self.args.error == "bad_crc" and msg_count % 10 == 0:
                log.info("%s injecting bad CRC (msg #%d)", self.addr, msg_count)
                self._send(make_bad_crc_frame())
                time.sleep(interval)
                continue

            # Error injection: drop connection
            if self.args.error == "drop" and msg_count > 10:
                log.info("%s dropping connection after %d msgs", self.addr, msg_count)
                return

            # Alternate between 1005 and 1077
            if msg_count % 2 == 1:
                frame = make_rtcm_1005()
            else:
                frame = make_rtcm_1077()

            try:
                self._send(frame)
            except (ConnectionResetError, BrokenPipeError):
                return

            # Check for incoming NMEA from client (GGA)
            try:
                ready, _, _ = select.select([self.conn], [], [], 0)
                if ready:
                    nmea = self.conn.recv(4096)
                    if nmea:
                        log.info("%s << NMEA: %s", self.addr, nmea.decode("ascii", errors="replace").strip())
                    else:
                        log.info("%s client closed", self.addr)
                        return
            except Exception:
                pass

            time.sleep(interval)

    def _send(self, data: bytes):
        self.conn.sendall(data)


# ---------------------------------------------------------------------------
# Server
# ---------------------------------------------------------------------------


def run_server(args):
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((args.host, args.port))
    srv.listen(5)

    log.info("Mock NTRIP caster listening on %s:%d", args.host, args.port)
    log.info("Mountpoints: MOCK1, MOCK2, MOCK_AUTH")
    if args.auth:
        log.info("Auth required: %s", args.auth)
    if args.error:
        log.info("Error injection: %s", args.error)
    log.info("RTCM rate: %s msg/sec", args.rate)
    log.info("")
    log.info("QGC settings:")
    log.info("  Host: %s", args.host if args.host != "0.0.0.0" else "127.0.0.1")
    log.info("  Port: %d", args.port)
    log.info("  Mountpoint: MOCK1")
    if args.auth:
        user, passwd = args.auth.split(":", 1)
        log.info("  Username: %s", user)
        log.info("  Password: %s", passwd)
    log.info("")

    try:
        while True:
            conn, addr = srv.accept()
            log.info("Connection from %s:%d", *addr)
            handler = ClientHandler(conn, addr, args)
            handler.start()
    except KeyboardInterrupt:
        log.info("Shutting down")
    finally:
        srv.close()


def main():
    parser = argparse.ArgumentParser(
        description="Mock NTRIP caster for QGroundControl testing"
    )
    parser.add_argument("--host", default="0.0.0.0", help="Bind address (default: 0.0.0.0)")
    parser.add_argument("--port", type=int, default=2101, help="Port (default: 2101)")
    parser.add_argument("--auth", metavar="USER:PASS", help="Require basic auth (e.g. user:pass)")
    parser.add_argument("--rate", type=float, default=1.0, help="Messages per second (default: 1)")
    parser.add_argument(
        "--error",
        choices=["bad_crc", "drop", "http_401", "http_500"],
        help="Inject errors: bad_crc (every 10th), drop (after 10), http_401, http_500",
    )
    args = parser.parse_args()
    run_server(args)


if __name__ == "__main__":
    main()
