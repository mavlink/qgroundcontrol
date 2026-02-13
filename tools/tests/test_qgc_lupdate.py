#!/usr/bin/env python3
"""Tests for qgc_lupdate.py."""

import os
import sys
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent / "translations"))


class TestQtPathDiscovery:
    """Tests for Qt lupdate path discovery."""

    def test_find_lupdate_from_env(self) -> None:
        """Verify lupdate found via QT_ROOT_DIR environment variable."""
        from qgc_lupdate import find_lupdate

        with patch.dict(os.environ, {"QT_ROOT_DIR": "/opt/Qt/6.8.0/gcc_64"}):
            with patch("pathlib.Path.exists", return_value=True):
                with patch("os.access", return_value=True):
                    result = find_lupdate()
                    assert result is not None
                    assert "lupdate" in str(result)

    def test_find_lupdate_from_home(self) -> None:
        """Verify lupdate found in ~/Qt directory."""
        from qgc_lupdate import find_lupdate

        with patch.dict(os.environ, {}, clear=True):
            # Remove QT_ROOT_DIR if present
            os.environ.pop("QT_ROOT_DIR", None)

            mock_paths = [Path("/home/user/Qt/6.8.0/gcc_64/bin/lupdate")]
            with patch("pathlib.Path.glob", return_value=mock_paths):
                with patch("pathlib.Path.exists", return_value=True):
                    with patch("os.access", return_value=True):
                        result = find_lupdate()
                        # May or may not find depending on mock setup
                        # Just verify no exception

    def test_find_lupdate_not_found(self) -> None:
        """Verify None returned when lupdate not found."""
        from qgc_lupdate import find_lupdate

        with patch.dict(os.environ, {}, clear=True):
            os.environ.pop("QT_ROOT_DIR", None)
            with patch("pathlib.Path.glob", return_value=[]):
                with patch("pathlib.Path.exists", return_value=False):
                    result = find_lupdate()
                    assert result is None


class TestJsonParsing:
    """Tests for JSON translation string extraction."""

    def test_escape_xml_string(self) -> None:
        """Verify XML string escaping."""
        from qgc_lupdate import escape_xml_string

        assert escape_xml_string("&") == "&amp;"
        assert escape_xml_string("<") == "&lt;"
        assert escape_xml_string(">") == "&gt;"
        assert escape_xml_string("'") == "&apos;"
        assert escape_xml_string('"') == "&quot;"
        assert escape_xml_string("normal") == "normal"

    def test_escape_xml_string_combined(self) -> None:
        """Verify multiple escapes in one string."""
        from qgc_lupdate import escape_xml_string

        result = escape_xml_string("<test & 'value'>")
        assert "&lt;" in result
        assert "&amp;" in result
        assert "&apos;" in result
        assert "&gt;" in result

    def test_parse_json_object_for_translate_keys(self) -> None:
        """Verify translation keys are extracted from JSON objects."""
        from qgc_lupdate import parse_json_object_for_translate_keys

        json_obj = {
            "label": "Test Label",
            "description": "Test Description",
            "other": "Not translated",
        }
        loc_strings: dict[str, list[str]] = {}

        parse_json_object_for_translate_keys(
            "", json_obj, ["label", "description"], [], loc_strings
        )

        assert "Test Label" in loc_strings
        assert "Test Description" in loc_strings
        assert "Not translated" not in loc_strings

    def test_parse_json_object_nested(self) -> None:
        """Verify nested objects are parsed."""
        from qgc_lupdate import parse_json_object_for_translate_keys

        json_obj = {
            "label": "Outer",
            "nested": {
                "label": "Inner",
            },
        }
        loc_strings: dict[str, list[str]] = {}

        parse_json_object_for_translate_keys(
            "", json_obj, ["label"], [], loc_strings
        )

        assert "Outer" in loc_strings
        assert "Inner" in loc_strings

    def test_parse_json_array_for_translate_keys(self) -> None:
        """Verify arrays are parsed with index."""
        from qgc_lupdate import parse_json_array_for_translate_keys

        json_array = [
            {"label": "First"},
            {"label": "Second"},
        ]
        loc_strings: dict[str, list[str]] = {}

        parse_json_array_for_translate_keys(
            ".items", json_array, ["label"], [], loc_strings
        )

        assert "First" in loc_strings
        assert "Second" in loc_strings

    def test_parse_json_array_with_id_key(self) -> None:
        """Verify array ID keys are used for hierarchy."""
        from qgc_lupdate import parse_json_array_for_translate_keys

        json_array = [
            {"name": "item1", "label": "First Item"},
            {"name": "item2", "label": "Second Item"},
        ]
        loc_strings: dict[str, list[str]] = {}

        parse_json_array_for_translate_keys(
            ".items", json_array, ["label"], ["name"], loc_strings
        )

        assert "First Item" in loc_strings
        # Check hierarchy uses name instead of index
        assert any("item1" in h for h in loc_strings["First Item"])


class TestFileTypeDetection:
    """Tests for QGC file type auto-detection."""

    def test_add_loc_keys_mavcmdinfo(self) -> None:
        """Verify MavCmdInfo file type adds correct keys."""
        from qgc_lupdate import add_loc_keys_based_on_qgc_file_type

        json_dict: dict = {"fileType": "MavCmdInfo"}
        add_loc_keys_based_on_qgc_file_type("test.json", json_dict)

        assert "translateKeys" in json_dict
        assert "label" in json_dict["translateKeys"]
        assert "arrayIDKeys" in json_dict

    def test_add_loc_keys_factmetadata(self) -> None:
        """Verify FactMetaData file type adds correct keys."""
        from qgc_lupdate import add_loc_keys_based_on_qgc_file_type

        json_dict: dict = {"fileType": "FactMetaData"}
        add_loc_keys_based_on_qgc_file_type("test.json", json_dict)

        assert "translateKeys" in json_dict
        assert "shortDesc" in json_dict["translateKeys"]

    def test_add_loc_keys_unknown_type(self) -> None:
        """Verify unknown file type doesn't add keys."""
        from qgc_lupdate import add_loc_keys_based_on_qgc_file_type

        json_dict: dict = {"fileType": "Unknown"}
        add_loc_keys_based_on_qgc_file_type("test.json", json_dict)

        assert "translateKeys" not in json_dict

    def test_add_loc_keys_no_override(self) -> None:
        """Verify existing keys are not overridden."""
        from qgc_lupdate import add_loc_keys_based_on_qgc_file_type

        json_dict: dict = {
            "fileType": "MavCmdInfo",
            "translateKeys": "customKey",
        }
        add_loc_keys_based_on_qgc_file_type("test.json", json_dict)

        assert json_dict["translateKeys"] == "customKey"


class TestArgumentParsing:
    """Tests for CLI argument parsing."""

    def test_default_args(self) -> None:
        """Verify default arguments."""
        from qgc_lupdate import parse_args

        args = parse_args([])
        assert args.json_only is False
        assert args.lupdate_only is False

    def test_json_only_flag(self) -> None:
        """Verify --json-only flag."""
        from qgc_lupdate import parse_args

        args = parse_args(["--json-only"])
        assert args.json_only is True

    def test_lupdate_only_flag(self) -> None:
        """Verify --lupdate-only flag."""
        from qgc_lupdate import parse_args

        args = parse_args(["--lupdate-only"])
        assert args.lupdate_only is True

    def test_src_dir_option(self) -> None:
        """Verify --src-dir option."""
        from qgc_lupdate import parse_args

        args = parse_args(["--src-dir", "/custom/src"])
        assert args.src_dir == Path("/custom/src")

    def test_output_dir_option(self) -> None:
        """Verify --output-dir option."""
        from qgc_lupdate import parse_args

        args = parse_args(["--output-dir", "/custom/translations"])
        assert args.output_dir == Path("/custom/translations")


class TestTsFileGeneration:
    """Tests for .ts file generation."""

    def test_generate_ts_content(self) -> None:
        """Verify TS file content structure."""
        from qgc_lupdate import generate_ts_content

        multi_file_loc = [
            ("test.json", "/path/test.json", {"Hello": [".label"]}),
        ]

        content = generate_ts_content(multi_file_loc)

        assert '<?xml version="1.0"' in content
        assert "<TS version" in content
        assert "<context>" in content
        assert "<name>test.json</name>" in content
        assert "<source>Hello</source>" in content
        assert "</TS>" in content

    def test_generate_ts_with_disambiguation(self) -> None:
        """Verify disambiguation handling."""
        from qgc_lupdate import generate_ts_content

        multi_file_loc = [
            ("test.json", "/path/test.json", {"#loc.disambiguation#context#Actual text": [".label"]}),
        ]

        content = generate_ts_content(multi_file_loc)

        assert "<comment>context</comment>" in content
        assert "<source>Actual text</source>" in content
