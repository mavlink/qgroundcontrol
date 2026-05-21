#!/usr/bin/env python3
"""Tests for tools/common/patterns.py."""

from __future__ import annotations

from pathlib import Path

import pytest
from common.file_traversal import find_header_files, find_repo_root
from common.patterns import (
    ACTIVE_VEHICLE_DIRECT_PATTERN,
    FACT_MEMBER_PATTERN,
    FACTGROUP_CLASS_PATTERN,
    MAVLINK_MSG_ID_PATTERN,
    PARAM_NAME_PATTERN,
    make_query_pattern,
)


class TestPatterns:
    def test_fact_member_pattern(self) -> None:
        line = "    Fact _speedFact = Fact(0, ...);"
        match = FACT_MEMBER_PATTERN.search(line)
        assert match is not None
        assert match.group(1) == "speed"

    def test_fact_member_pattern_with_spaces(self) -> None:
        line = "Fact  _temperatureFact  =  Fact(..."
        match = FACT_MEMBER_PATTERN.search(line)
        assert match is not None
        assert match.group(1) == "temperature"

    def test_factgroup_class_pattern(self) -> None:
        line = "class VehicleGPSFactGroup : public FactGroup"
        match = FACTGROUP_CLASS_PATTERN.search(line)
        assert match is not None
        assert match.group(1) == "VehicleGPSFactGroup"

    def test_mavlink_msg_id_pattern(self) -> None:
        line = "case MAVLINK_MSG_ID_HEARTBEAT:"
        match = MAVLINK_MSG_ID_PATTERN.search(line)
        assert match is not None
        assert match.group(1) == "HEARTBEAT"

    def test_param_name_pattern(self) -> None:
        line = '            "name": "latitude",'
        match = PARAM_NAME_PATTERN.search(line)
        assert match is not None
        assert match.group(1) == "latitude"

    def test_active_vehicle_direct_pattern(self) -> None:
        line = "activeVehicle()->parameterManager()->getParameter(...);"
        match = ACTIVE_VEHICLE_DIRECT_PATTERN.search(line)
        assert match is not None
        assert match.group(1) == "parameterManager"


class TestMakeQueryPattern:
    def test_make_query_pattern_filters(self) -> None:
        pattern = make_query_pattern(FACT_MEMBER_PATTERN, "speed")
        assert pattern.search("Fact _speedFact = ...") is not None
        assert pattern.search("Fact _groundSpeedFact = ...") is not None
        assert pattern.search("Fact _altitudeFact = ...") is None

    def test_make_query_pattern_case_insensitive(self) -> None:
        pattern = make_query_pattern(FACT_MEMBER_PATTERN, "GPS")
        assert pattern.search("Fact _gpsLatFact = ...") is not None
        assert pattern.search("Fact _GPSLonFact = ...") is not None


class TestLocatorIntegration:
    def test_search_real_codebase(self) -> None:
        repo_root = find_repo_root(Path(__file__))
        src_dir = repo_root / "src"
        if not src_dir.exists():
            pytest.skip("Source directory not found")

        query_pattern = make_query_pattern(FACT_MEMBER_PATTERN, "lat")
        for header in find_header_files(src_dir):
            try:
                content = header.read_text()
            except Exception:
                continue
            if query_pattern.search(content):
                return
        msg = "Should find at least one Fact containing 'lat'"
        raise AssertionError(msg)
