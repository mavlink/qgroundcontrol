"""Contracts for pseudo-localized Qt translation generation."""

from __future__ import annotations

from typing import TYPE_CHECKING

from pseudo_loc import process_ts, pseudo_loc

if TYPE_CHECKING:
    from pathlib import Path


def test_process_ts_sets_locale_and_finishes_translations(tmp_path: Path) -> None:
    source = tmp_path / "source.ts"
    output = tmp_path / "pseudo.ts"
    source.write_text(
        """<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS language="en">
    <context>
        <message><source>Hello %1</source><translation type="unfinished"/></message>
    </context>
</TS>
""",
        encoding="utf-8",
    )

    assert process_ts(source, output, "eo") == 1
    generated = output.read_text(encoding="utf-8")
    assert '<TS language="eo" sourcelanguage="en">' in generated
    assert 'type="unfinished"' not in generated
    assert pseudo_loc("Hello %1") in generated
