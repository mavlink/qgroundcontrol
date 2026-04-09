"""Tests for xml_utils.py."""

from __future__ import annotations

import pytest

from xml_utils import XMLParseError, xml_parse


class TestXmlParse:
    """Tests for xml_parse function."""

    def test_parse_valid_xml(self, tmp_path):
        """Parse a simple well-formed XML file."""
        xml_file = tmp_path / "valid.xml"
        xml_file.write_text("<root><child/></root>")
        tree = xml_parse(str(xml_file))
        root = tree.getroot()
        assert root.tag == "root"
        assert root.find("child") is not None

    def test_allows_doctype(self, tmp_path):
        """Allow XML with a bare DOCTYPE declaration (e.g. Cobertura coverage.xml)."""
        xml_file = tmp_path / "doctype.xml"
        xml_file.write_text('<!DOCTYPE foo [<!ELEMENT foo ANY>]><foo/>')
        root = xml_parse(str(xml_file)).getroot()
        assert root.tag == "foo"

    def test_rejects_entity(self, tmp_path):
        """Reject XML with ENTITY declaration."""
        xml_file = tmp_path / "entity.xml"
        xml_file.write_text(
            '<?xml version="1.0"?>\n'
            '<!DOCTYPE foo [<!ENTITY xxe "bad">]>\n'
            "<foo/>"
        )
        with pytest.raises(XMLParseError):
            xml_parse(str(xml_file))

    def test_parse_returns_tree(self, tmp_path):
        """Result has a getroot() method (ElementTree object)."""
        xml_file = tmp_path / "tree.xml"
        xml_file.write_text("<root/>")
        result = xml_parse(str(xml_file))
        assert hasattr(result, "getroot")
        assert result.getroot().tag == "root"
