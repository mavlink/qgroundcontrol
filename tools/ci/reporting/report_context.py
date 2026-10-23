#!/usr/bin/env python3
"""Resolve a completed build's current PR or default-branch reporting context.

Outputs `pr` and `base_sha` for the open PR whose head is the build's commit (set only for
pull_request builds), `baseline` when a successful master push build may save baselines, and
`current` while that PR head or master still points at the commit.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any
from urllib.parse import quote

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import gh_api_json as api
from common.gh_actions import gh_error, require_repository, write_github_output

from ci.reporting.workflow_runs import triggering_run


def resolve_context(repo: str, run: dict[str, Any]) -> dict[str, str]:
    sha = run["head_sha"]
    result = {"pr": "", "current": "false", "sha": sha, "base_sha": "", "baseline": "false"}
    if run.get("event") == "pull_request":
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
                    "base_sha": pr["base"]["sha"],
                }
    elif run.get("event") == "push" and run.get("head_branch") == "master":
        head = api(f"repos/{repo}/git/ref/heads/master")["object"]["sha"]
        result["current"] = str(head == sha).lower()
        result["baseline"] = str(run.get("conclusion") == "success").lower()
    return result


def main(argv: list[str] | None = None) -> int:
    argparse.ArgumentParser(description=__doc__).parse_args(argv)
    run = triggering_run()
    if not run:
        gh_error("GITHUB_EVENT_PATH holds no workflow_run event")
        return 1
    result = resolve_context(require_repository(), run)
    write_github_output(result)
    print(json.dumps(result))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
