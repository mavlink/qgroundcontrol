#!/usr/bin/env python3
"""Tests for FactGroup generator."""

import json
import subprocess
import sys
from pathlib import Path

import pytest

TOOLS_DIR = Path(__file__).parent.parent
FIXTURES_DIR = Path(__file__).parent / "fixtures"

# Add tools to path for imports
sys.path.insert(0, str(TOOLS_DIR))

from generators.factgroup.generator import (
    FactSpec,
    MavlinkMessageSpec,
    FactGroupSpec,
    FactGroupGenerator,
    parse_facts_string,
    parse_mavlink_string,
    load_spec_from_file,
    validate_spec,
)


class TestFactSpec:
    """Test FactSpec dataclass."""

    def test_cpp_type_double(self):
        """Double type should map correctly."""
        spec = FactSpec(name="test", value_type="double")
        assert spec.cpp_type == "FactMetaData::valueTypeDouble"

    def test_cpp_type_uint32(self):
        """Uint32 type should map correctly."""
        spec = FactSpec(name="test", value_type="uint32")
        assert spec.cpp_type == "FactMetaData::valueTypeUint32"

    def test_cpp_type_unknown(self):
        """Unknown type should default to double."""
        spec = FactSpec(name="test", value_type="unknown")
        assert spec.cpp_type == "FactMetaData::valueTypeDouble"


class TestMavlinkMessageSpec:
    """Test MavlinkMessageSpec dataclass."""

    def test_handler_name(self):
        """Handler name should be camelCase."""
        spec = MavlinkMessageSpec(message_id="GPS_RAW_INT")
        assert spec.handler_name == "_handleGpsRawInt"

    def test_struct_name(self):
        """Struct name should be lowercase with _t suffix."""
        spec = MavlinkMessageSpec(message_id="GPS_RAW_INT")
        assert spec.struct_name == "mavlink_gps_raw_int_t"

    def test_decode_func(self):
        """Decode function name should follow pattern."""
        spec = MavlinkMessageSpec(message_id="GPS_RAW_INT")
        assert spec.decode_func == "mavlink_msg_gps_raw_int_decode"

    def test_msg_id_constant(self):
        """Message ID constant should be uppercase."""
        spec = MavlinkMessageSpec(message_id="gps_raw_int")
        assert spec.msg_id_constant == "MAVLINK_MSG_ID_GPS_RAW_INT"


class TestFactGroupSpec:
    """Test FactGroupSpec dataclass."""

    def test_class_name(self):
        """Class name should include Vehicle prefix."""
        spec = FactGroupSpec(domain="Wind")
        assert spec.class_name == "VehicleWindFactGroup"

    def test_filenames(self):
        """Filenames should follow pattern."""
        spec = FactGroupSpec(domain="Wind")
        assert spec.header_filename == "VehicleWindFactGroup.h"
        assert spec.source_filename == "VehicleWindFactGroup.cc"
        assert spec.json_filename == "WindFact.json"

    def test_json_resource_path(self):
        """Resource path should be Qt format."""
        spec = FactGroupSpec(domain="Wind")
        assert spec.json_resource_path == ":/json/Vehicle/WindFact.json"

    def test_factgroup_name(self):
        """FactGroup name should have lowercase first letter."""
        spec = FactGroupSpec(domain="Wind")
        assert spec.factgroup_name == "wind"


class TestParsing:
    """Test parsing functions."""

    def test_parse_facts_string_basic(self):
        """Should parse basic fact string."""
        facts = parse_facts_string("speed:double:m/s,altitude:double:m")
        assert len(facts) == 2
        assert facts[0].name == "speed"
        assert facts[0].value_type == "double"
        assert facts[0].units == "m/s"

    def test_parse_facts_string_no_units(self):
        """Should handle missing units."""
        facts = parse_facts_string("count:uint32")
        assert len(facts) == 1
        assert facts[0].units == ""

    def test_parse_mavlink_string(self):
        """Should parse MAVLink message list."""
        messages = parse_mavlink_string("GPS_RAW_INT,HEARTBEAT")
        assert len(messages) == 2
        assert messages[0].message_id == "GPS_RAW_INT"
        assert messages[1].message_id == "HEARTBEAT"

    def test_parse_mavlink_string_lowercase(self):
        """Should uppercase message IDs."""
        messages = parse_mavlink_string("gps_raw_int")
        assert messages[0].message_id == "GPS_RAW_INT"


class TestValidation:
    """Test validation functions."""

    def test_validate_valid_spec(self):
        """Valid spec should pass validation."""
        spec = FactGroupSpec(
            domain="Test",
            facts=[FactSpec(name="value", value_type="double")],
        )
        errors = validate_spec(spec)
        assert len(errors) == 0

    def test_validate_missing_domain(self):
        """Should catch missing domain."""
        spec = FactGroupSpec(
            domain="",
            facts=[FactSpec(name="value", value_type="double")],
        )
        errors = validate_spec(spec)
        assert any("Domain" in e for e in errors)

    def test_validate_no_facts(self):
        """Should catch missing facts."""
        spec = FactGroupSpec(domain="Test", facts=[])
        errors = validate_spec(spec)
        assert any("fact" in e.lower() for e in errors)

    def test_validate_duplicate_names(self):
        """Should catch duplicate fact names."""
        spec = FactGroupSpec(
            domain="Test",
            facts=[
                FactSpec(name="value", value_type="double"),
                FactSpec(name="value", value_type="float"),
            ],
        )
        errors = validate_spec(spec)
        assert any("Duplicate" in e for e in errors)

    def test_validate_invalid_type(self):
        """Should catch invalid types."""
        spec = FactGroupSpec(
            domain="Test",
            facts=[FactSpec(name="value", value_type="badtype")],
        )
        errors = validate_spec(spec)
        assert any("Invalid type" in e for e in errors)


class TestYamlLoading:
    """Test YAML/JSON loading."""

    def test_load_yaml_spec(self):
        """Should load valid YAML spec."""
        spec = load_spec_from_file(FIXTURES_DIR / "test_spec.yaml")
        assert spec.domain == "Test"
        assert len(spec.facts) == 3
        assert len(spec.mavlink_messages) == 2
        assert spec.update_rate_ms == 500

    def test_load_yaml_fact_properties(self):
        """Should parse all fact properties from YAML."""
        spec = load_spec_from_file(FIXTURES_DIR / "test_spec.yaml")
        temp = next(f for f in spec.facts if f.name == "temperature")
        assert temp.units == "degC"
        assert temp.decimal_places == 1
        assert temp.min_value == -40
        assert temp.max_value == 85

    def test_load_invalid_yaml(self):
        """Should validate loaded spec."""
        spec = load_spec_from_file(FIXTURES_DIR / "test_spec_invalid.yaml")
        errors = validate_spec(spec)
        assert len(errors) >= 2  # Duplicate + invalid type


class TestGenerator:
    """Test code generation."""

    def test_generate_header(self, tmp_path):
        """Should generate valid header file."""
        spec = FactGroupSpec(
            domain="Test",
            facts=[FactSpec(name="value", value_type="double", units="m")],
            mavlink_messages=[MavlinkMessageSpec(message_id="TEST")],
        )
        generator = FactGroupGenerator(spec, tmp_path)
        header = generator.generate_header()

        assert "class VehicleTestFactGroup" in header
        assert "Q_PROPERTY(Fact *value" in header
        assert "Fact *value()" in header
        assert "_handleTest" in header

    def test_generate_source(self, tmp_path):
        """Should generate valid source file."""
        spec = FactGroupSpec(
            domain="Test",
            facts=[FactSpec(name="value", value_type="double")],
            mavlink_messages=[MavlinkMessageSpec(message_id="TEST")],
        )
        generator = FactGroupGenerator(spec, tmp_path)
        source = generator.generate_source()

        assert "VehicleTestFactGroup::VehicleTestFactGroup" in source
        assert "_addFact(&_valueFact)" in source
        assert "handleMessage" in source
        assert "MAVLINK_MSG_ID_TEST" in source

    def test_generate_json(self, tmp_path):
        """Should generate valid JSON metadata."""
        spec = FactGroupSpec(
            domain="Test",
            facts=[FactSpec(name="value", value_type="double", units="m", decimal_places=2)],
        )
        generator = FactGroupGenerator(spec, tmp_path)
        json_content = generator.generate_json()

        # Should be valid JSON
        data = json.loads(json_content)
        assert data["version"] == 1
        assert len(data["QGC.MetaData.Facts"]) == 1
        assert data["QGC.MetaData.Facts"][0]["name"] == "value"

    def test_generate_all_creates_files(self, tmp_path):
        """Should create all files when not dry_run."""
        spec = FactGroupSpec(
            domain="Test",
            facts=[FactSpec(name="value", value_type="double")],
        )
        generator = FactGroupGenerator(spec, tmp_path)
        generator.generate_all(dry_run=False)

        assert (tmp_path / "VehicleTestFactGroup.h").exists()
        assert (tmp_path / "VehicleTestFactGroup.cc").exists()
        assert (tmp_path / "TestFact.json").exists()

    def test_dry_run_no_files(self, tmp_path):
        """Dry run should not create files."""
        spec = FactGroupSpec(
            domain="Test",
            facts=[FactSpec(name="value", value_type="double")],
        )
        generator = FactGroupGenerator(spec, tmp_path)
        files = generator.generate_all(dry_run=True)

        assert len(files) == 3
        assert not (tmp_path / "VehicleTestFactGroup.h").exists()


class TestCLI:
    """Test CLI interface."""

    def run_cli(self, *args):
        """Run the CLI and return result."""
        cmd = [sys.executable, "-m", "tools.generators.factgroup.cli"] + list(args)
        return subprocess.run(cmd, capture_output=True, text=True, cwd=TOOLS_DIR.parent)

    def test_cli_help(self):
        """--help should show usage."""
        result = self.run_cli("--help")
        assert result.returncode == 0
        assert "spec" in result.stdout.lower()

    def test_cli_validate_valid(self):
        """--validate should pass for valid spec."""
        result = self.run_cli("--spec", str(FIXTURES_DIR / "test_spec.yaml"), "--validate")
        assert result.returncode == 0
        assert "valid" in result.stdout.lower()

    def test_cli_validate_invalid(self):
        """--validate should fail for invalid spec."""
        result = self.run_cli("--spec", str(FIXTURES_DIR / "test_spec_invalid.yaml"), "--validate")
        assert result.returncode == 1
        assert "error" in result.stderr.lower()

    def test_cli_dry_run(self):
        """--dry-run should show generated content."""
        result = self.run_cli(
            "--spec", str(FIXTURES_DIR / "test_spec.yaml"),
            "--dry-run"
        )
        assert result.returncode == 0
        assert "VehicleTestFactGroup" in result.stdout

    def test_cli_inline_args(self):
        """Should work with inline arguments."""
        result = self.run_cli(
            "--name", "Inline",
            "--facts", "temp:double:degC",
            "--dry-run"
        )
        assert result.returncode == 0
        assert "VehicleInlineFactGroup" in result.stdout


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
