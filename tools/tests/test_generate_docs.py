#!/usr/bin/env python3
"""Tests for tools/generate_docs.py."""

from __future__ import annotations

from generate_docs import build_doxyfile_text


def test_build_doxyfile_text_overrides_output_and_pdf(tmp_path) -> None:
    base = "OUTPUT_DIRECTORY = docs/api\nGENERATE_LATEX = NO\n"
    rendered = build_doxyfile_text(base, tmp_path / "api", generate_pdf=True)

    assert 'OUTPUT_DIRECTORY       = "' in rendered
    assert (tmp_path / "api").as_posix() in rendered
    assert "GENERATE_LATEX         = YES" in rendered

