#!/usr/bin/env python3
"""
Generate pseudo-localized Qt .ts files for UI layout/translation testing.

Usage:
    python3 tools/pseudo_loc.py

Reads translations/qgc.ts and translations/qgc-json.ts, writes
translations/qgc_source_eo.ts and translations/qgc_json_eo.ts
(Esperanto locale, "eo", used as the pseudo-loc slot).

Transformation applied to each translatable string:
  - ASCII letters are replaced with accented look-alikes
  - The result is wrapped in [brackets] and padded with underscores
    to increase length by ~35% (surface layout overflow testing)
  - Qt format markers (%1, %L2, etc.), HTML tags, and XML entities
    are passed through unchanged
"""

import re
import sys
import xml.etree.ElementTree as ET  # for Element/SubElement/indent/tostring construction
from pathlib import Path

import defusedxml.ElementTree as DET  # parse-only hardening for untrusted XML input

# ---------------------------------------------------------------------------
# Character substitution table
# ---------------------------------------------------------------------------
_CHAR_MAP: dict[str, str] = {
    "a": "ȧ", "b": "ƀ", "c": "ċ", "d": "ḋ", "e": "ė", "f": "ƒ",
    "g": "ġ", "h": "ħ", "i": "ı", "j": "ĵ", "k": "ķ", "l": "ĺ",
    "m": "m̃", "n": "ń", "o": "ǒ", "p": "ṗ", "q": "q̈", "r": "ŕ",
    "s": "ŝ", "t": "ṫ", "u": "ũ", "v": "v̇", "w": "ẇ", "x": "ẋ",
    "y": "ẏ", "z": "ż",
    "A": "Ȧ", "B": "Ɓ", "C": "Ċ", "D": "Ḋ", "E": "Ė", "F": "Ƒ",
    "G": "Ġ", "H": "Ħ", "I": "İ", "J": "Ĵ", "K": "Ķ", "L": "Ĺ",
    "M": "M̃", "N": "Ń", "O": "Ǒ", "P": "Ṗ", "Q": "Q̈", "R": "Ŕ",
    "S": "Ŝ", "T": "Ṫ", "U": "Ũ", "V": "V̇", "W": "Ẇ", "X": "Ẋ",
    "Y": "Ẏ", "Z": "Ż",
}

# Pattern that matches tokens to pass through unchanged:
#   %1 %2 %L1  — Qt positional format markers
#   <tag ...>   — HTML/XML tags
#   &name;      — XML/HTML entities
#   &           — Qt accelerator prefix (standalone ampersand)
_PASSTHROUGH = re.compile(
    r"%L?\d+"           # Qt format markers
    r"|<[^>]+>"         # HTML/XML tags
    r"|&\w+;"           # HTML entities (&amp; &lt; etc.)
    r"|&(?=\w)"         # Qt accelerator prefix (&File, &Edit …)
)


def pseudo_loc(text: str) -> str:
    """Return a pseudo-localized version of *text*."""
    if not text:
        return text

    # Tokenise: split into passthrough tokens and ordinary text segments.
    parts: list[str] = []
    cursor = 0
    for m in _PASSTHROUGH.finditer(text):
        if m.start() > cursor:
            parts.append(_substitute(text[cursor : m.start()]))
        parts.append(m.group())   # pass through unchanged
        cursor = m.end()
    if cursor < len(text):
        parts.append(_substitute(text[cursor:]))

    body = "".join(parts)

    # Count alphabetic characters to decide padding length (~35% expansion).
    alpha_count = sum(1 for c in text if c.isalpha())
    pad_len = max(1, round(alpha_count * 0.35))
    padding = "_" * pad_len

    return f"[{body}{padding}]"


def _substitute(segment: str) -> str:
    return "".join(_CHAR_MAP.get(c, c) for c in segment)


# ---------------------------------------------------------------------------
# .ts file processing
# ---------------------------------------------------------------------------

def process_ts(src_path: Path, dst_path: Path, language: str) -> int:
    """Read *src_path*, write pseudo-loc *dst_path*. Returns message count."""
    tree = DET.parse(src_path)
    root = tree.getroot()

    # Set locale on the <TS> element
    root.set("language", language)
    root.set("sourcelanguage", "en")

    count = 0
    for message in root.iter("message"):
        source_el = message.find("source")
        if source_el is None or not source_el.text:
            continue

        trans_el = message.find("translation")
        if trans_el is None:
            trans_el = ET.SubElement(message, "translation")

        # Remove type="unfinished" — our pseudo strings are "finished"
        if "type" in trans_el.attrib:
            del trans_el.attrib["type"]

        trans_el.text = pseudo_loc(source_el.text)
        count += 1

    ET.indent(tree, space="    ")

    # Build output preserving XML declaration
    xml_decl = '<?xml version="1.0" encoding="utf-8"?>\n'
    doctype  = '<!DOCTYPE TS>\n'
    body     = ET.tostring(root, encoding="unicode", xml_declaration=False)
    dst_path.write_text(xml_decl + doctype + body + "\n", encoding="utf-8")
    return count


def main() -> None:
    repo = Path(__file__).parent.parent
    translations = repo / "translations"

    pairs = [
        (translations / "qgc.ts",      translations / "qgc_source_eo.ts"),
        (translations / "qgc-json.ts", translations / "qgc_json_eo.ts"),
    ]

    for src, dst in pairs:
        if not src.exists():
            print(f"WARNING: source not found: {src}", file=sys.stderr)
            continue
        n = process_ts(src, dst, language="eo")
        print(f"Wrote {dst.name}  ({n} messages)")


if __name__ == "__main__":
    main()
