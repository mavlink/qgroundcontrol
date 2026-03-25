#!/usr/bin/env python3
"""Tests for tools/pre_commit.py."""

from __future__ import annotations

from unittest.mock import patch

from pre_commit import build_precommit_args, extract_hook_lines, parse_args, summarize_output


def test_summarize_output_counts_states() -> None:
    passed, failed, skipped = summarize_output(
        "hook-a........................Passed\nhook-b........................Failed\nhook-c........................Skipped\n"
    )
    assert (passed, failed, skipped) == (1, 1, 1)


def test_extract_hook_lines_strips_ansi() -> None:
    lines = extract_hook_lines("\x1b[31mhook-a........................Failed\x1b[0m\n")
    assert lines == ["hook-a........................Failed"]


def test_build_precommit_args_all_files() -> None:
    args = parse_args([])
    with patch("pre_commit.git_default_branch_ref", return_value="master"):
        assert build_precommit_args(args)[-1] == "--all-files"


def test_build_precommit_args_changed_mode() -> None:
    args = parse_args(["--changed"])
    with patch("pre_commit.git_default_branch_ref", return_value="master"):
        built = build_precommit_args(args)
    assert built[-4:] == ["--from-ref", "master", "--to-ref", "HEAD"]


def test_build_precommit_args_changed_main_branch() -> None:
    args = parse_args(["--changed"])
    with patch("pre_commit.git_default_branch_ref", return_value="main"):
        built = build_precommit_args(args)
    assert built[-4:] == ["--from-ref", "main", "--to-ref", "HEAD"]


def test_build_precommit_args_changed_no_ref() -> None:
    args = parse_args(["--changed"])
    with patch("pre_commit.git_default_branch_ref", return_value=None):
        built = build_precommit_args(args)
    assert built[-1] == "--all-files"
