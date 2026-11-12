#!/usr/bin/env python3
"""Post a pull request's Code Analysis findings on its changed lines as one inline review.

build-results.yml's analysis-review job runs this with trusted default-branch code when a pull
request's Code Analysis run completes, over the ``*-diagnostics.json`` reports it downloaded.
Only bug-class findings are posted (common.analysis_diagnostics.is_bug_class: errors, and
clang-tidy's bugprone-* and clang-analyzer-* warnings); the Build Results comment summarizes
every warning. A pull request gets at most MAX_WARNING_COMMENTS_PER_PR finding comments over all
its reviews, counting those it already has, outdated ones too. Errors are posted past that cap,
at most MAX_COMMENTS in one review, and warnings take turns by check, so one frequent check
cannot fill every slot.
Those reports come from the PR's own code, so a finding is posted only on a line the PR's GitHub
diff adds, and its message is escaped and its source excerpt fenced. Code Analysis numbers lines
in GitHub's merge commit, so a file whose merge-commit content differs from the PR head's is
skipped. A finding is posted once per place: never twice in one review, nor again after a review
posted it. Failures only warn; the analysis run's logs and artifacts still hold every finding.
"""

from __future__ import annotations

import argparse
import hashlib
import itertools
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any
from urllib.parse import quote

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.analysis_diagnostics import (
    AnalysisWarning,
    is_bug_class,
    load_analysis_commit,
    load_changed_findings,
)
from common.gh_actions import gh, gh_warning, require_repository
from common.gh_actions import gh_api_json as api
from common.io import write_json
from common.markdown import code_block, code_span, escape_text

from ci.reporting.report_context import resolve_context
from ci.reporting.workflow_runs import triggering_run

ANALYSIS_WORKFLOW = ".github/workflows/analysis.yml"
# One review's comments; only errors can reach it.
MAX_COMMENTS = 25
# A pull request's finding comments over all its reviews; errors are posted past it.
MAX_WARNING_COMMENTS_PER_PR = 10
# GitHub's compare API lists at most this many changed files.
COMPARE_FILE_LIMIT = 300
# build-results.yml downloads the Code Analysis artifacts here.
DEFAULT_ANALYSIS_DIR = Path("analysis-artifacts")
REVIEW_MARKER = "<!-- qgc-analysis-review -->"
# Counted only at the start of a comment body, never inside a PR's own source excerpt.
_FINDING_MARKER = re.compile(r"<!-- qgc-analysis:([0-9a-f]{64}) -->")
_HUNK = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,\d+)? @@")


def added_lines(patch: str) -> set[int]:
    """Return the line numbers that a GitHub file patch adds on its RIGHT side."""
    lines: set[int] = set()
    number: int | None = None
    # Only "\n" ends a diff line; splitlines() also splits on form feeds and other separators.
    for text in patch.split("\n"):
        if hunk := _HUNK.match(text):
            number = int(hunk[1])
        elif number is None:
            continue
        elif text.startswith("+"):
            lines.add(number)
            number += 1
        elif text.startswith(" "):
            number += 1
        elif not text.startswith(("-", "\\")):
            number = None
    return lines


def fingerprint(finding: AnalysisWarning) -> str:
    """Identify a finding apart from its line, which moves as the pull request changes."""
    identity = [finding.tool, finding.file, finding.check, finding.message]
    return hashlib.sha256(json.dumps(identity).encode()).hexdigest()


def comment_body(finding: AnalysisWarning) -> str:
    check = f" {code_span(finding.check)}" if finding.check else ""
    body = (
        f"<!-- qgc-analysis:{fingerprint(finding)} -->\n**{escape_text(finding.tool)}** "
        f"{escape_text(finding.severity)}{check}: {escape_text(finding.message, break_urls=True)}"
    )
    return f"{body}\n\n{code_block(finding.context)}" if finding.context.strip() else body


def _list(path: str) -> list[dict[str, Any]]:
    listing = gh("api", "--paginate", "--jq", ".[]", path, retry_transient=True)
    return [json.loads(line) for line in listing.stdout.splitlines() if line.strip()]


def _blob(repo: str, path: str, ref: str) -> str:
    """Return *path*'s blob SHA at *ref*, or '' when it cannot be read."""
    try:
        data = api(f"repos/{repo}/contents/{quote(path)}?ref={ref}")
    except (OSError, ValueError, subprocess.SubprocessError):
        return ""
    return str(data.get("sha") or "") if isinstance(data, dict) else ""


def _merge_changed(repo: str, head: str, commit: str, paths: set[str]) -> set[str]:
    """Return the files whose content at *commit* may differ from *head*'s, covering *paths*."""
    # Only the first page lists files, so one commit per page keeps the response small.
    files = api(f"repos/{repo}/compare/{head}...{commit}?per_page=1")["files"]
    if len(files) < COMPARE_FILE_LIMIT:
        return {
            name
            for file in files
            for name in (file.get("filename"), file.get("previous_filename"))
            if name
        }
    return {
        path
        for path in paths
        if not ((blob := _blob(repo, path, head)) and blob == _blob(repo, path, commit))
    }


def _taking_turns(warnings: list[AnalysisWarning]) -> list[AnalysisWarning]:
    """Order *warnings* as each check's first, then each check's second, and so on.

    Checks go by name, and a check's warnings keep their order.
    """
    by_check: dict[str, list[AnalysisWarning]] = {}
    for warning in warnings:
        by_check.setdefault(warning.check, []).append(warning)
    turns = itertools.zip_longest(*(by_check[check] for check in sorted(by_check)))
    return [warning for turn in turns for warning in turn if warning is not None]


def new_comments(
    repo: str, pr: str, head: str, analysis_dir: Path
) -> tuple[list[dict[str, Any]], int]:
    """Return the review comments to post and how many unposted bug-class findings qualified."""
    findings = sorted(
        (
            finding
            for finding in load_changed_findings(analysis_dir)
            if finding.message and is_bug_class(finding)
        ),
        key=lambda w: (w.file, w.line, w.column, w.tool, w.check, w.severity),
    )
    commit = load_analysis_commit(analysis_dir)
    if not findings or not commit:
        return [], 0
    added = {
        file["filename"]: added_lines(str(file.get("patch") or ""))
        for file in _list(f"repos/{repo}/pulls/{pr}/files?per_page=100")
        if file.get("status") != "removed"
    }
    findings = [finding for finding in findings if finding.line in added.get(finding.file, ())]
    if findings and commit != head:
        changed = _merge_changed(repo, head, commit, {finding.file for finding in findings})
        findings = [finding for finding in findings if finding.file not in changed]
    # GitHub keeps a comment's line current as the diff moves (None once outdated), so a finding
    # was posted when a bot comment with its fingerprint sits at its path and line now.
    posted = [
        (marker[1], comment.get("path"), comment.get("line"))
        for comment in _list(f"repos/{repo}/pulls/{pr}/comments?per_page=100")
        if (comment.get("user") or {}).get("type") == "Bot"
        and (marker := _FINDING_MARKER.match(str(comment.get("body") or "")))
    ]
    seen = set(posted)
    unposted = []
    for finding in findings:
        if (key := (fingerprint(finding), finding.file, finding.line)) not in seen:
            seen.add(key)
            unposted.append(finding)
    errors = [finding for finding in unposted if finding.severity == "error"]
    warnings = _taking_turns([finding for finding in unposted if finding.severity != "error"])
    # The cap counts every finding comment so far, outdated ones too, and this review's errors.
    remaining = max(0, MAX_WARNING_COMMENTS_PER_PR - len(posted) - len(errors))
    comments = [
        {"path": w.file, "line": w.line, "side": "RIGHT", "body": comment_body(w)}
        for w in [*errors, *warnings[:remaining]][:MAX_COMMENTS]
    ]
    return comments, len(unposted)


def review_body(count: int, total: int, run_url: str, conclusion: str) -> str:
    shown = f"{count} of {total}" if total > count else str(count)
    noun = "finding" if total == 1 else "findings"
    body = (
        f"{REVIEW_MARKER}\nCode Analysis: {shown} new bug-class {noun} (errors, and clang-tidy "
        "`bugprone-*` and `clang-analyzer-*` warnings) on lines this pull request changed."
    )
    if total > count:
        body += (
            f" Inline comments stop at {MAX_WARNING_COMMENTS_PER_PR} per pull request, except"
            f" for errors (at most {MAX_COMMENTS} in one review)."
        )
    run = f"[analysis run]({run_url})" if run_url else "analysis run"
    body += (
        " The Build Results comment summarizes every warning on the changed lines, and the"
        f" {run} lists them all."
    )
    if conclusion != "success":
        body += (
            f"\n\nThe analysis run ended with {code_span(conclusion or 'no conclusion')}, so these"
            " are partial results: other changed lines may still have findings."
        )
    return body


def post_review(repo: str, run: dict[str, Any], pr: str, analysis_dir: Path) -> int:
    head = str(run["head_sha"])
    comments, total = new_comments(repo, pr, head, analysis_dir)
    if not comments:
        reason = f"{total} are past the cap for this pull request" if total else "none are new"
        print(f"No bug-class Code Analysis findings to post on changed lines: {reason}")
        return 0
    run_url = str(run.get("html_url") or "")
    if not run_url.startswith(f"https://github.com/{repo}/actions/runs/"):
        run_url = ""
    # The PR may have moved on while the reports were read; post only on the analyzed head.
    context = resolve_context(repo, run)
    if context["current"] != "true" or context["pr"] != pr:
        print("The pull request is closed or has a newer head; not reviewing")
        return 0
    with tempfile.TemporaryDirectory() as temporary:
        payload = Path(temporary) / "review.json"
        write_json(
            payload,
            {
                "commit_id": head,
                "event": "COMMENT",
                "body": review_body(
                    len(comments), total, run_url, str(run.get("conclusion") or "")
                ),
                "comments": comments,
            },
        )
        gh("api", "--method", "POST", f"repos/{repo}/pulls/{pr}/reviews", "--input", str(payload))
    print(f"Posted {len(comments)} Code Analysis review comments on #{pr}")
    return len(comments)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--analysis-dir", type=Path, default=DEFAULT_ANALYSIS_DIR)
    args = parser.parse_args(argv)
    pr = os.environ.get("PR_NUMBER", "")
    run = triggering_run()
    if run.get("path") != ANALYSIS_WORKFLOW:
        print("Code Analysis did not trigger this run; nothing to review")
        return 0
    if not pr.isdecimal() or not run.get("head_sha"):
        gh_warning("No pull request to review; build-results.yml sets PR_NUMBER")
        return 0
    try:
        post_review(require_repository(), run, pr, args.analysis_dir)
    except (OSError, ValueError, KeyError, TypeError, subprocess.SubprocessError) as error:
        detail = str(getattr(error, "stderr", None) or error).strip()
        gh_warning(f"Cannot post the Code Analysis review on #{pr}: {detail}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
