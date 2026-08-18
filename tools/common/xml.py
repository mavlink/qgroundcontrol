"""Safe XML parsing helpers."""

from __future__ import annotations

from typing import TYPE_CHECKING
from xml.etree.ElementTree import ParseError as XMLParseError

from defusedxml.common import DefusedXmlException
from defusedxml.ElementTree import parse as _xml_parse_impl

if TYPE_CHECKING:
    from os import PathLike
    from xml.etree.ElementTree import ElementTree

__all__ = ["XMLParseError", "xml_parse"]


def xml_parse(path: str | PathLike[str]) -> ElementTree:
    """Parse XML safely, rejecting entity declarations."""
    try:
        return _xml_parse_impl(path)
    except DefusedXmlException as exc:
        raise XMLParseError(str(exc)) from exc
