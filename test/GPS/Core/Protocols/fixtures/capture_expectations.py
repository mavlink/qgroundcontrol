"""Receiver and caster captures: where each slice comes from, and what the pinned tools decode from it."""

import hashlib
import io
import math
import re
import subprocess
from pathlib import Path

from pynmeagps import NMEAReader
from pynmeagps.nmeahelpers import dmm2ddd
from pyrtcm import RTCMReader
from pyubx2 import UBXReader

ROOT = Path(__file__).resolve().parent
NTRIP_ROOT = ROOT.parents[2] / "NTRIP" / "fixtures"
PROTOCOL_HEADER = ROOT / "GPSCaptureExpectations.h"
NTRIP_HEADER = NTRIP_ROOT / "NTRIPCaptureExpectations.h"

# The gpsd commit its slices were taken at; pyubx2 and pyrtcm slices come from the pinned oracle commits.
GPSD_COMMIT = "092e62282f7390840b272692bc258853643b072c"

# Capture: its directory, then for a slice of a public log the upstream project, the file in it, the offset and the
# length. A capture with its directory alone is no such slice, as one a user sent: SOURCES.md says where it comes
# from, and its digest in the expectations header pins its bytes.
CAPTURES: dict[str, tuple[Path] | tuple[Path, str, str, int, int]] = {
    "gpsd-zed-f9t-base.cap": (ROOT, "gpsd", "test/daemon/ublox-zed-f9t-rtcm3.log", 327, 3216),
    "gpsd-zed-f9p-rtk.cap": (ROOT, "gpsd", "test/daemon/ublox-zed-f9p-rtk.log", 462, 882),
    "gpsd-neo-m8p-rover.cap": (ROOT, "gpsd", "test/daemon/ublox-neo-m8p.log", 881, 534),
    "pyubx2-zed-f9p-session.cap": (
        ROOT,
        "pyubx2",
        "tests/ucenter-ZEDF9P-configdebug.log",
        14098,
        1409,
    ),
    "pyubx2-zed-f9p-rejected.cap": (
        ROOT,
        "pyubx2",
        "tests/ucenter-ZEDF9P-configdebug.log",
        19063,
        259,
    ),
    "gpsd-ac12-pashr.cap": (ROOT, "gpsd", "test/daemon/ac12_binary.log", 510, 1642),
    "gpsd-trimble-rtk.cap": (ROOT, "gpsd", "test/daemon/nmea-rtk.log", 18587, 1262),
    "gpsd-sapos-sourcetable.cap": (
        NTRIP_ROOT,
        "gpsd",
        "test/daemon/ntrip_sourcetable.log",
        280,
        2517,
    ),
    "gpsd-orgn-caster.cap": (NTRIP_ROOT, "gpsd", "test/daemon/rtcm3.bndm.log", 276, 1163),
    "pyrtcm-ntrip-uscl.cap": (
        NTRIP_ROOT,
        "pyrtcm",
        "tests/pygpsdata-NTRIP-USCL00CHL0.log",
        0,
        4606,
    ),
}

UBX_CAPTURES = (
    "gpsd-zed-f9t-base.cap",
    "gpsd-zed-f9p-rtk.cap",
    "gpsd-neo-m8p-rover.cap",
    "pyubx2-zed-f9p-session.cap",
    "pyubx2-zed-f9p-rejected.cap",
)
NMEA_CAPTURES = ("gpsd-ac12-pashr.cap", "gpsd-trimble-rtk.cap")
CASTER_CAPTURES = ("gpsd-orgn-caster.cap", "pyrtcm-ntrip-uscl.cap")

SENTENCE = re.compile(rb"\$[A-Z]{5},[^\r\n$]*\*[0-9A-F]{2}\r\n")


def capture(name: str) -> bytes:
    return (CAPTURES[name][0] / name).read_bytes()


def literal(value) -> str:
    if isinstance(value, bool):
        return str(value).lower()
    if isinstance(value, str):
        return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'
    if isinstance(value, float) and math.isnan(value):
        return "std::numeric_limits<double>::quiet_NaN()"
    return repr(value)


def table(declaration: str, name: str, rows: list[tuple]) -> list[str]:
    lines = [declaration, f"inline constexpr {declaration.split()[1]} {name}[] = {{"]
    lines += ["    {" + ", ".join(literal(value) for value in row) + "}," for row in rows]
    return [*lines, "};"]


def geodetic(x: float, y: float, z: float) -> tuple[float, float, float]:
    """WGS 84 latitude and longitude in degrees and ellipsoidal height of an ECEF position."""
    a = 6378137.0
    f = 1 / 298.257223563
    e2 = f * (2 - f)
    p = math.hypot(x, y)
    latitude = math.atan2(z, p * (1 - e2))
    height = 0.0
    for _ in range(8):
        n = a / math.sqrt(1 - e2 * math.sin(latitude) ** 2)
        height = p / math.cos(latitude) - n
        latitude = math.atan2(z, p * (1 - e2 * n / (n + height)))
    return math.degrees(latitude), math.degrees(math.atan2(y, x)), height


def text(field: bytes) -> str:
    return field.split(b"\0", 1)[0].decode("ascii")


def ubx_messages(name: str):
    """Every UBX and RTCM3 message of a capture, with its offset."""
    stream = io.BytesIO(capture(name))
    for raw, msg in UBXReader(stream, quitonerror=2):
        if raw.startswith(b"\xd3"):
            msg = RTCMReader.parse(raw)
        yield stream.tell() - len(raw), raw, msg


def rtcm_messages(name: str):
    stream = io.BytesIO(capture(name))
    for raw, msg in RTCMReader(stream, quitonerror=2):
        yield stream.tell() - len(raw), raw, msg


def sentences(name: str):
    """Every NMEA sentence of a capture whose checksum the pinned reader accepts, with its offset."""
    for found in SENTENCE.finditer(capture(name)):
        yield found.start(), found.group(), NMEAReader.parse(found.group())


def station_rows(name: str, messages) -> list[tuple]:
    rows = []
    for offset, raw, msg in messages:
        if msg.identity in ("1005", "1006"):
            antenna = msg.DF028 if msg.identity == "1006" else math.nan
            rows.append(
                (
                    name,
                    offset,
                    len(raw),
                    int(msg.identity),
                    *geodetic(msg.DF025, msg.DF026, msg.DF027),
                    antenna,
                )
            )
    return rows


def correction_rows(name: str, messages) -> list[tuple]:
    return [
        (name, offset, len(raw), int(msg.identity.split("_")[0]))
        for offset, raw, msg in messages
        if raw.startswith(b"\xd3")
    ]


def digests(names) -> list[str]:
    return [f"// {name}: sha256 {hashlib.sha256(capture(name)).hexdigest()}" for name in names]


def protocol_expectations() -> str:
    epochs, surveys, versions, acknowledgements, corrections, stations = [], [], [], [], [], []
    for name in UBX_CAPTURES:
        messages = list(ubx_messages(name))
        corrections += correction_rows(name, messages)
        stations += station_rows(name, messages)
        for offset, raw, msg in messages:
            if msg.identity == "NAV-PVT":
                epochs.append(
                    (
                        name,
                        offset,
                        len(raw),
                        msg.lat,
                        msg.lon,
                        msg.hMSL / 1000,
                        msg.height / 1000,
                        msg.fixType,
                        msg.carrSoln,
                        msg.numSV,
                        bool(msg.gnssFixOk),
                        bool(msg.diffSoln),
                    )
                )
            elif msg.identity == "NAV-SVIN":
                # meanAcc is in 0.1 mm.
                surveys.append(
                    (
                        name,
                        offset,
                        len(raw),
                        msg.dur,
                        msg.meanAcc / 10000,
                        bool(msg.active),
                        bool(msg.valid),
                    )
                )
            elif msg.identity == "MON-VER" and len(raw) > 8:
                extensions = [
                    text(value) for key, value in vars(msg).items() if key.startswith("extension_")
                ]
                field = {
                    key: value for key, _, value in (item.partition("=") for item in extensions)
                }
                versions.append(
                    (
                        name,
                        offset,
                        len(raw),
                        text(msg.swVersion),
                        text(msg.hwVersion),
                        field["FWVER"],
                        field["PROTVER"],
                        field["MOD"],
                    )
                )
            elif msg.identity in ("ACK-ACK", "ACK-NAK"):
                acknowledgements.append(
                    (name, offset, msg.clsID, msg.msgID, msg.identity == "ACK-ACK")
                )
    fixes, positions = [], []
    for name in NMEA_CAPTURES:
        latest = {}
        for offset, raw, msg in sentences(name):
            if msg.msgID == "GGA":
                latest[msg.payload[0]] = (msg.lat, msg.lon)
                fixes.append(
                    (
                        name,
                        offset,
                        len(raw),
                        msg.talker,
                        msg.lat,
                        msg.lon,
                        msg.alt,
                        msg.sep,
                        msg.HDOP,
                        msg.quality,
                        msg.numSV,
                    )
                )
            elif msg.talker == "P" and msg.payload[0] == "POS":
                # $PASHR,POS,d1,d2,m3,m4,c5,m6,c7,f8,f9,f10,f11,f12,f13,f14,f15,f16,s17: the pinned reader checks the
                # checksum and splits the fields, which it does not name.
                mode, used, time, lat, ns, lon, ew, height, _, course, knots, _, _, hdop, vdop = (
                    msg.payload[1:16]
                )
                latitude = dmm2ddd(lat) * (-1 if ns == "S" else 1)
                longitude = dmm2ddd(lon) * (-1 if ew == "W" else 1)
                # The receiver's GGA of the same fix, which the reader decodes by name, has the same coordinates.
                if latest[time] != (latitude, longitude):
                    raise RuntimeError(f"{name}: $PASHR,POS at {offset} differs from its GGA")
                positions.append(
                    (
                        name,
                        offset,
                        len(raw),
                        latitude,
                        longitude,
                        float(height),
                        float(course),
                        float(knots),
                        float(hdop),
                        float(vdop),
                        int(mode),
                        int(used),
                    )
                )
    lines = [
        "// Generated by generate_expectations.py; do not edit.",
        "#pragma once",
        "",
        "#include <limits>",
        "",
        "// clang-format off",
        "namespace GPSCapture {",
        *digests((*UBX_CAPTURES, *NMEA_CAPTURES)),
        *table(
            "struct Epoch { const char* file; unsigned offset, size; double latitude, longitude, altitudeMsl, altitudeEllipsoid; unsigned fixType, carrierSolution, satellites; bool fixOk, differential; };",
            "epochs",
            epochs,
        ),
        *table(
            "struct Survey { const char* file; unsigned offset, size, duration; double meanAccuracy; bool active, valid; };",
            "surveys",
            surveys,
        ),
        *table(
            "struct Version { const char* file; unsigned offset, size; const char* software; const char* hardware; const char* firmware; const char* protocol; const char* module; };",
            "versions",
            versions,
        ),
        *table(
            "struct Acknowledgement { const char* file; unsigned offset, messageClass, messageId; bool acknowledged; };",
            "acknowledgements",
            acknowledgements,
        ),
        *table(
            "struct Correction { const char* file; unsigned offset, size, id; };",
            "corrections",
            corrections,
        ),
        *table(
            "struct Station { const char* file; unsigned offset, size, id; double latitude, longitude, height, antennaHeight; };",
            "stations",
            stations,
        ),
        *table(
            "struct Gga { const char* file; unsigned offset, size; const char* talker; double latitude, longitude, altitude, geoid, hdop; unsigned quality, satellites; };",
            "ggas",
            fixes,
        ),
        *table(
            "struct AshtechPosition { const char* file; unsigned offset, size; double latitude, longitude, altitudeEllipsoid, course, speedKnots, hdop, vdop; unsigned mode, satellites; };",
            "ashtechPositions",
            positions,
        ),
        "}  // namespace GPSCapture",
        "",
        "// clang-format on",
        "",
    ]
    return "\n".join(lines)


def ntrip_expectations() -> str:
    # gpsd keeps the caster's response with LF line endings; its Content-Length counts the CRLF the caster sent.
    head, _, body = capture("gpsd-sapos-sourcetable.cap").partition(b"\n\n")
    length = next(
        int(line.split(b":")[1])
        for line in head.split(b"\n")
        if line.startswith(b"Content-Length:")
    )
    if len(body) + body.count(b"\n") != length:
        raise RuntimeError(
            "The source table does not have its Content-Length with CRLF line endings"
        )
    rows = [line.split(";") for line in body.decode("ascii").split("\n")]
    # STR;mountpoint;identifier;format;format-details;carrier;nav-system;network;country;latitude;longitude;nmea;
    # solution;generator;compression;authentication;fee;bitrate;misc
    mountpoints = [
        (row[1], row[3], row[6], row[8], float(row[9]), float(row[10]), int(row[17]))
        for row in rows
        if row[0] == "STR"
    ]
    corrections, stations = [], []
    for name in CASTER_CAPTURES:
        messages = list(rtcm_messages(name))
        corrections += correction_rows(name, messages)
        stations += station_rows(name, messages)
    lines = [
        "// Generated by test/GPS/Core/Protocols/fixtures/generate_expectations.py; do not edit.",
        "#pragma once",
        "",
        "#include <limits>",
        "",
        "// clang-format off",
        "namespace NTRIPCapture {",
        *digests(("gpsd-sapos-sourcetable.cap", *CASTER_CAPTURES)),
        f"inline constexpr unsigned sourceTableContentLength = {length};",
        f"inline constexpr unsigned sourceTableRows = {sum(row[0] in ('CAS', 'NET', 'STR') for row in rows)};",
        *table(
            "struct Mountpoint { const char* mountpoint; const char* format; const char* navSystem; const char* country; double latitude, longitude; int bitrate; };",
            "mountpoints",
            mountpoints,
        ),
        *table(
            "struct Correction { const char* file; unsigned offset, size, id; };",
            "corrections",
            corrections,
        ),
        *table(
            "struct Station { const char* file; unsigned offset, size, id; double latitude, longitude, height, antennaHeight; };",
            "stations",
            stations,
        ),
        "}  // namespace NTRIPCapture",
        "",
        "// clang-format on",
        "",
    ]
    return "\n".join(lines)


def check_sources(upstream: Path, check: bool, commits: dict[str, str]) -> None:
    """Compares each capture that has an upstream source with its slice of the checkouts in @a upstream, or writes it.

    A capture without one only has to be there.
    """
    for project, commit in commits.items():
        head = subprocess.run(
            ["git", "-C", str(upstream / project), "rev-parse", "HEAD"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        if head != commit:
            raise SystemExit(f"{upstream / project} is at {head}; check out {commit}")
    for name, source in CAPTURES.items():
        if len(source) == 1:
            if not (source[0] / name).is_file():
                raise SystemExit(f"{name} is missing, and has no upstream source to take it from")
            continue
        directory, project, path, offset, length = source
        data = (upstream / project / path).read_bytes()[offset : offset + length]
        if len(data) != length:
            raise SystemExit(f"{project} {path} is shorter than {offset + length} bytes")
        if not check:
            (directory / name).write_bytes(data)
        elif (directory / name).read_bytes() != data:
            raise SystemExit(
                f"{name} differs from bytes {offset} to {offset + length} of {project} {path}"
            )


def generate(check: bool, upstream: Path | None, pins: dict[str, str]) -> None:
    """Writes or checks both expectation headers, and the captures too when @a upstream has the upstream checkouts."""
    if upstream:
        check_sources(
            upstream,
            check,
            {"gpsd": GPSD_COMMIT, "pyubx2": pins["pyubx2"], "pyrtcm": pins["pyrtcm"]},
        )
    for header, expected in (
        (PROTOCOL_HEADER, protocol_expectations()),
        (NTRIP_HEADER, ntrip_expectations()),
    ):
        if not check:
            header.write_text(expected)
        elif not header.exists() or header.read_text() != expected:
            raise SystemExit(f"{header.name} differs; regenerate with the pinned tools")
