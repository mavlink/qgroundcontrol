"""Inline Code Analysis reviews post only escaped, unposted warnings on lines the PR adds."""

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


def _write_reports(directory: Path, changed: dict[str, list[list[Any]]], commit: str = MERGE):
    report = directory / "clang-tidy-clazy-output-1/clang-tidy-diagnostics.json"
    report.parent.mkdir(parents=True)
    report.write_text(
        json.dumps({"tool": "clang-tidy", "commit": commit, "diagnostics": {}, "changed": changed})
    )


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
                [2, 1, "warning", "check-a", "use @owner's <b>value</b> from https://x.io"],
                [3, 1, "warning", "check-b", "already posted"],
                [4, 1, "warning", "check-c", "context line, not added"],
            ],
            "src/merged.cc": [[2, 1, "warning", "check-d", "base branch changed this file"]],
        },
    )
    github["files"].append({"filename": "src/merged.cc", "status": "modified", "patch": PATCH})
    github["merge_changed"] = [{"filename": "src/merged.cc", "status": "modified"}]
    posted = AnalysisWarning("clang-tidy", "src/a.cc", 3, 1, "check-b", "already posted")
    github["comments"] = [
        {"user": {"type": "Bot"}, "body": review.comment_body(posted)},
        {"user": {"type": "User"}, "body": "<!-- qgc-analysis:" + "0" * 64 + " -->"},
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
    assert "`check-a`" in comment["body"]
    # One comparison of the head with the merge commit, not two reads per file.
    assert github["reads"] == [f"repos/o/r/compare/{HEAD}...{MERGE}?per_page=1"]


def test_a_merge_too_large_to_compare_falls_back_to_each_file_s_blobs(
    tmp_path: Path, github: dict[str, Any]
):
    row = [2, 1, "warning", "check", "message"]
    _write_reports(tmp_path, {"src/a.cc": [row], "src/merged.cc": [row]})
    github["files"].append({"filename": "src/merged.cc", "status": "modified", "patch": PATCH})
    github["merge_changed"] = [{"filename": f"f{n}"} for n in range(review.COMPARE_FILE_LIMIT)]
    github["blobs"][("src/merged.cc", MERGE)] = "differs"

    assert review.post_review("o/r", RUN, "7", tmp_path) == 1

    (comment,) = github["posted"][0]["comments"]
    assert comment["path"] == "src/a.cc"


def test_a_finding_at_two_columns_of_a_line_is_posted_once(tmp_path: Path, github: dict[str, Any]):
    rows = [[2, column, "warning", "check", "message"] for column in (1, 9)]
    _write_reports(tmp_path, {"src/a.cc": rows}, commit=HEAD)

    assert review.post_review("o/r", RUN, "7", tmp_path) == 1

    (payload,) = github["posted"]
    assert "Showing the first" not in payload["body"]


def test_review_is_capped_and_says_how_many_qualified(tmp_path: Path, github: dict[str, Any]):
    github["files"][0]["patch"] = "@@ -0,0 +1,40 @@\n" + "+x\n" * 40
    rows = [[line, 1, "warning", "check", f"m{line}"] for line in range(1, 41)]
    _write_reports(tmp_path, {"src/a.cc": rows}, commit=HEAD)

    assert review.post_review("o/r", RUN, "7", tmp_path) == review.MAX_COMMENTS

    (payload,) = github["posted"]
    assert f"first {review.MAX_COMMENTS} of 40; the analysis run has the rest." in payload["body"]


@pytest.mark.parametrize("conclusion", ["failure", "timed_out", None])
def test_review_of_an_unsuccessful_run_says_its_results_are_partial(
    tmp_path: Path, github: dict[str, Any], conclusion: str | None
):
    _write_reports(tmp_path, {"src/a.cc": [[2, 1, "warning", "check", "message"]]}, commit=HEAD)

    assert review.post_review("o/r", {**RUN, "conclusion": conclusion}, "7", tmp_path) == 1

    (payload,) = github["posted"]
    assert f"`{conclusion or 'no conclusion'}`" in payload["body"]
    assert "partial results" in payload["body"]


@pytest.mark.parametrize(
    "case", ["newer head", "no commit", "no message", "line outside the diff", "no warnings"]
)
def test_nothing_is_posted_without_a_current_located_warning(
    tmp_path: Path, github: dict[str, Any], case: str
):
    row = [2, 1, "warning", "check", "message"]
    if case == "newer head":
        github["current"] = {"current": "false", "pr": ""}
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
