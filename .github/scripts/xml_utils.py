"""Safe XML parsing with defusedxml fallback."""

from pathlib import Path

try:
    from defusedxml.ElementTree import parse as _xml_parse_impl
except ImportError:
    from xml.etree.ElementTree import parse as _xml_parse_impl

from xml.etree.ElementTree import ParseError as XMLParseError


def xml_parse(path):
    """Parse XML safely, rejecting ENTITY declarations."""
    text = Path(path).read_text(encoding="utf-8", errors="replace")
    if "<!ENTITY" in text:
        raise XMLParseError("ENTITY declarations are not allowed")
    return _xml_parse_impl(path)


__all__ = ["XMLParseError", "xml_parse"]
