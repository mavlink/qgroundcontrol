"""Tests for tools/pre_commit.py."""

from __future__ import annotations

import os
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
    with patch("pre_commit.get_default_branch_ref", return_value="master"):
        assert build_precommit_args(args)[-1] == "--all-files"


def test_build_precommit_args_changed_mode() -> None:
    args = parse_args(["--changed"])
    with patch("pre_commit.get_default_branch_ref", return_value="master"):
        built = build_precommit_args(args)
    assert built[-4:] == ["--from-ref", "master", "--to-ref", "HEAD"]


def test_build_precommit_args_changed_main_branch() -> None:
    args = parse_args(["--changed"])
    with patch("pre_commit.get_default_branch_ref", return_value="main"):
        built = build_precommit_args(args)
    assert built[-4:] == ["--from-ref", "main", "--to-ref", "HEAD"]


def test_build_precommit_args_changed_no_ref() -> None:
    args = parse_args(["--changed"])
    with patch("pre_commit.get_default_branch_ref", return_value=None):
        built = build_precommit_args(args)
    assert built[-1] == "--all-files"


def test_changed_files_use_pr_head_instead_of_merge_checkout() -> None:
    with patch.dict(os.environ, {"PR_BASE_SHA": "base-sha", "PR_HEAD_SHA": "head-sha"}):
        built = build_precommit_args(parse_args(["--changed"]))
    assert built[-4:] == ["--from-ref", "base-sha", "--to-ref", "head-sha"]


def test_summary_counts_colored_hook_results_and_ignores_diagnostics():
    output = (
        "format...........\x1b[41mFailed\x1b[0m\n"
        "python...........\x1b[42mPassed\x1b[0m\n"
        "qml..............(no files)\x1b[46mSkipped\x1b[0m\n"
        "Failed to load a file\n"
    )
    assert summarize_output(output) == (1, 1, 1)
