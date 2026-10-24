#!/usr/bin/env python3
"""Resolve a completed pull_request build's PR, and publish its build-results comment.

Outputs `pr` and `base_sha` for the open PR whose head is the build's commit, and `current`
while that PR head still points at the commit. `base_sha` approximates the base commit the build
merged the PR into: the base branch's tip when the build's run was created, else the PR's
`base.sha`. With --comment, it then creates or updates the PR's one build-results comment from
that file, but only while the build is still current.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path
from typing import Any
from urllib.parse import quote, urlencode

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import gh, gh_error, require_repository, write_github_output
from common.gh_actions import gh_api_json as api
from common.io import write_json

from ci.reporting.workflow_runs import triggering_run

COMMENT_MARKER = "<!-- build-results -->"
# Marks the comments thollander/actions-comment-pull-request posted before this script did.
_LEGACY_MARKER = '<!-- thollander/actions-comment-pull-request "build-results" -->'


def tested_base(repo: str, run: dict[str, Any], pr: dict[str, Any]) -> str:
    """Return the base-branch tip when *run* was created, else the PR's ``base.sha``.

    A pull_request run builds GitHub's merge of the PR head into the base-branch tip at its event;
    the PR's ``base.sha`` can be weeks older, since pushes to the PR need not update it. The run's
    ``created_at`` is the event time (a re-run builds the same merge while ``run_started_at``
    moves), and base-branch commits are squash merges whose committer dates track when they landed.
    """
    fallback = str(pr["base"]["sha"])
    branch = str(pr["base"].get("ref") or "")
    created = str(run.get("created_at") or "")
    if not branch or not created:
        reason = "the PR has no base branch or the run no creation time"
    else:
        query = urlencode({"sha": branch, "until": created, "per_page": 1})
        try:
            commits = api(f"repos/{repo}/commits?{query}")
        except (OSError, subprocess.SubprocessError, ValueError) as exc:
            detail = str(getattr(exc, "stderr", None) or exc).strip()
            reason = f"looking up {branch} at {created} failed: {detail}"
        else:
            first = commits[0] if isinstance(commits, list) and commits else None
            if isinstance(first, dict) and first.get("sha"):
                return str(first["sha"])
            reason = f"{branch} has no commit before {created}"
    print(f"Comparing with the PR's base.sha {fallback}: {reason}")
    return fallback


def resolve_context(repo: str, run: dict[str, Any]) -> dict[str, str]:
    sha = run["head_sha"]
    result = {"pr": "", "current": "false", "sha": sha, "base_sha": ""}
    if run.get("event") != "pull_request":
        return result
    candidates = run.get("pull_requests", [])
    owner = (run.get("head_repository") or {}).get("owner", {}).get("login")
    if not candidates and owner and run.get("head_branch"):
        # Fork PRs leave pull_requests empty, and the base repository's commit lookup does not
        # see commits that exist only in the fork.
        head = f"{quote(owner, safe='')}:{quote(run['head_branch'], safe='')}"
        candidates = api(f"repos/{repo}/pulls?state=open&head={head}")
    if not candidates:
        candidates = api(f"repos/{repo}/commits/{sha}/pulls")
    for candidate in candidates:
        pr = api(f"repos/{repo}/pulls/{int(candidate['number'])}")
        if pr["state"] == "open" and pr["head"]["sha"] == sha:
            return {
                **result,
                "pr": str(pr["number"]),
                "current": "true",
                "base_sha": tested_base(repo, run, pr),
            }
    return result


def find_comment(repo: str, pr: str) -> int | None:
    """Return the id of the PR's build-results comment, or None before the first one."""
    listing = gh(
        "api",
        "--paginate",
        "--jq",
        ".[] | {id, type: .user.type, body}",
        f"repos/{repo}/issues/{pr}/comments?per_page=100",
        retry_transient=True,
    )
    for line in listing.stdout.splitlines():
        if not line.strip():
            continue
        comment = json.loads(line)
        body = str(comment.get("body") or "")
        if comment.get("type") == "Bot" and (COMMENT_MARKER in body or _LEGACY_MARKER in body):
            return int(comment["id"])
    return None


def publish_comment(repo: str, pr: str, path: Path) -> None:
    """Create or update the PR's build-results comment with the Markdown in *path*."""
    payload = path.with_name(f"{path.name}.payload.json")
    write_json(
        payload, {"body": f"{path.read_text(encoding='utf-8').rstrip()}\n\n{COMMENT_MARKER}\n"}
    )
    comment = find_comment(repo, pr)
    if comment is None:
        gh("api", "--method", "POST", f"repos/{repo}/issues/{pr}/comments", "--input", str(payload))
        print(f"Posted the build-results comment on #{pr}")
    else:
        gh(
            "api",
            "--method",
            "PATCH",
            f"repos/{repo}/issues/comments/{comment}",
            "--input",
            str(payload),
        )
        print(f"Updated build-results comment {comment} on #{pr}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--comment",
        type=Path,
        default=None,
        help="Post this Markdown file as the PR's build-results comment while the build is current",
    )
    args = parser.parse_args(argv)
    run = triggering_run()
    if not run:
        gh_error("GITHUB_EVENT_PATH holds no workflow_run event")
        return 1
    repo = require_repository()
    result = resolve_context(repo, run)
    write_github_output(result)
    print(json.dumps(result))
    if args.comment is None:
        return 0
    if result["current"] != "true":
        print("The pull request is closed or has a newer head; not publishing")
        return 0
    try:
        publish_comment(repo, result["pr"], args.comment)
    except (OSError, subprocess.SubprocessError) as exc:
        detail = str(getattr(exc, "stderr", None) or exc).strip()
        gh_error(f"Cannot publish the build-results comment on #{result['pr']}: {detail}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
