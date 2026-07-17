#!/usr/bin/env python3
"""Contracts for Markdown table rendering."""

from __future__ import annotations

import pytest
from common.markdown import md_table


def test_table_rendering_handles_rows_alignment_and_newlines() -> None:
    assert md_table(["A", "B"], [["1", "2"], ["3", "4"]]) == (
        "| A | B |\n| --- | --- |\n| 1 | 2 |\n| 3 | 4 |"
    )
    assert md_table(["A", "B"], []) == "| A | B |\n| --- | --- |"
    assert not md_table(["A"], [["1"]]).endswith("\n")
    assert "| Count | 42 |" in md_table(["K", "V"], [["Count", 42]])
    assert md_table(["A", "B", "C"], [], align=["left", "right", "center"]).splitlines()[1] == (
        "| --- | ---: | :---: |"
    )
    rendered = md_table(["H"], [["a\r\nb"]])
    assert "| a<br>b |" in rendered and "\r" not in rendered


def test_table_rejects_shape_and_alignment_errors() -> None:
    cases = [
        ((["A", "B"], [], ["left"]), "align must match headers length"),
        ((["A"], [], ["centre"]), "invalid align value"),
    ]
    for (headers, rows, align), message in cases:
        with pytest.raises(ValueError, match=message):
            md_table(headers, rows, align=align)
    with pytest.raises(ValueError, match="row 1 has 1 cells; expected 2"):
        md_table(["A", "B"], [["1", "2"], ["3"]])
