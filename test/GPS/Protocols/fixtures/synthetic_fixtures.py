"""Deterministic, invented test data serialized by the pinned independent oracles."""

from pynmeagps import NMEAMessage
from pysbf2 import SBFMessage
from pyubx2 import GET, UBXMessage


def synthetic_fixtures() -> dict[str, bytes]:
    def ubx(group, name, **fields):
        return UBXMessage(group, name, GET, **fields).serialize()

    def nmea(talker, name, **fields):
        return NMEAMessage(talker, name, GET, time="120000.125", **fields).serialize()

    def sbf(name, **fields):
        return SBFMessage(name, **{"TOW": 123000, "WNc": 2200, **fields}).serialize()

    pvt = {
        "Type": 4,
        "Latitude": 0.125,
        "Longitude": -0.25,
        "Height": 123.5,
        "Undulation": -12.25,
        "Vn": -3.0,
        "Ve": 4.0,
        "Vu": 0.5,
        "COG": 270.0,
        "NrSV": 17,
        "HAccuracy": 120,
        "VAccuracy": 340,
    }
    dop = {"HDOP": 125, "VDOP": 250}
    covariance = {"Cov_VnVn": 0.04, "Cov_VeVe": 0.09, "Cov_VuVu": 0.16}
    attitude = {"Mode": 1, "Heading": 270.0}
    attitude_covariance = {"Cov_HeadHead": 2.25}

    def epoch(position, dilution, velocity_covariance, euler, euler_covariance):
        return b"".join(
            sbf(name, **fields)
            for name, fields in (
                ("PVTGeodetic", position),
                ("DOP", dilution),
                ("VelCovGeodetic", velocity_covariance),
                ("AttEuler", euler),
                ("AttCovEuler", euler_covariance),
            )
        )

    utc = {
        "iTOW": 123000,
        "year": 2024,
        "month": 3,
        "day": 1,
        "hour": 0,
        "min": 0,
        "sec": 0,
        "nano": -250000000,
        "validTOW": 1,
        "validWKN": 1,
        "validUTC": 1,
    }
    gga = {
        "lat": 0.0,
        "lon": 0.0,
        "quality": 1,
        "numSV": 0,
        "HDOP": 0.0,
        "alt": 0.0,
        "altUnit": "M",
        "sep": 0.0,
        "sepUnit": "M",
        "diffAge": "",
        "diffStation": "",
    }
    return {
        "synthetic-integrity.ubx": b"".join(
            (
                ubx("NAV", "NAV-STATUS", iTOW=123000, gpsFix=3, spoofDetState=2),
                ubx(
                    "MON",
                    "MON-RF",
                    version=0,
                    nBlocks=2,
                    blockId_01=0,
                    jammingState_01=2,
                    noisePerMS_01=1234,
                    agcCnt_01=5678,
                    jamInd_01=91,
                    blockId_02=1,
                    jammingState_02=1,
                    noisePerMS_02=9012,
                    agcCnt_02=3456,
                    jamInd_02=17,
                ),
                ubx("RXM", "RXM-RTCM", version=2, msgUsed=2, crcFailed=0, msgType=1077),
                ubx("RXM", "RXM-RTCM", version=2, msgUsed=1, crcFailed=1, msgType=1087),
            )
        ),
        "synthetic-timeutc.ubx": b"".join(
            ubx("NAV", "NAV-TIMEUTC", **{**utc, **changes})
            for changes in ({}, {"validUTC": 0}, {"nano": 125000000})
        ),
        "synthetic-gga.nmea": b"".join(
            nmea(talker, "GGA", **{**gga, **changes})
            for talker, changes in (
                ("GN", {}),
                (
                    "GA",
                    {
                        "lat": 12.5,
                        "lon": -45.25,
                        "quality": 4,
                        "numSV": "",
                        "HDOP": "",
                        "alt": "",
                        "sep": "",
                    },
                ),
                (
                    "GL",
                    {
                        "lat": -12.5,
                        "lon": -45.25,
                        "quality": 5,
                        "numSV": 12,
                        "alt": -15.5,
                        "sep": -30.25,
                    },
                ),
                (
                    "GB",
                    {"lat": 12.5, "lon": 45.25, "quality": 2, "numSV": 8, "HDOP": 0.75, "sep": ""},
                ),
                ("GP", {"quality": 0}),
                ("GN", {"lat": "", "lon": "", "quality": 0, "numSV": ""}),
            )
        ),
        "synthetic-gst.nmea": b"".join(
            nmea(talker, "GST", stdLat=latitude, stdLong=longitude, stdAlt=altitude)
            for talker, latitude, longitude, altitude in (
                ("GN", 3.0, 4.0, 12.0),
                ("GA", 0.0, 0.0, 0.0),
                ("GB", "", 4.0, ""),
            )
        ),
        "synthetic-valid.sbf": epoch(pvt, dop, covariance, attitude, attitude_covariance),
        "synthetic-zero.sbf": epoch(
            {**dict.fromkeys(pvt, 0), "Type": 1, "2D": 1},
            dict.fromkeys(dop, 0),
            dict.fromkeys(covariance, 0),
            {"Mode": 1, "Heading": 0.0},
            {"Cov_HeadHead": 0.0},
        ),
        "synthetic-unavailable.sbf": epoch(
            {
                **pvt,
                "Type": 5,
                "Vn": -2e10,
                "COG": -2e10,
                "NrSV": 255,
                "HAccuracy": 65535,
                "VAccuracy": 65535,
            },
            {"HDOP": 65535, "VDOP": 65535},
            {"Error": 1, "Cov_VnVn": -2e10, "Cov_VeVe": -2e10, "Cov_VuVu": -2e10},
            {"Error": 128, "Mode": 0, "Heading": -2e10},
            {"Error": 128, "Cov_HeadHead": -2e10},
        ),
        "synthetic-error.sbf": sbf("PVTGeodetic", **{**pvt, "Error": 3, "NrSV": 0}),
        "synthetic-invalid-time.sbf": b"".join(
            sbf("PVTGeodetic", **pvt, **time)
            for time in ({"WNc": 65535}, {"TOW": 4294967295}, {"TOW": 604800000})
        ),
    }
