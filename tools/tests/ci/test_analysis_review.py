"""Inline Code Analysis reviews post only escaped, unposted bug-class findings on lines the PR
adds, up to a cap per pull request."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

import pytest
from ci.reporting import analysis_review as review
from common.analysis_diagnostics import AnalysisWarning

from .._helpers import completed

HEAD = "a" * 40
MERGE = "b" * 40
PATCH = "@@ -1,3 +1,4 @@\n ctx\n-old\n+new2\n+new3\n ctx\n\\ No newline at end of file\n@@ -10 +11,2 @@\n+n11\n x\n"


def test_added_lines_follow_each_hunk_s_right_side():
    assert review.added_lines(PATCH) == {2, 3, 11}
    assert review.added_lines("") == set()


def test_added_lines_split_only_on_newlines():
    patch = "@@ -1 +1,3 @@\n+page\x0cbreak\n+line\u2028separator\n+next\n"
    assert review.added_lines(patch) == {1, 2, 3}


def _write_reports(
    directory: Path,
    changed: dict[str, list[list[Any]]],
    commit: str = MERGE,
    tool: str = "clang-tidy",
):
    report = directory / f"clang-tidy-clazy-output-1/{tool}-diagnostics.json"
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(
        json.dumps({"tool": tool, "commit": commit, "diagnostics": {}, "changed": changed})
    )


def _bot_comment(path: str, line: int | None, body: str) -> dict[str, Any]:
    """A review comment as GitHub lists it; an outdated one has no line."""
    return {"user": {"type": "Bot"}, "path": path, "line": line, "body": body}


def _posted_before(count: int) -> list[dict[str, Any]]:
    """Finding comments of earlier reviews, every other one outdated."""
    return [
        _bot_comment(
            "src/old.cc",
            None if index % 2 else index + 1,
            review.comment_body(
                AnalysisWarning(
                    "clang-tidy", "src/old.cc", index + 1, 1, "bugprone-old", f"old{index}"
                )
            ),
        )
        for index in range(count)
    ]


def _add_lines(github: dict[str, Any], count: int) -> None:
    github["files"][0]["patch"] = f"@@ -0,0 +1,{count} @@\n" + "+x\n" * count


@pytest.fixture
def github(monkeypatch: pytest.MonkeyPatch) -> dict[str, Any]:
    """Fake the PR's files and comments, the merge's changed files, blobs, and the review POST."""
    state: dict[str, Any] = {
        "files": [{"filename": "src/a.cc", "status": "modified", "patch": PATCH}],
        "comments": [],
        "merge_changed": [],
        "blobs": {},
        "reads": [],
        "posted": [],
        "current": {"current": "true", "pr": "7"},
    }

    def gh(*args: str, **kwargs: Any):
        path = next(arg for arg in args if arg.startswith("repos/"))
        if "--paginate" in args:
            key = "files" if "/files" in path else "comments"
            return completed("\n".join(json.dumps(item) for item in state[key]))
        assert args[:3] == ("api", "--method", "POST")
        state["posted"].append(json.loads(Path(args[-1]).read_text(encoding="utf-8")))
        return completed("{}")

    def api(path: str) -> dict[str, Any]:
        state["reads"].append(path)
        if path == f"repos/o/r/compare/{HEAD}...{MERGE}?per_page=1":
            return {"files": state["merge_changed"]}
        file, ref = path.removeprefix("repos/o/r/contents/").split("?ref=")
        return {"sha": state["blobs"].get((file, ref), f"blob-{file}")}

    monkeypatch.setattr(review, "gh", gh)
    monkeypatch.setattr(review, "api", api)
    monkeypatch.setattr(review, "resolve_context", lambda repo, run: state["current"])
    return state


RUN = {
    "conclusion": "success",
    "head_sha": HEAD,
    "html_url": "https://github.com/o/r/actions/runs/5",
    "path": review.ANALYSIS_WORKFLOW,
}


def test_posts_new_warnings_on_added_lines_of_files_the_merge_left_unchanged(
    tmp_path: Path, github: dict[str, Any]
):
    _write_reports(
        tmp_path,
        {
            "src/a.cc": [
                [2, 1, "warning", "bugprone-a", "use @owner's <b>value</b> from https://x.io"],
                [3, 1, "warning", "bugprone-b", "already posted"],
                [4, 1, "warning", "bugprone-c", "context line, not added"],
            ],
            "src/merged.cc": [[2, 1, "warning", "bugprone-d", "base branch changed this file"]],
        },
    )
    github["files"].append({"filename": "src/merged.cc", "status": "modified", "patch": PATCH})
    github["merge_changed"] = [{"filename": "src/merged.cc", "status": "modified"}]
    posted = AnalysisWarning("clang-tidy", "src/a.cc", 3, 1, "bugprone-b", "already posted")
    github["comments"] = [
        {
            "user": {"type": "Bot"},
            "path": "src/a.cc",
            "line": 3,
            "body": review.comment_body(posted),
        },
        {
            "user": {"type": "User"},
            "path": "src/a.cc",
            "line": 2,
            "body": "<!-- qgc-analysis:" + review.fingerprint(posted) + " -->",
        },
    ]

    assert review.post_review("o/r", RUN, "7", tmp_path) == 1

    (payload,) = github["posted"]
    assert payload["commit_id"] == HEAD
    assert payload["event"] == "COMMENT"
    assert review.REVIEW_MARKER in payload["body"]
    assert "(https://github.com/o/r/actions/runs/5)" in payload["body"]
    assert "partial results" not in payload["body"]
    (comment,) = payload["comments"]
    assert (comment["path"], comment["line"], comment["side"]) == ("src/a.cc", 2, "RIGHT")
    assert "@owner" not in comment["body"]
    assert "<b>" not in comment["body"]
    assert "https://" not in comment["body"]
    assert "`bugprone-a`" in comment["body"]
    # One comparison of the head with the merge commit, not two reads per file.
    assert github["reads"] == [f"repos/o/r/compare/{HEAD}...{MERGE}?per_page=1"]


def test_a_merge_too_large_to_compare_falls_back_to_each_file_s_blobs(
    tmp_path: Path, github: dict[str, Any]
):
    row = [2, 1, "warning", "bugprone-x", "message"]
    _write_reports(tmp_path, {"src/a.cc": [row], "src/merged.cc": [row]})
    github["files"].append({"filename": "src/merged.cc", "status": "modified", "patch": PATCH})
    github["merge_changed"] = [{"filename": f"f{n}"} for n in range(review.COMPARE_FILE_LIMIT)]
    github["blobs"][("src/merged.cc", MERGE)] = "differs"

    assert review.post_review("o/r", RUN, "7", tmp_path) == 1

    (comment,) = github["posted"][0]["comments"]
    assert comment["path"] == "src/a.cc"


def test_a_finding_at_two_columns_of_a_line_is_posted_once(tmp_path: Path, github: dict[str, Any]):
    rows = [[2, column, "warning", "bugprone-x", "message"] for column in (1, 9)]
    _write_reports(tmp_path, {"src/a.cc": rows}, commit=HEAD)

    assert review.post_review("o/r", RUN, "7", tmp_path) == 1

    (payload,) = github["posted"]
    assert "Code Analysis: 1 new bug-class finding (" in payload["body"]


def test_a_comment_github_moved_with_its_line_is_not_posted_again(
    tmp_path: Path, github: dict[str, Any]
):
    # A line inserted above moved the finding from line 9 to 2, and GitHub moved its comment too;
    # the same finding at line 3 is another occurrence.
    rows = [[2, 1, "warning", "bugprone-x", "message"], [3, 1, "warning", "bugprone-x", "message"]]
    _write_reports(tmp_path, {"src/a.cc": rows}, commit=HEAD)
    earlier = AnalysisWarning("clang-tidy", "src/a.cc", 9, 1, "bugprone-x", "message")
    github["comments"] = [
        {
            "user": {"type": "Bot"},
            "path": "src/a.cc",
            "line": 2,
            "body": review.comment_body(earlier),
        }
    ]

    assert review.post_review("o/r", RUN, "7", tmp_path) == 1

    (comment,) = github["posted"][0]["comments"]
    assert comment["line"] == 3


def test_a_marker_inside_a_source_excerpt_does_not_count_as_posted(
    tmp_path: Path, github: dict[str, Any]
):
    _write_reports(
        tmp_path, {"src/a.cc": [[2, 1, "warning", "bugprone-x", "message"]]}, commit=HEAD
    )
    finding = AnalysisWarning("clang-tidy", "src/a.cc", 2, 1, "bugprone-x", "message")
    forged = AnalysisWarning(
        "clang-tidy",
        "src/a.cc",
        2,
        1,
        "other",
        "unrelated",
        context=f"// <!-- qgc-analysis:{review.fingerprint(finding)} -->",
    )
    github["comments"] = [
        {
            "user": {"type": "Bot"},
            "path": "src/a.cc",
            "line": 2,
            "body": review.comment_body(forged),
        }
    ]

    assert review.post_review("o/r", RUN, "7", tmp_path) == 1


def test_only_bug_class_findings_are_posted_inline(tmp_path: Path, github: dict[str, Any]):
    _add_lines(github, 10)
    _write_reports(
        tmp_path,
        {
            "src/a.cc": [
                [1, 1, "warning", "modernize-use-nodiscard", "style"],
                [2, 1, "warning", "performance-unnecessary-value-param", "cost"],
                [3, 1, "warning", "bugprone-narrowing-conversions", "bug"],
                [4, 1, "warning", "clang-analyzer-core.CallAndMessage", "analyzer"],
                [5, 1, "error", "clang-diagnostic-error", "does not compile"],
            ]
        },
        commit=HEAD,
    )
    _write_reports(
        tmp_path,
        {
            "src/a.cc": [
                [6, 1, "warning", "-Wclazy-range-loop-detach", "Qt"],
                [7, 1, "error", "-Wclazy-old-style-connect", "Qt error"],
            ]
        },
        commit=HEAD,
        tool="clazy",
    )

    assert review.post_review("o/r", RUN, "7", tmp_path) == 4

    (payload,) = github["posted"]
    # Errors come first, then the warnings.
    assert [comment["line"] for comment in payload["comments"]] == [5, 7, 3, 4]
    assert "Code Analysis: 4 new bug-class findings (" in payload["body"]


def test_a_pull_request_gets_ten_finding_comments_over_all_its_reviews(
    tmp_path: Path, github: dict[str, Any]
):
    _add_lines(github, 40)
    rows = [[line, 1, "warning", "bugprone-x", f"m{line}"] for line in range(1, 15)]
    _write_reports(tmp_path, {"src/a.cc": rows}, commit=HEAD)
    # A user's comment and the bot's other comments are not finding comments.
    marker = "<!-- qgc-analysis:" + "0" * 64 + " -->"
    github["comments"] = [
        *_posted_before(3),
        {"user": {"type": "User"}, "path": "src/old.cc", "line": 1, "body": marker},
        _bot_comment("src/old.cc", 1, f"A reply quoting {marker}"),
    ]

    assert review.post_review("o/r", RUN, "7", tmp_path) == 7

    (payload,) = github["posted"]
    assert [comment["line"] for comment in payload["comments"]] == list(range(1, 8))
    assert "Code Analysis: 7 of 14 new bug-class findings (" in payload["body"]

    github["comments"] += [
        _bot_comment(comment["path"], comment["line"], comment["body"])
        for comment in payload["comments"]
    ]
    assert review.post_review("o/r", RUN, "7", tmp_path) == 0
    assert len(github["posted"]) == 1


def test_errors_are_posted_past_the_cap_and_count_toward_it(tmp_path: Path, github: dict[str, Any]):
    _add_lines(github, 40)
    errors = [[line, 1, "error", "clang-diagnostic-error", f"e{line}"] for line in (20, 21)]
    warnings = [[line, 1, "warning", "bugprone-x", f"m{line}"] for line in range(1, 15)]
    _write_reports(tmp_path, {"src/a.cc": errors + warnings}, commit=HEAD)

    # With the cap already reached, only the errors are posted.
    github["comments"] = _posted_before(review.MAX_WARNING_COMMENTS_PER_PR)
    assert review.post_review("o/r", RUN, "7", tmp_path) == 2
    assert [comment["line"] for comment in github["posted"][0]["comments"]] == [20, 21]
    assert "Code Analysis: 2 of 16 new bug-class findings (" in github["posted"][0]["body"]

    # Below it, the errors take two of the remaining four comments.
    github["comments"] = _posted_before(review.MAX_WARNING_COMMENTS_PER_PR - 4)
    assert review.post_review("o/r", RUN, "7", tmp_path) == 4
    assert [comment["line"] for comment in github["posted"][1]["comments"]] == [20, 21, 1, 2]


def test_one_review_holds_at_most_max_comments_errors(tmp_path: Path, github: dict[str, Any]):
    _add_lines(github, 40)
    rows = [[line, 1, "error", "clang-diagnostic-error", f"e{line}"] for line in range(1, 41)]
    _write_reports(tmp_path, {"src/a.cc": rows}, commit=HEAD)

    assert review.post_review("o/r", RUN, "7", tmp_path) == review.MAX_COMMENTS


def test_warnings_take_turns_by_check(tmp_path: Path, github: dict[str, Any]):
    _add_lines(github, 40)
    # The frequent check sorts first by name and by line, yet leaves the others their turn.
    rows = [[line, 1, "warning", "bugprone-frequent", f"m{line}"] for line in range(1, 31)]
    rows += [
        [35, 1, "warning", "bugprone-rare", "rare"],
        [40, 1, "warning", "clang-analyzer-core.Rare", "rare too"],
    ]
    _write_reports(tmp_path, {"src/a.cc": rows}, commit=HEAD)

    assert review.post_review("o/r", RUN, "7", tmp_path) == review.MAX_WARNING_COMMENTS_PER_PR

    (payload,) = github["posted"]
    assert [comment["line"] for comment in payload["comments"]] == [1, 35, 40, 2, 3, 4, 5, 6, 7, 8]


def test_review_body_says_what_it_shows_and_where_the_rest_is():
    marker = f"{review.REVIEW_MARKER}\nCode Analysis: "
    kinds = "(errors, and clang-tidy `bugprone-*` and `clang-analyzer-*` warnings)"
    rest = (
        " The Build Results comment summarizes every warning on the changed lines, and the"
        " [analysis run](https://github.com/o/r/actions/runs/5) lists them all."
    )
    url = "https://github.com/o/r/actions/runs/5"

    assert review.review_body(1, 1, url, "success") == (
        f"{marker}1 new bug-class finding {kinds} on lines this pull request changed.{rest}"
    )
    assert review.review_body(10, 32, url, "success") == (
        f"{marker}10 of 32 new bug-class findings {kinds} on lines this pull request changed."
        " Inline comments stop at 10 per pull request, except for errors (at most 25 in one"
        f" review).{rest}"
    )
    assert review.review_body(2, 2, "", "success").endswith("and the analysis run lists them all.")


@pytest.mark.parametrize("conclusion", ["failure", "timed_out", None])
def test_review_of_an_unsuccessful_run_says_its_results_are_partial(
    tmp_path: Path, github: dict[str, Any], conclusion: str | None
):
    _write_reports(
        tmp_path, {"src/a.cc": [[2, 1, "warning", "bugprone-x", "message"]]}, commit=HEAD
    )

    assert review.post_review("o/r", {**RUN, "conclusion": conclusion}, "7", tmp_path) == 1

    (payload,) = github["posted"]
    assert f"`{conclusion or 'no conclusion'}`" in payload["body"]
    assert "partial results" in payload["body"]


@pytest.mark.parametrize(
    "case",
    [
        "newer head",
        "no commit",
        "no message",
        "line outside the diff",
        "no warnings",
        "style warning",
        "performance warning",
    ],
)
def test_nothing_is_posted_without_a_current_located_bug_class_finding(
    tmp_path: Path, github: dict[str, Any], case: str
):
    row = [2, 1, "warning", "bugprone-x", "message"]
    if case == "newer head":
        github["current"] = {"current": "false", "pr": ""}
    elif case == "style warning":
        row[3] = "modernize-use-nodiscard"
    elif case == "performance warning":
        row[3] = "performance-unnecessary-value-param"
    elif case == "no message":
        row = row[:4]
    elif case == "line outside the diff":
        row[0] = 1
    _write_reports(
        tmp_path,
        {} if case == "no warnings" else {"src/a.cc": [row]},
        commit="" if case == "no commit" else MERGE,
    )

    assert review.post_review("o/r", RUN, "7", tmp_path) == 0
    assert github["posted"] == []


def test_api_failures_only_warn(tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys):
    event = tmp_path / "event.json"
    event.write_text(json.dumps({"workflow_run": RUN}))
    monkeypatch.setenv("GITHUB_EVENT_PATH", str(event))
    monkeypatch.setenv("GITHUB_REPOSITORY", "o/r")
    monkeypatch.setenv("PR_NUMBER", "7")

    def fail(*_args: Any, **_kwargs: Any) -> None:
        raise OSError("API unavailable")

    monkeypatch.setattr(review, "post_review", fail)
    assert review.main(["--analysis-dir", str(tmp_path)]) == 0
    assert "::warning::Cannot post the Code Analysis review on #7" in capsys.readouterr().out


def test_other_triggers_review_nothing(tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys):
    event = tmp_path / "event.json"
    event.write_text(json.dumps({"workflow_run": {**RUN, "path": ".github/workflows/linux.yml"}}))
    monkeypatch.setenv("GITHUB_EVENT_PATH", str(event))
    monkeypatch.setenv("PR_NUMBER", "7")
    monkeypatch.setattr(review, "post_review", lambda *_args: pytest.fail("reviewed"))
    assert review.main([]) == 0
    assert "Code Analysis did not trigger this run" in capsys.readouterr().out


def test_errors_are_reviewed_with_their_fenced_source_excerpt(
    tmp_path: Path, github: dict[str, Any]
):
    excerpt = '    2 | auto s = R"(```)";\n      |          ^'
    row = [2, 9, "error", "clang-diagnostic-error", "use of undeclared y", excerpt]
    _write_reports(tmp_path, {"src/a.cc": [row]}, commit=HEAD)

    assert review.post_review("o/r", RUN, "7", tmp_path) == 1

    (comment,) = github["posted"][0]["comments"]
    assert "**clang-tidy** error `clang-diagnostic-error`: use of undeclared y" in comment["body"]
    # The fence outlasts any backtick run in the excerpt, so the excerpt stays literal.
    assert comment["body"].endswith(f"````\n{excerpt}\n````")
