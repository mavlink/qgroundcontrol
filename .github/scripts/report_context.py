#!/usr/bin/env python3
"""Resolve a completed build's current PR or default-branch reporting context."""

from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh, require_repository, write_github_output
from common.io import read_json


def api(path: str) -> Any:
    return json.loads(gh("api", path).stdout)


def resolve_context(repo: str, run: dict[str, Any]) -> dict[str, str]:
    sha = run["head_sha"]
    result = {"pr": "", "current": "false", "sha": sha}
    if run.get("event") == "pull_request":
        candidates = run.get("pull_requests", [])
        if not candidates:
            candidates = api(f"repos/{repo}/commits/{sha}/pulls")
        for candidate in candidates:
            pr = api(f"repos/{repo}/pulls/{int(candidate['number'])}")
            if pr["state"] == "open" and pr["head"]["sha"] == sha:
                return {**result, "pr": str(pr["number"]), "current": "true"}
    elif run.get("event") == "push" and run.get("head_branch") == "master":
        head = api(f"repos/{repo}/git/ref/heads/master")["object"]["sha"]
        result["current"] = str(head == sha).lower()
    return result


def main() -> int:
    event = read_json(Path(os.environ["GITHUB_EVENT_PATH"]))
    result = resolve_context(require_repository(), event["workflow_run"])
    write_github_output(result)
    print(json.dumps(result))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
