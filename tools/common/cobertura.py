"""Typed parsing for Cobertura coverage reports."""

from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING

from .xml import XMLParseError, xml_parse

if TYPE_CHECKING:
    from os import PathLike
    from xml.etree.ElementTree import Element

__all__ = ["CoberturaError", "CoberturaMetrics", "read_cobertura"]


class CoberturaError(ValueError):
    """Raised when a Cobertura report cannot provide valid metrics."""


@dataclass(frozen=True)
class CoberturaMetrics:
    """Normalized coverage percentages and source-line counts."""

    line_percent: float
    branch_percent: float | None
    lines_valid: int
    lines_covered: int


def _optional_percent(element: Element, attribute: str) -> float | None:
    value = element.get(attribute)
    if value is None or value == "":
        return None
    return float(value) * 100.0


def _integer(element: Element, attribute: str) -> int:
    value = element.get(attribute)
    if value is None or value == "":
        return 0
    return int(value)


def read_cobertura(path: str | PathLike[str]) -> CoberturaMetrics:
    """Read root metrics, falling back to the first package with a line rate."""
    try:
        root = xml_parse(path).getroot()
        if root is None:
            raise CoberturaError("coverage report has no root element")

        metrics_element = root
        if root.get("line-rate") in {None, ""}:
            metrics_element = next(
                (
                    package
                    for package in root.findall(".//package")
                    if package.get("line-rate") not in {None, ""}
                ),
                None,
            )
            if metrics_element is None:
                raise CoberturaError("coverage report has no line-rate metric")

        line_percent = _optional_percent(metrics_element, "line-rate")
        if line_percent is None:
            raise CoberturaError("coverage report has no line-rate metric")
        return CoberturaMetrics(
            line_percent=line_percent,
            branch_percent=_optional_percent(metrics_element, "branch-rate"),
            lines_valid=_integer(metrics_element, "lines-valid"),
            lines_covered=_integer(metrics_element, "lines-covered"),
        )
    except CoberturaError:
        raise
    except (XMLParseError, OSError, TypeError, ValueError) as exc:
        raise CoberturaError(f"failed to parse {path}: {exc}") from exc
