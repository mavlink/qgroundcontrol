"""GitHub-flavoured Markdown builders for CI job summaries and PR comments.

Job summaries share one layout: an H3 heading per section, a two-column facts table, an optional
body, and collapsed ``<details>`` blocks for long listings (``summary_section``).
"""

from __future__ import annotations

import html
import re
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Iterable, Sequence

__all__ = [
    "code_block",
    "code_span",
    "details",
    "escape_text",
    "facts_table",
    "md_table",
    "more_items",
    "summary_section",
]

_ALIGN_SEP = {"left": "---", "right": "---:", "center": ":---:"}
_BACKTICK_RUN = re.compile(r"`+")
_MARKDOWN_PUNCTUATION = re.compile(r"([\\`*_\[\]()#!|~])")

Fact = tuple[str, object]


def _cell(value: object, *, markdown: bool = False) -> str:
    # A literal pipe or newline in a cell breaks the row; escape/flatten them. Literal text also
    # keeps its backslashes; Markdown cells keep their own escapes.
    text = str(value) if markdown else str(value).replace("\\", "\\\\")
    text = text.replace("|", "\\|")
    return text.replace("\r\n", "<br>").replace("\n", "<br>").replace("\r", "<br>")


def md_table(
    headers: Sequence[str],
    rows: Iterable[Sequence[object]],
    *,
    align: Sequence[str] | None = None,
    markdown: bool = False,
) -> str:
    """Return a GitHub-flavoured Markdown table (no trailing newline).

    Cells are stringified literal text, or Markdown with *markdown* (links, code spans, and
    ``escape_text`` output). *align* is an optional per-column list of
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
        out.append("| " + " | ".join(_cell(cell, markdown=markdown) for cell in cells) + " |")
    return "\n".join(out)


def _fence_for(text: str, minimum: int) -> str:
    longest = max((len(run) for run in _BACKTICK_RUN.findall(text)), default=0)
    return "`" * max(minimum, longest + 1)


def code_span(text: object) -> str:
    """Return *text* as inline code that no backtick, newline, or pipe inside it can break."""
    value = " ".join(str(text).split())
    fence = _fence_for(value, 1)
    pad = " " if value.startswith("`") or value.endswith("`") else ""
    return f"{fence}{pad}{value}{pad}{fence}"


def escape_text(text: object) -> str:
    """Return untrusted *text* as one line of literal Markdown/HTML text.

    Escapes HTML and Markdown punctuation and breaks ``@`` so names from pull requests cannot
    inject links or markup or mention users.
    """
    value = html.escape(" ".join(str(text).split()), quote=False)
    return _MARKDOWN_PUNCTUATION.sub(r"\\\1", value).replace("@", "@\u200b")


def code_block(text: str, language: str = "") -> str:
    """Return *text* as a fenced code block whose fence is longer than any backtick run in it."""
    fence = _fence_for(text, 3)
    return f"{fence}{language}\n{text.rstrip()}\n{fence}"


def details(summary: str, body: str) -> str:
    """Return a collapsed ``<details>`` block; *summary* is plain text."""
    return f"<details><summary>{summary}</summary>\n\n{body.strip()}\n\n</details>"


def facts_table(facts: Iterable[Fact]) -> str:
    """Return a two-column table of the facts whose value is not None or empty, or ``""``.

    Values are Markdown, so they may hold links and code spans; pass untrusted text through
    ``escape_text`` or ``code_span``.
    """
    rows = [(key, value) for key, value in facts if value is not None and str(value) != ""]
    return md_table(["Item", "Value"], rows, markdown=True) if rows else ""


def more_items(count: int) -> str:
    """Return the trailer for a list truncated by *count* entries."""
    return f"…and {count} more"


def summary_section(
    title: str,
    facts: Iterable[Fact] = (),
    *,
    body: str = "",
    collapsed: Iterable[tuple[str, str]] = (),
) -> str:
    """Return one job-summary section: H3 *title*, facts table, *body*, collapsed blocks.

    *collapsed* holds ``(summary, body)`` pairs; empty bodies are skipped. The section ends with a
    blank line so sections that consecutive steps append stay separate.
    """
    parts = [f"### {title}", facts_table(facts), body.strip()]
    parts += [details(label, text) for label, text in collapsed if text.strip()]
    return "\n\n".join(part for part in parts if part) + "\n\n"
