#!/usr/bin/env python3
"""Tests for mavlink_instance_fields generator."""

import textwrap

import pytest

from generators.mavlink_instance_fields import (
    extract_instance_fields,
    generate_header,
    resolve_includes,
)


@pytest.fixture
def xml_dir(tmp_path):
    """Create a minimal MAVLink XML structure for testing."""
    # minimal.xml - no instance fields
    (tmp_path / "minimal.xml").write_text(textwrap.dedent("""\
        <?xml version="1.0"?>
        <mavlink>
          <messages>
            <message id="0" name="HEARTBEAT">
              <field type="uint8_t" name="type">Type</field>
            </message>
          </messages>
        </mavlink>
    """))

    # common.xml - includes minimal, has instance fields
    (tmp_path / "common.xml").write_text(textwrap.dedent("""\
        <?xml version="1.0"?>
        <mavlink>
          <include>minimal.xml</include>
          <messages>
            <message id="147" name="BATTERY_STATUS">
              <field type="uint8_t" name="id" instance="true">Battery ID</field>
              <field type="uint8_t" name="battery_function">Function</field>
            </message>
            <message id="132" name="DISTANCE_SENSOR">
              <field type="uint8_t" name="sensor_id" instance="true">Sensor ID</field>
              <field type="uint8_t" name="type">Type</field>
            </message>
            <message id="251" name="NAMED_VALUE_FLOAT">
              <field type="char[10]" name="name" instance="true">Name</field>
              <field type="float" name="value">Value</field>
            </message>
            <message id="100" name="NO_INSTANCE">
              <field type="uint8_t" name="id">Regular field</field>
            </message>
          </messages>
        </mavlink>
    """))

    # all.xml - includes common
    (tmp_path / "all.xml").write_text(textwrap.dedent("""\
        <?xml version="1.0"?>
        <mavlink>
          <include>common.xml</include>
        </mavlink>
    """))

    return tmp_path


class TestResolveIncludes:
    """Test XML include resolution."""

    def test_single_file(self, xml_dir):
        paths = resolve_includes(xml_dir, "minimal")
        assert len(paths) == 1
        assert paths[0].name == "minimal.xml"

    def test_include_chain(self, xml_dir):
        paths = resolve_includes(xml_dir, "common")
        assert len(paths) == 2
        assert paths[0].name == "minimal.xml"
        assert paths[1].name == "common.xml"

    def test_full_chain(self, xml_dir):
        paths = resolve_includes(xml_dir, "all")
        assert len(paths) == 3
        assert paths[0].name == "minimal.xml"
        assert paths[1].name == "common.xml"
        assert paths[2].name == "all.xml"

    def test_missing_dialect(self, xml_dir):
        paths = resolve_includes(xml_dir, "nonexistent")
        assert paths == []

    def test_no_circular(self, xml_dir):
        """Circular includes don't infinite loop."""
        (xml_dir / "a.xml").write_text(textwrap.dedent("""\
            <?xml version="1.0"?>
            <mavlink><include>b.xml</include></mavlink>
        """))
        (xml_dir / "b.xml").write_text(textwrap.dedent("""\
            <?xml version="1.0"?>
            <mavlink><include>a.xml</include></mavlink>
        """))
        paths = resolve_includes(xml_dir, "a")
        assert len(paths) == 2


class TestExtractInstanceFields:
    """Test instance field extraction."""

    def test_finds_instance_fields(self, xml_dir):
        paths = resolve_includes(xml_dir, "common")
        fields = extract_instance_fields(paths)
        assert 147 in fields
        assert fields[147] == ("BATTERY_STATUS", "id")
        assert 132 in fields
        assert fields[132] == ("DISTANCE_SENSOR", "sensor_id")

    def test_string_instance_field(self, xml_dir):
        paths = resolve_includes(xml_dir, "common")
        fields = extract_instance_fields(paths)
        assert 251 in fields
        assert fields[251] == ("NAMED_VALUE_FLOAT", "name")

    def test_skips_non_instance(self, xml_dir):
        paths = resolve_includes(xml_dir, "common")
        fields = extract_instance_fields(paths)
        assert 100 not in fields
        assert 0 not in fields

    def test_deduplicates_by_msg_id(self, xml_dir):
        """If same message appears in multiple XMLs, keep first occurrence."""
        (xml_dir / "extra.xml").write_text(textwrap.dedent("""\
            <?xml version="1.0"?>
            <mavlink>
              <include>common.xml</include>
              <messages>
                <message id="147" name="BATTERY_STATUS">
                  <field type="uint8_t" name="other_id" instance="true">Other</field>
                </message>
              </messages>
            </mavlink>
        """))
        paths = resolve_includes(xml_dir, "extra")
        fields = extract_instance_fields(paths)
        # First occurrence wins (from common.xml)
        assert fields[147] == ("BATTERY_STATUS", "id")


class TestGenerateHeader:
    """Test header generation."""

    def test_generates_valid_header(self, xml_dir):
        paths = resolve_includes(xml_dir, "common")
        fields = extract_instance_fields(paths)
        header = generate_header(fields)
        assert "#pragma once" in header
        assert "QMap<quint32, QString>" in header
        assert '{147, QStringLiteral("id")}' in header
        assert '{132, QStringLiteral("sensor_id")}' in header
        assert "BATTERY_STATUS" in header

    def test_sorted_by_msg_id(self, xml_dir):
        paths = resolve_includes(xml_dir, "common")
        fields = extract_instance_fields(paths)
        header = generate_header(fields)
        # 132 should appear before 147
        pos_132 = header.find("132")
        pos_147 = header.find("147")
        assert pos_132 < pos_147

    def test_empty_fields(self):
        header = generate_header({})
        assert "#pragma once" in header
        assert "QMap<quint32, QString>" in header
