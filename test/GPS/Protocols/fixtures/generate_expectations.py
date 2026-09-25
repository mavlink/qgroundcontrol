#!/usr/bin/env python3
"""Regenerate/check offline C++ expectations with the pinned SEMU decoder tools."""

import argparse
import calendar
import hashlib
import io
import json
import math
from importlib.metadata import distribution
from pathlib import Path

from pynmeagps import NMEAReader
from pyrtcm import RTCMReader
from pysbf2 import SBFReader
from pyubx2 import UBXReader
from synthetic_fixtures import synthetic_fixtures

ROOT = Path(__file__).resolve().parent
PINS = {
    "pyubx2": "9eadeeefc25162206a9200eea1a90bbb8ca315b9",
    "pysbf2": "7ea5e3aecb1349396d47f80938cfe20c461af95b",
    "pynmeagps": "0dd3a69752b6df4172e31255748bf794082845cb",
    "pyrtcm": "620833ee9031207d6181c4818f32b20bab31f9b5",
}


def verify_pins() -> None:
    for package, revision in PINS.items():
        metadata = json.loads(distribution(package).read_text("direct_url.json") or "{}")
        if metadata.get("vcs_info", {}).get("commit_id") != revision:
            raise RuntimeError(f"Install the pinned {package} from fixtures/requirements.txt")


def literal(value) -> str:
    if isinstance(value, bool):
        return str(value).lower()
    if isinstance(value, str):
        return json.dumps(value)
    if isinstance(value, float) and math.isnan(value):
        return "std::numeric_limits<double>::quiet_NaN()"
    return repr(value)


def row(*values) -> str:
    return "    {" + ", ".join(literal(value) for value in values) + "},"


def optional_number(value):
    return math.nan if value == "" else value


def expanded_expectations() -> list[str]:
    lines = [
        "struct Integrity { unsigned offset, size, spoofing, jamming; int noise, agc, indicator, correctionUse, crcFailed; };",
        "inline constexpr Integrity integrity[] = {",
    ]
    stream = io.BytesIO((ROOT / "synthetic-integrity.ubx").read_bytes())
    state = {
        "spoofing": 0,
        "jamming": 0,
        "noise": -1,
        "agc": -1,
        "indicator": -1,
        "correctionUse": -1,
        "crcFailed": -1,
    }
    for raw, msg in UBXReader(stream, quitonerror=2):
        if msg.identity == "NAV-STATUS":
            state["spoofing"] = msg.spoofDetState
        elif msg.identity == "MON-RF":
            state.update(
                jamming=msg.jammingState_01,
                noise=msg.noisePerMS_01,
                agc=msg.agcCnt_01,
                indicator=msg.jamInd_01,
            )
        elif msg.identity == "RXM-RTCM":
            state.update(correctionUse=msg.msgUsed, crcFailed=msg.crcFailed)
        lines.append(row(stream.tell() - len(raw), len(raw), *state.values()))
    lines += [
        "};",
        "struct Utc { unsigned offset, size; unsigned long long microseconds; };",
        "inline constexpr Utc utc[] = {",
    ]
    stream = io.BytesIO((ROOT / "synthetic-timeutc.ubx").read_bytes())
    for raw, msg in UBXReader(stream, quitonerror=2):
        seconds = calendar.timegm((msg.year, msg.month, msg.day, msg.hour, msg.min, msg.sec))
        microseconds = seconds * 1000000 + msg.nano // 1000 if msg.validUTC else 0
        lines.append(row(stream.tell() - len(raw), len(raw), microseconds))
    lines += [
        "};",
        "struct Gga { unsigned offset, size; const char* talker; double latitude, longitude, altitude, geoid, hdop; unsigned quality; int satellites; };",
        "inline constexpr Gga ggas[] = {",
    ]
    stream = io.BytesIO((ROOT / "synthetic-gga.nmea").read_bytes())
    for raw, msg in NMEAReader(stream, quitonerror=2):
        lines.append(
            row(
                stream.tell() - len(raw),
                len(raw),
                msg.talker,
                *(
                    optional_number(value)
                    for value in (msg.lat, msg.lon, msg.alt, msg.sep, msg.HDOP)
                ),
                msg.quality,
                -1 if msg.numSV == "" else msg.numSV,
            )
        )
    lines += [
        "};",
        "struct Gst { unsigned offset, size; const char* talker; double horizontal, vertical; };",
        "inline constexpr Gst gsts[] = {",
    ]
    stream = io.BytesIO((ROOT / "synthetic-gst.nmea").read_bytes())
    for raw, msg in NMEAReader(stream, quitonerror=2):
        horizontal = math.hypot(optional_number(msg.stdLat), optional_number(msg.stdLong))
        lines.append(
            row(
                stream.tell() - len(raw),
                len(raw),
                msg.talker,
                horizontal,
                optional_number(msg.stdAlt),
            )
        )
    lines += [
        "};",
        "struct SbfEpoch { const char* filename; double latitude, longitude, ellipsoid, msl, horizontalAccuracy, verticalAccuracy, north, east, down, course, hdop, vdop, speedAccuracy, heading, headingAccuracy; unsigned mode, error, satellites; bool twoDimensional, velocityAvailable; };",
        "inline constexpr SbfEpoch sbfEpochs[] = {",
    ]
    for filename in (
        "synthetic-valid.sbf",
        "synthetic-zero.sbf",
        "synthetic-unavailable.sbf",
        "synthetic-error.sbf",
    ):
        messages = {
            msg.identity: msg
            for _, msg in SBFReader(io.BytesIO((ROOT / filename).read_bytes()), quitonerror=2)
        }
        pvt = messages["PVTGeodetic"]
        dop = messages.get("DOP")
        cov = messages.get("VelCovGeodetic")
        att = messages.get("AttEuler")
        att_cov = messages.get("AttCovEuler")
        speed_accuracy = math.nan
        if cov and not cov.Error:
            speed_accuracy = math.sqrt(max(cov.Cov_VnVn, cov.Cov_VeVe, cov.Cov_VuVu))
        # Convert centimetres and two-sigma uncertainty to the report's RMS metres.
        lines.append(
            row(
                filename,
                math.degrees(pvt.Latitude),
                math.degrees(pvt.Longitude),
                pvt.Height,
                pvt.Height - pvt.Undulation,
                math.nan if pvt.HAccuracy == 65535 else pvt.HAccuracy / 200,
                math.nan if pvt.VAccuracy == 65535 else pvt.VAccuracy / 200,
                pvt.Vn,
                pvt.Ve,
                -pvt.Vu,
                math.nan if pvt.COG == -2e10 else math.radians(pvt.COG),
                dop.HDOP / 100 if dop and dop.HDOP != 65535 else math.nan,
                dop.VDOP / 100 if dop and dop.VDOP != 65535 else math.nan,
                speed_accuracy,
                math.radians(math.remainder(att.Heading, 360))
                if att and not att.Error
                else math.nan,
                math.radians(math.sqrt(att_cov.Cov_HeadHead))
                if att_cov and not att_cov.Error
                else math.nan,
                pvt.Type,
                pvt.Error,
                pvt.NrSV,
                bool(getattr(pvt, "2D")),
                not pvt.Error and all(value != -2e10 for value in (pvt.Vn, pvt.Ve, pvt.Vu)),
            )
        )
    lines += [
        "};",
        "struct SbfTime { unsigned offset, size, tow, week; };",
        "inline constexpr SbfTime invalidSbfTimes[] = {",
    ]
    stream = io.BytesIO((ROOT / "synthetic-invalid-time.sbf").read_bytes())
    for raw, msg in SBFReader(stream, quitonerror=2):
        if msg.WNc != 65535 and msg.TOW < 604800000:
            raise RuntimeError("Expected unavailable or out-of-week SBF time")
        lines.append(row(stream.tell() - len(raw), len(raw), msg.TOW, msg.WNc))
    lines.append("};")
    return lines


def generate() -> str:
    lines = [
        "// Generated by generate_expectations.py; do not edit.",
        "#pragma once",
        "",
        "#include <limits>",
        "",
        "// clang-format off",
        "namespace GPSFixture {",
    ]
    for filename in (
        "navigation.ubx",
        "mixed.gps",
        "geodetic.sbf",
        "attitude.sbf",
        "relative.ubx",
        "gga.nmea",
        *synthetic_fixtures(),
    ):
        digest = hashlib.sha256((ROOT / filename).read_bytes()).hexdigest()
        lines.append(f"// {filename}: sha256 {digest}")
    lines += [
        "struct Position { unsigned offset, size; double latitude, longitude, altitude; };",
        "inline constexpr Position positions[] = {",
    ]
    stream = io.BytesIO((ROOT / "navigation.ubx").read_bytes())
    for raw, msg in UBXReader(stream, quitonerror=2):
        if msg.identity in ("NAV-PVT", "NAV-POSLLH"):
            lines.append(
                f"    {{{stream.tell() - len(raw)}, {len(raw)}, {msg.lat!r}, {msg.lon!r}, {msg.hMSL / 1000!r}}},"
            )
    lines += [
        "};",
        "struct Satellite { unsigned gnss, id, signal; int elevation, azimuth; bool used; };",
        "inline constexpr Satellite satellites[] = {",
    ]
    sat = UBXReader.parse((ROOT / "nav-sat.ubx").read_bytes())
    for i in range(1, sat.numSvs + 1):
        values = [
            getattr(sat, f"{key}_{i:02}")
            for key in ("gnssId", "svId", "cno", "elev", "azim", "svUsed")
        ]
        lines.append(
            "    {"
            + ", ".join(str(v) for v in values[:-1])
            + ", "
            + str(bool(values[-1])).lower()
            + "},"
        )
    lines += [
        "};",
        "struct Correction { unsigned offset, size, id; };",
        "inline constexpr Correction corrections[] = {",
    ]
    stream = io.BytesIO((ROOT / "mixed.gps").read_bytes())
    for raw, _ in UBXReader(stream, quitonerror=2):
        if raw.startswith(b"\xd3"):
            msg = RTCMReader.parse(raw)
            lines.append(f"    {{{stream.tell() - len(raw)}, {len(raw)}, {msg.identity}}},")
    lines.append("};")
    messages = {
        msg.identity: msg
        for _, msg in SBFReader(io.BytesIO((ROOT / "geodetic.sbf").read_bytes()), quitonerror=2)
    }
    covariance = messages["VelCovGeodetic"]
    lines.append(
        f"inline constexpr double speedVariance = {max(covariance.Cov_VnVn, covariance.Cov_VeVe, covariance.Cov_VuVu)!r};"
    )
    for _, msg in SBFReader(io.BytesIO((ROOT / "attitude.sbf").read_bytes()), quitonerror=2):
        if msg.Error != 128:
            raise RuntimeError("Expected receiver attitude-unavailable fixture")
    relative = UBXReader.parse((ROOT / "relative.ubx").read_bytes())
    lines.append(
        f"inline constexpr bool relativeValid = {str(bool(relative.relPosValid)).lower()};"
    )
    gga = NMEAReader.parse((ROOT / "gga.nmea").read_bytes())
    for name, value in (
        ("ggaLatitude", gga.lat),
        ("ggaLongitude", gga.lon),
        ("ggaAltitude", gga.alt),
        ("ggaSatellites", gga.numSV),
    ):
        lines.append(f"inline constexpr double {name} = {value!r};")
    lines += expanded_expectations()
    lines += ["}  // namespace GPSFixture", "", "// clang-format on", ""]
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check", action="store_true", help="fail if fixtures, fuzz seeds, or expectations differ"
    )
    args = parser.parse_args()
    verify_pins()
    for filename, data in synthetic_fixtures().items():
        for path in (ROOT / filename, ROOT.parent / "corpus" / filename):
            if args.check:
                if not path.exists() or path.read_bytes() != data:
                    raise SystemExit(f"{path} differs; regenerate with the pinned tools")
            else:
                path.write_bytes(data)
    output = ROOT / "GPSFixtureExpectations.h"
    expected = generate()
    if args.check:
        if output.read_text() != expected:
            raise SystemExit("Fixture expectations differ; regenerate with the pinned tools")
        print("Pinned fixture expectations match")
    else:
        output.write_text(expected)


if __name__ == "__main__":
    main()
