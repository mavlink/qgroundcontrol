"""Safe XML parsing helpers."""

from __future__ import annotations

from pathlib import Path
from typing import TYPE_CHECKING
from xml.etree.ElementTree import ParseError as XMLParseError

from defusedxml.ElementTree import parse as _xml_parse_impl

if TYPE_CHECKING:
    from os import PathLike
    from xml.etree.ElementTree import ElementTree

__all__ = ["XMLParseError", "xml_parse"]


def xml_parse(path: str | PathLike[str]) -> ElementTree:
    """Parse XML safely, rejecting entity declarations."""
    xml_path = Path(path)
    text = xml_path.read_text(encoding="utf-8", errors="replace")
    if "<!ENTITY" in text:
        raise XMLParseError("ENTITY declarations are not allowed")
    return _xml_parse_impl(xml_path)
