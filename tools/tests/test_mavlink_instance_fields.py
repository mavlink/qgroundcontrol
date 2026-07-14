"""Contract tests for the MAVLink C++ header generators."""

from __future__ import annotations

import subprocess
import sys
import textwrap
from typing import TYPE_CHECKING

import pytest
from generators.mavlink_instance_fields import (
    extract_instance_fields,
    generate_header,
    resolve_includes,
)

from ._helpers import TOOLS_DIR

if TYPE_CHECKING:
    from pathlib import Path


def _write_xml(path: Path, body: str) -> None:
    path.write_text(f'<?xml version="1.0"?>\n<mavlink>{textwrap.dedent(body)}</mavlink>\n')


def _xml_dialects(tmp_path: Path) -> Path:
    _write_xml(
        tmp_path / "minimal.xml",
        '<messages><message id="0" name="HEARTBEAT"><field type="uint8_t" name="type"/></message></messages>',
    )
    _write_xml(
        tmp_path / "common.xml",
        """
        <include>minimal.xml</include>
        <messages>
          <message id="147" name="BATTERY_STATUS">
            <field type="uint8_t" name="id" instance="true"/>
            <field type="uint8_t" name="battery_function"/>
          </message>
          <message id="132" name="DISTANCE_SENSOR">
            <field type="uint8_t" name="sensor_id" instance="true"/>
          </message>
          <message id="251" name="NAMED_VALUE_FLOAT">
            <field type="char[10]" name="name" instance="true"/>
          </message>
          <message id="100" name="NO_INSTANCE"><field type="uint8_t" name="id"/></message>
        </messages>
        """,
    )
    _write_xml(tmp_path / "all.xml", "<include>common.xml</include>")
    return tmp_path


def test_instance_field_generator_resolves_extracts_and_sorts(tmp_path: Path) -> None:
    dialects = _xml_dialects(tmp_path)
    paths = resolve_includes(dialects, "all")
    assert [path.name for path in paths] == ["minimal.xml", "common.xml", "all.xml"]
    assert resolve_includes(dialects, "missing") == []

    fields = extract_instance_fields(paths)
    assert fields == {
        132: ("DISTANCE_SENSOR", "sensor_id"),
        147: ("BATTERY_STATUS", "id"),
        251: ("NAMED_VALUE_FLOAT", "name"),
    }
    header = generate_header(fields)
    assert "#pragma once" in header
    assert "QMap<quint32, QString>" in header
    assert header.index("{132,") < header.index("{147,") < header.index("{251,")


def test_instance_field_generator_breaks_include_cycles_and_keeps_first_definition(
    tmp_path: Path,
) -> None:
    _write_xml(
        tmp_path / "a.xml",
        '<include>b.xml</include><messages><message id="1" name="A"><field type="uint8_t" name="a" instance="true"/></message></messages>',
    )
    _write_xml(
        tmp_path / "b.xml",
        '<include>a.xml</include><messages><message id="1" name="B"><field type="uint8_t" name="b" instance="true"/></message></messages>',
    )
    paths = resolve_includes(tmp_path, "a")
    assert len(paths) == 2
    assert extract_instance_fields(paths)[1] == ("B", "b")


def test_instance_field_generator_rejects_incomplete_xml(tmp_path: Path) -> None:
    _write_xml(tmp_path / "missing_include.xml", "<include/>")
    with pytest.raises(ValueError, match="must name a dialect"):
        resolve_includes(tmp_path, "missing_include")

    _write_xml(
        tmp_path / "missing_attribute.xml",
        '<messages><message name="BROKEN"><field name="id" instance="true"/></message></messages>',
    )
    with pytest.raises(ValueError, match="required 'id' attribute"):
        extract_instance_fields([tmp_path / "missing_attribute.xml"])


def test_instance_field_entry_point_writes_only_on_change(tmp_path: Path) -> None:
    dialects = _xml_dialects(tmp_path)
    output = tmp_path / "MAVLinkInstanceFields.h"
    command = [
        sys.executable,
        str(TOOLS_DIR / "generators/mavlink_instance_fields.py"),
        str(dialects),
        "common",
        str(output),
    ]

    first = subprocess.run(command, capture_output=True, text=True, check=True)
    second = subprocess.run(command, capture_output=True, text=True, check=True)

    assert "Generated" in first.stdout
    assert "Unchanged" in second.stdout
    assert "BATTERY_STATUS" in output.read_text()


def test_enum_entry_point_writes_only_on_change(tmp_path: Path) -> None:
    dialect = tmp_path / "dialect"
    dialect.mkdir()
    (dialect / "dialect.h").write_text(
        "// ENUM DEFINITIONS\ntypedef enum TEST_ENUM { TEST_ENUM_VALUE = 0, } TEST_ENUM;\n// MESSAGE DEFINITIONS\n"
    )
    output = tmp_path / "MAVLinkEnums.h"
    command = [
        sys.executable,
        str(TOOLS_DIR / "generators/mavlink_enums.py"),
        str(tmp_path),
        str(output),
    ]

    first = subprocess.run(command, capture_output=True, text=True, check=True)
    second = subprocess.run(command, capture_output=True, text=True, check=True)

    assert "Generated 1 enums" in first.stdout
    assert "up to date" in second.stdout
    assert "TEST_ENUM" in output.read_text()
