#!/usr/bin/env python3
"""Contracts for reusable source-code patterns."""

from __future__ import annotations

from common.patterns import (
    ACTIVE_VEHICLE_DIRECT_PATTERN,
    FACT_MEMBER_PATTERN,
    FACTGROUP_CLASS_PATTERN,
    MAVLINK_MSG_ID_PATTERN,
    PARAM_NAME_PATTERN,
    make_query_pattern,
)


def test_patterns_extract_expected_identifiers() -> None:
    cases = [
        (FACT_MEMBER_PATTERN, "Fact _speedFact = Fact(0, ...);", "speed"),
        (FACT_MEMBER_PATTERN, "Fact  _temperatureFact  =  Fact(...", "temperature"),
        (
            FACTGROUP_CLASS_PATTERN,
            "class VehicleGPSFactGroup : public FactGroup",
            "VehicleGPSFactGroup",
        ),
        (MAVLINK_MSG_ID_PATTERN, "case MAVLINK_MSG_ID_HEARTBEAT:", "HEARTBEAT"),
        (PARAM_NAME_PATTERN, '"name": "latitude",', "latitude"),
        (ACTIVE_VEHICLE_DIRECT_PATTERN, "activeVehicle()->parameterManager();", "parameterManager"),
    ]
    for pattern, source, expected in cases:
        match = pattern.search(source)
        assert match is not None
        assert match.group(1) == expected


def test_query_pattern_filters_case_insensitively() -> None:
    speed = make_query_pattern(FACT_MEMBER_PATTERN, "speed")
    assert speed.search("Fact _speedFact = ...")
    assert speed.search("Fact _groundSpeedFact = ...")
    assert not speed.search("Fact _altitudeFact = ...")

    gps = make_query_pattern(FACT_MEMBER_PATTERN, "GPS")
    assert gps.search("Fact _gpsLatFact = ...")
    assert gps.search("Fact _GPSLonFact = ...")
