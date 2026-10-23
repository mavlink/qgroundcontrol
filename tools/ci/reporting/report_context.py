#!/usr/bin/env python3
"""Resolve a completed build's current PR or default-branch reporting context."""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Any
from urllib.parse import quote

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import gh_api_json as api
from common.gh_actions import require_repository, write_github_output
from common.io import read_json


def resolve_context(repo: str, run: dict[str, Any]) -> dict[str, str]:
    sha = run["head_sha"]
    result = {"pr": "", "current": "false", "sha": sha, "base_sha": ""}
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
    return result


def main(argv: list[str] | None = None) -> int:
    argparse.ArgumentParser(description=__doc__).parse_args(argv)
    event = read_json(Path(os.environ["GITHUB_EVENT_PATH"]))
    result = resolve_context(require_repository(), event["workflow_run"])
    write_github_output(result)
    print(json.dumps(result))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
