#!/usr/bin/env python3
"""Tests for tools/common/markdown.py."""

from __future__ import annotations

import pytest
from common.markdown import md_table


def test_basic_table() -> None:
    out = md_table(["A", "B"], [["1", "2"], ["3", "4"]])
    assert out == "| A | B |\n| --- | --- |\n| 1 | 2 |\n| 3 | 4 |"


def test_no_trailing_newline() -> None:
    assert not md_table(["A"], [["1"]]).endswith("\n")


def test_empty_rows_keeps_header_and_separator() -> None:
    assert md_table(["A", "B"], []) == "| A | B |\n| --- | --- |"


def test_cells_are_stringified() -> None:
    assert "| Count | 42 |" in md_table(["K", "V"], [["Count", 42]])


def test_alignment_separators() -> None:
    out = md_table(["A", "B", "C"], [], align=["left", "right", "center"])
    assert out.splitlines()[1] == "| --- | ---: | :---: |"


def test_align_length_mismatch_raises() -> None:
    with pytest.raises(ValueError, match="align must match headers length"):
        md_table(["A", "B"], [], align=["left"])


def test_invalid_align_value_raises() -> None:
    with pytest.raises(ValueError, match="invalid align value"):
        md_table(["A"], [], align=["centre"])


def test_row_length_mismatch_raises() -> None:
    with pytest.raises(ValueError, match="row 1 has 1 cells; expected 2"):
        md_table(["A", "B"], [["1", "2"], ["3"]])


def test_crlf_flattened_to_br() -> None:
    assert "| a<br>b |" in md_table(["H"], [["a\r\nb"]])
    assert "\r" not in md_table(["H"], [["a\rb"]])
