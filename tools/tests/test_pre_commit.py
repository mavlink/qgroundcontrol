"""Contracts for the pre-commit wrapper's parsing and selection."""

from __future__ import annotations

from unittest.mock import patch

from pre_commit import build_precommit_args, extract_hook_lines, parse_args, summarize_output


def test_result_parsing_counts_states_and_removes_ansi() -> None:
    output = (
        "hook-a........................Passed\n"
        "\x1b[31mhook-b........................Failed\x1b[0m\n"
        "hook-c........................Skipped\n"
        "diagnostic: operation Failed after Passed preconditions\n"
        "source..........text mentions Skipped but is not a hook result\n"
    )
    assert summarize_output(output) == (1, 1, 1)
    assert extract_hook_lines(output) == [
        "hook-a........................Passed",
        "hook-b........................Failed",
        "hook-c........................Skipped",
    ]


def test_argument_selection_uses_changed_range_or_all_files() -> None:
    for argv, default_ref, expected in (
        ([], "master", ["--all-files"]),
        (["--changed"], "master", ["--from-ref", "master", "--to-ref", "HEAD"]),
        (["--changed"], "main", ["--from-ref", "main", "--to-ref", "HEAD"]),
        (["--changed"], None, ["--all-files"]),
    ):
        with (
            patch.dict("os.environ", {"GITHUB_BASE_REF": ""}),
            patch("pre_commit.get_default_branch_ref", return_value=default_ref),
        ):
            built = build_precommit_args(parse_args(argv))
        assert built[-len(expected) :] == expected


def test_changed_selection_prefers_github_pull_request_base() -> None:
    with (
        patch.dict("os.environ", {"GITHUB_BASE_REF": "Stable_V5.0"}),
        patch("pre_commit.get_default_branch_ref") as get_default_branch_ref,
    ):
        built = build_precommit_args(parse_args(["--changed"]))

    assert built[-4:] == ["--from-ref", "origin/Stable_V5.0", "--to-ref", "HEAD"]
    get_default_branch_ref.assert_not_called()
