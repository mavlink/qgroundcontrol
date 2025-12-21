#!/usr/bin/env python3
"""Tests for common utilities."""

import sys
from pathlib import Path

import pytest

TOOLS_DIR = Path(__file__).parent.parent

# Add tools to path for imports
sys.path.insert(0, str(TOOLS_DIR))

from common.patterns import (
    FACT_MEMBER_PATTERN,
    FACTGROUP_CLASS_PATTERN,
    MAVLINK_MSG_ID_PATTERN,
    PARAM_NAME_PATTERN,
    ACTIVE_VEHICLE_DIRECT_PATTERN,
    make_query_pattern,
)
from common.file_traversal import (
    find_repo_root,
    should_skip_path,
    DEFAULT_SKIP_DIRS,
)


class TestPatterns:
    """Test regex patterns."""

    def test_fact_member_pattern(self):
        """Should match Fact member declarations."""
        line = "    Fact _speedFact = Fact(0, ...);"
        match = FACT_MEMBER_PATTERN.search(line)
        assert match is not None
        assert match.group(1) == "speed"

    def test_fact_member_pattern_with_spaces(self):
        """Should handle various spacing."""
        line = "Fact  _temperatureFact  =  Fact(..."
        match = FACT_MEMBER_PATTERN.search(line)
        assert match is not None
        assert match.group(1) == "temperature"

    def test_factgroup_class_pattern(self):
        """Should match FactGroup class declarations."""
        line = "class VehicleGPSFactGroup : public FactGroup"
        match = FACTGROUP_CLASS_PATTERN.search(line)
        assert match is not None
        assert match.group(1) == "VehicleGPSFactGroup"

    def test_mavlink_msg_id_pattern(self):
        """Should match MAVLINK_MSG_ID constants."""
        line = "case MAVLINK_MSG_ID_HEARTBEAT:"
        match = MAVLINK_MSG_ID_PATTERN.search(line)
        assert match is not None
        assert match.group(1) == "HEARTBEAT"

    def test_param_name_pattern(self):
        """Should match JSON parameter names."""
        line = '            "name": "latitude",'
        match = PARAM_NAME_PATTERN.search(line)
        assert match is not None
        assert match.group(1) == "latitude"

    def test_active_vehicle_direct_pattern(self):
        """Should match activeVehicle()->method() calls."""
        line = "activeVehicle()->parameterManager()->getParameter(...);"
        match = ACTIVE_VEHICLE_DIRECT_PATTERN.search(line)
        assert match is not None
        assert match.group(1) == "parameterManager"


class TestMakeQueryPattern:
    """Test query pattern creation."""

    def test_make_query_pattern_filters(self):
        """Should create pattern that filters by query."""
        pattern = make_query_pattern(FACT_MEMBER_PATTERN, "speed")

        # Should match facts containing 'speed'
        assert pattern.search("Fact _speedFact = ...") is not None
        assert pattern.search("Fact _groundSpeedFact = ...") is not None

        # Should not match facts without 'speed'
        assert pattern.search("Fact _altitudeFact = ...") is None

    def test_make_query_pattern_case_insensitive(self):
        """Query should be case-insensitive."""
        pattern = make_query_pattern(FACT_MEMBER_PATTERN, "GPS")
        assert pattern.search("Fact _gpsLatFact = ...") is not None
        assert pattern.search("Fact _GPSLonFact = ...") is not None


class TestFileTraversal:
    """Test file traversal utilities."""

    def test_find_repo_root(self):
        """Should find .git directory."""
        root = find_repo_root(Path(__file__))
        assert (root / ".git").exists()

    def test_should_skip_path_build(self):
        """Should skip build directories."""
        assert should_skip_path(Path("/project/build/file.cpp"))
        assert should_skip_path(Path("/project/src/build/nested.h"))

    def test_should_skip_path_libs(self):
        """Should skip libs directories."""
        assert should_skip_path(Path("/project/libs/external/file.h"))

    def test_should_skip_path_cache(self):
        """Should skip cache directories."""
        assert should_skip_path(Path("/project/.cache/file.cpp"))

    def test_should_not_skip_src(self):
        """Should not skip src directories."""
        assert not should_skip_path(Path("/project/src/file.cpp"))
        assert not should_skip_path(Path("/project/src/Vehicle/Vehicle.h"))

    def test_default_skip_dirs_contents(self):
        """DEFAULT_SKIP_DIRS should contain expected entries."""
        assert "build" in DEFAULT_SKIP_DIRS
        assert "libs" in DEFAULT_SKIP_DIRS
        assert "node_modules" in DEFAULT_SKIP_DIRS
        assert ".git" in DEFAULT_SKIP_DIRS


class TestLocatorIntegration:
    """Integration tests for locator using patterns."""

    def test_search_real_codebase(self):
        """Should find real Facts in QGC codebase."""
        from common.file_traversal import find_header_files

        repo_root = find_repo_root(Path(__file__))
        src_dir = repo_root / "src"

        if not src_dir.exists():
            pytest.skip("Source directory not found")

        # Search for a known Fact pattern
        query_pattern = make_query_pattern(FACT_MEMBER_PATTERN, "lat")
        found = False

        for header in find_header_files(src_dir):
            try:
                content = header.read_text()
                if query_pattern.search(content):
                    found = True
                    break
            except Exception:
                continue

        assert found, "Should find at least one Fact containing 'lat'"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
