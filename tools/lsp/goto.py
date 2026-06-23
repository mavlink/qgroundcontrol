"""Go-to-definition support for Fact references."""

from __future__ import annotations

import json
import logging
import re
from pathlib import Path  # noqa: TC003

from lsprotocol import types

logger = logging.getLogger(__name__)


def extract_fact_name(word: str, line: str, word_start: int, word_end: int) -> str | None:
    """Recognize the Fact-naming idioms used in QGC C++ and pull the bare name out."""
    if word.startswith("_") and word.endswith("Fact"):
        return word[1:-4]

    if word.endswith("Fact"):
        rest = line[word_end:].lstrip()
        if rest.startswith("("):
            return word[:-4]

    string_match = re.search(r'QStringLiteral\s*\(\s*"(\w+)"\s*\)', line)
    if string_match and word_start <= string_match.end() and word_end >= string_match.start():
        return string_match.group(1)

    if re.search(r'":/json/([^"]+)Fact\.json"', line):
        return None
    return None


def find_fact_definition(project_root: Path | None, fact_name: str) -> types.Location | None:
    """Search project Fact JSONs for `fact_name`; return its Location or None."""
    if not project_root:
        return None

    src_root = project_root / "src"
    for pattern in ("*Fact.json", "*Facts.json", "FactMetaData/*.json"):
        for json_path in src_root.rglob(pattern):
            location = _search_fact_in_json(str(json_path), fact_name)
            if location:
                return location
    return None


def _search_fact_in_json(json_path: str, fact_name: str) -> types.Location | None:
    try:
        with open(json_path, encoding="utf-8") as f:
            content = f.read()
        data = json.loads(content)
        for fact in data.get("QGC.MetaData.Facts", []):
            if fact.get("name") == fact_name:
                line_num = _find_fact_line(content, fact_name)
                if line_num >= 0:
                    return types.Location(
                        uri=f"file://{json_path}",
                        range=types.Range(
                            start=types.Position(line=line_num, character=0),
                            end=types.Position(line=line_num, character=0),
                        ),
                    )
    except (json.JSONDecodeError, OSError) as e:
        logger.debug(f"Failed to parse {json_path}: {e}")
    return None


def _find_fact_line(content: str, fact_name: str) -> int:
    for i, line in enumerate(content.split("\n")):
        if re.search(rf'"name"\s*:\s*"{re.escape(fact_name)}"', line):
            return i
    return -1
