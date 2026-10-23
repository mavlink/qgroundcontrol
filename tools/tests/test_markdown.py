"""Tests for tools/common/markdown.py."""

from __future__ import annotations

import pytest
from common.markdown import (
    code_block,
    code_span,
    details,
    escape_text,
    facts_table,
    md_table,
    more_items,
    summary_section,
)


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


def test_code_span_outgrows_backticks_and_flattens_lines() -> None:
    assert code_span("plain") == "`plain`"
    assert code_span("a `b` c") == "``a `b` c``"
    assert code_span("`edge`") == "`` `edge` ``"
    assert code_span("two\nlines") == "`two lines`"


def test_code_block_fence_is_longer_than_any_backtick_run() -> None:
    assert code_block("x = 1\n") == "```\nx = 1\n```"
    assert code_block("has ``` inside", "text") == "````text\nhas ``` inside\n````"


def test_details_collapses_body() -> None:
    assert details("More", "\nbody\n") == "<details><summary>More</summary>\n\nbody\n\n</details>"


def test_facts_table_skips_empty_values() -> None:
    table = facts_table([("Result", "passed"), ("Skipped", None), ("Empty", ""), ("Zero", 0)])
    assert table == "| Item | Value |\n| --- | --- |\n| Result | passed |\n| Zero | 0 |"
    assert facts_table([("Only", None)]) == ""


def test_more_items() -> None:
    assert more_items(3) == "…and 3 more"


def test_summary_section_layout() -> None:
    section = summary_section(
        "Build",
        [("Result", "passed"), ("Duration", "3m 05s")],
        body="Failures:\n- one",
        collapsed=[("Slowest steps", "| a | b |"), ("Empty", "  ")],
    )
    assert section == (
        "### Build\n\n"
        "| Item | Value |\n| --- | --- |\n| Result | passed |\n| Duration | 3m 05s |\n\n"
        "Failures:\n- one\n\n"
        "<details><summary>Slowest steps</summary>\n\n| a | b |\n\n</details>\n\n"
    )


def test_summary_section_without_facts_or_body_is_just_the_heading() -> None:
    assert summary_section("Tests") == "### Tests\n\n"


def test_escape_text_neutralizes_markup_links_and_mentions() -> None:
    assert escape_text("Test + Coverage linux_gcc_64") == "Test + Coverage linux\\_gcc\\_64"
    assert escape_text("<img src=x> & [x](y) @user\n#1") == (
        "&lt;img src=x&gt; &amp; \\[x\\]\\(y\\) @\u200buser \\#1"
    )


def test_markdown_cells_keep_their_escapes() -> None:
    assert md_table(["H"], [[escape_text("a_b|c")]], markdown=True) == (
        "| H |\n| --- |\n| a\\_b\\\\|c |"
    )
