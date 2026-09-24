#!/usr/bin/env python3
"""Report the state of QGC's Simplified Chinese Qt translation catalogs."""

from __future__ import annotations

import argparse
import re
import xml.etree.ElementTree as ET
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
CATALOGS = (
    REPO_ROOT / "translations" / "qgc_source_zh_CN.ts",
    REPO_ROOT / "translations" / "qgc_json_zh_CN.ts",
)
PLACEHOLDER = re.compile(r"(?<!%)%(?:L[1-9]\d*|[1-9]\d*|n)")
IGNORED_TYPES = {"obsolete", "vanished"}
# These are identifiers, not natural-language UI copy. Review other unchanged English manually.
IDENTIFIERS = {"MAVLink", "PX4", "ArduPilot", "QGroundControl", "QGC"}


@dataclass
class Counts:
    total: int = 0
    unfinished: int = 0
    empty: int = 0
    unchanged: int = 0
    placeholders: int = 0

    def add(self, other: Counts) -> None:
        self.total += other.total
        self.unfinished += other.unfinished
        self.empty += other.empty
        self.unchanged += other.unchanged
        self.placeholders += other.placeholders


def text_of(element: ET.Element) -> str:
    return "".join(element.itertext()).strip()


def audit_catalog(path: Path) -> tuple[Counts, dict[str, Counts], list[str]]:
    root = ET.parse(path).getroot()
    totals = Counts()
    contexts: dict[str, Counts] = defaultdict(Counts)
    errors: list[str] = []

    for context in root.findall("context"):
        name = context.findtext("name") or "(unnamed)"
        counts = contexts[name]
        for message in context.findall("message"):
            translation = message.find("translation")
            if translation is not None and translation.get("type") in IGNORED_TYPES:
                continue
            counts.total += 1
            source = text_of(message.find("source")) if message.find("source") is not None else ""
            translated = text_of(translation) if translation is not None else ""
            if translation is None or translation.get("type") == "unfinished":
                counts.unfinished += 1
            if not translated:
                counts.empty += 1
            if source == translated and source not in IDENTIFIERS and re.search(r"[A-Za-z]", source):
                counts.unchanged += 1

            source_tokens = Counter(PLACEHOLDER.findall(source))
            forms = translation.findall("numerusform") if translation is not None else []
            variants = [text_of(form) for form in forms] if forms else [translated]
            if any(Counter(PLACEHOLDER.findall(variant)) != source_tokens for variant in variants):
                counts.placeholders += 1
                errors.append(f"{name}: {source!r}: Qt placeholder mismatch")
        totals.add(counts)
    return totals, dict(contexts), errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--strict", action="store_true", help="Fail on unfinished or invalid translations")
    args = parser.parse_args()
    has_failures = False

    for path in CATALOGS:
        totals, contexts, errors = audit_catalog(path)
        print(
            f"{path.name}: total={totals.total} unfinished={totals.unfinished} "
            f"empty={totals.empty} unchanged={totals.unchanged} placeholders={totals.placeholders}"
        )
        for name, counts in sorted(contexts.items(), key=lambda item: (-item[1].unfinished, item[0])):
            if counts.unfinished:
                print(f"  {name}: {counts.unfinished}/{counts.total} unfinished")
        for error in errors:
            print(f"  {error}")
        has_failures |= bool(totals.unfinished or totals.empty or totals.placeholders)
    return int(args.strict and has_failures)


if __name__ == "__main__":
    raise SystemExit(main())
