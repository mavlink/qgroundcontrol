"""GitHub-flavoured Markdown table builder for CI step summaries.

Replaces hand-rolled ``header`` + ``|---|`` separator + row-join blocks in
ccache_helper, size_analysis, collect_build_status, gstreamer_archive,
coverage_comment and test_duration_report.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Iterable, Sequence

__all__ = ["md_table"]

_ALIGN_SEP = {"left": "---", "right": "---:", "center": ":---:"}


def _cell(value: object) -> str:
    # A literal pipe or newline in a cell breaks the row; escape/flatten them.
    text = str(value).replace("\\", "\\\\").replace("|", "\\|")
    return text.replace("\r\n", "<br>").replace("\n", "<br>").replace("\r", "<br>")


def md_table(
    headers: Sequence[str],
    rows: Iterable[Sequence[object]],
    *,
    align: Sequence[str] | None = None,
) -> str:
    """Return a GitHub-flavoured Markdown table (no trailing newline).

    Cells are stringified. *align* is an optional per-column list of
    ``"left"``/``"right"``/``"center"``; defaults to all-left.
    """
    aligns = list(align) if align is not None else ["left"] * len(headers)
    if len(aligns) != len(headers):
        raise ValueError("align must match headers length")
    bad = [a for a in aligns if a not in _ALIGN_SEP]
    if bad:
        raise ValueError(f"invalid align value(s) {bad}; expected one of {sorted(_ALIGN_SEP)}")
    sep = [_ALIGN_SEP[a] for a in aligns]
    out = [
        "| " + " | ".join(_cell(h) for h in headers) + " |",
        "| " + " | ".join(sep) + " |",
    ]
    for i, row in enumerate(rows):
        cells = list(row)
        if len(cells) != len(headers):
            raise ValueError(f"row {i} has {len(cells)} cells; expected {len(headers)}")
        out.append("| " + " | ".join(_cell(cell) for cell in cells) + " |")
    return "\n".join(out)
