"""Safe XML parsing with defusedxml fallback."""

from pathlib import Path

try:
    from defusedxml.ElementTree import ParseError as XMLParseError
    from defusedxml.ElementTree import parse as _xml_parse_impl

    _USING_DEFUSEDXML = True
except ImportError:
    from xml.etree.ElementTree import ParseError as XMLParseError
    from xml.etree.ElementTree import parse as _xml_parse_impl

    _USING_DEFUSEDXML = False


def xml_parse(path):
    """Parse XML safely, rejecting DTD/ENTITY declarations when defusedxml is unavailable."""
    if _USING_DEFUSEDXML:
        return _xml_parse_impl(path)
    text = Path(path).read_text(encoding="utf-8", errors="replace")
    if "<!DOCTYPE" in text or "<!ENTITY" in text:
        raise XMLParseError("DOCTYPE/ENTITY declarations are not allowed")
    return _xml_parse_impl(path)


__all__ = ["XMLParseError", "xml_parse"]
