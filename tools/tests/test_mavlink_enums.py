"""Tests for MAVLink enum header generation (no Qt build required)."""

from __future__ import annotations

import textwrap

import pytest
from generators import mavlink_enums

STATE_ENUM = """\
typedef enum TEST_STATE
{
   TEST_STATE_UNKNOWN=-1,
   TEST_STATE_OK=1,
   TEST_STATE_ALIAS=TEST_STATE_OK,
   TEST_STATE_ENUM_END=2,
} TEST_STATE;
"""

FLAGS_ENUM = """\
typedef enum TEST_FLAGS
{
   TEST_FLAG_LOW=0x01,
   TEST_FLAG_HIGH=2147483648,
   TEST_FLAG_HIGH_HEX=0x80000000,
   TEST_FLAG_COMBINED=TEST_FLAG_LOW | TEST_FLAG_HIGH,
   TEST_FLAGS_ENUM_END=2147483649,
} TEST_FLAGS;
"""


@pytest.fixture
def dialect_dir(tmp_path):
    """Include guards and duplicate enums match generated dialect headers."""
    for name, enums in (
        (
            "zeta",
            (
                ("TEST_STATE", STATE_ENUM.replace("TEST_STATE_OK=1", "TEST_STATE_OK=99")),
                ("TEST_FLAGS", FLAGS_ENUM),
            ),
        ),
        ("alpha", (("TEST_STATE", STATE_ENUM),)),
    ):
        dialect = tmp_path / name
        dialect.mkdir()
        definitions = "".join(
            f"#ifndef HAVE_ENUM_{enum_name}\n#define HAVE_ENUM_{enum_name}\n{definition}#endif\n"
            for enum_name, definition in enums
        )
        (dialect / f"{name}.h").write_text(
            f"// ENUM DEFINITIONS\n{definitions}// MESSAGE DEFINITIONS\n"
        )
    return tmp_path


def test_qml_declares_enums_before_registering_them(dialect_dir):
    enums_header, names = mavlink_enums.build_enums_header(mavlink_enums.find_dialects(dialect_dir))
    qml_header = mavlink_enums.build_qml_header(enums_header)

    assert '#include "MAVLinkEnums.h"' in qml_header
    assert "namespace MAVLinkEnums {" in qml_header
    assert "Q_NAMESPACE" in qml_header
    assert "QML_NAMED_ELEMENT(MAVLinkEnums)" in qml_header
    # A `using ::NAME;` alias is invisible to moc: the namespace must hold real declarations.
    assert "using ::" not in qml_header
    assert "HAVE_ENUM_" not in qml_header
    for name, definition in zip(names, (STATE_ENUM, FLAGS_ENUM), strict=True):
        declaration = definition.replace("typedef enum", "enum").replace(f"}} {name};", "};")
        expected = textwrap.indent(declaration + f"Q_ENUM_NS({name})", "    ")
        assert expected in qml_header
        assert qml_header.index("QML_NAMED_ELEMENT(MAVLinkEnums)") < qml_header.index(expected)


def test_first_dialect_wins_in_both_headers(dialect_dir, capsys):
    dialects = mavlink_enums.find_dialects(dialect_dir)
    enums_header, names = mavlink_enums.build_enums_header(dialects)
    qml_header = mavlink_enums.build_qml_header(enums_header)

    assert [name for name, _ in dialects] == ["alpha", "zeta"]
    assert names == ["TEST_STATE", "TEST_FLAGS"]
    assert STATE_ENUM in enums_header
    assert FLAGS_ENUM in enums_header
    assert "#ifndef HAVE_ENUM_TEST_STATE" in enums_header
    assert 'extern "C" {' in enums_header
    for header in (enums_header, qml_header):
        assert header.count("enum TEST_STATE\n") == 1
        assert "TEST_STATE_OK=99" not in header
        assert header.index("enum TEST_STATE\n") < header.index("enum TEST_FLAGS\n")
    assert qml_header.count("Q_ENUM_NS(") == len(names)
    assert "keeping first occurrence" in capsys.readouterr().err


@pytest.mark.parametrize("explicit_qml_paths", [False, True])
def test_main_generates_headers_without_rewriting_unchanged_files(
    dialect_dir, tmp_path, monkeypatch, capsys, explicit_qml_paths
):
    output_dir = tmp_path / "output"
    enums_path = output_dir / "MAVLinkEnums.h"
    qml_path = output_dir / ("explicit.h" if explicit_qml_paths else "MAVLinkEnumsQml.h")
    anchor_path = output_dir / ("explicit.cc" if explicit_qml_paths else "MAVLinkEnumsQml.cc")
    args = ["mavlink_enums.py", str(dialect_dir), str(enums_path)]
    if explicit_qml_paths:
        args.extend((str(qml_path), str(anchor_path)))
    monkeypatch.setattr(mavlink_enums.sys, "argv", args)

    mavlink_enums.main()

    assert "Generated 2 enums from 2 dialects" in capsys.readouterr().out
    assert qml_path.read_text() == mavlink_enums.build_qml_header(enums_path.read_text())
    assert anchor_path.read_text() == mavlink_enums.build_qml_anchor_cc()
    output_paths = (enums_path, qml_path, anchor_path)
    original_mtimes = [path.stat().st_mtime_ns for path in output_paths]

    mavlink_enums.main()

    assert "up to date" in capsys.readouterr().out
    assert [path.stat().st_mtime_ns for path in output_paths] == original_mtimes
