"""Tests for safe XML parsing."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from common.xml import XMLParseError, xml_parse

if TYPE_CHECKING:
    from pathlib import Path


def test_parser_returns_trees_for_plain_and_doctype_xml(tmp_path: Path) -> None:
    for name, content, tag in (
        ("plain.xml", "<root><child/></root>", "root"),
        ("doctype.xml", "<!DOCTYPE foo [<!ELEMENT foo ANY>]><foo/>", "foo"),
        ("comment.xml", "<root><!-- harmless <!ENTITY text> --></root>", "root"),
        ("cdata.xml", "<root><![CDATA[harmless <!ENTITY text>]]></root>", "root"),
    ):
        path = tmp_path / name
        path.write_text(content, encoding="utf-8")
        tree = xml_parse(path)
        root = tree.getroot()
        assert root is not None
        assert root.tag == tag


def test_parser_rejects_entity_declarations(tmp_path: Path) -> None:
    path = tmp_path / "entity.xml"
    path.write_text('<!DOCTYPE foo [<!ENTITY xxe "bad">]><foo/>', encoding="utf-8")
    with pytest.raises(XMLParseError):
        xml_parse(path)
