#!/usr/bin/env python3
"""Dispatch release builds, wait on their exact run IDs, and save a download snapshot."""

from __future__ import annotations

import argparse
import json
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh, list_workflow_runs_for_sha
from common.io import write_json

WORKFLOWS = {
    "Linux": "linux.yml",
    "Windows": "windows.yml",
    "MacOS": "macos.yml",
    "Android": "android.yml",
    "iOS": "ios.yml",
}


def identify_runs(runs: list[dict[str, Any]], tag: str, since: str) -> dict[str, dict[str, Any]]:
    selected: dict[str, dict[str, Any]] = {}
    for run in runs:
        name = run.get("name", "")
        if (
            name not in WORKFLOWS
            or run.get("event") != "workflow_dispatch"
            or run.get("head_branch") != tag
            or run.get("created_at", "") < since
        ):
            continue
        if name in selected:
            raise RuntimeError(f"Ambiguous release builds for {name}: multiple dispatches at {tag}")
        selected[name] = run
    return selected


def wait_for_builds(repo: str, sha: str, tag: str, output: Path, timeout: int = 9600) -> None:
    since = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    for workflow in WORKFLOWS.values():
        gh("workflow", "run", workflow, "--repo", repo, "--ref", tag)
    deadline = time.monotonic() + timeout
    selected: dict[str, dict[str, Any]] = {}
    while time.monotonic() < deadline:
        if len(selected) < len(WORKFLOWS):
            discovered = identify_runs(list_workflow_runs_for_sha(repo, sha), tag, since)
            for name, run in discovered.items():
                if name in selected and selected[name]["id"] != run["id"]:
                    raise RuntimeError(f"Release run changed for {name}")
                selected[name] = run
        current = [
            json.loads(gh("api", f"repos/{repo}/actions/runs/{run['id']}").stdout)
            for run in selected.values()
        ]
        for run in current:
            if run["head_sha"] != sha or run["head_branch"] != tag:
                raise RuntimeError("Release run identity changed")
            if run["status"] == "completed" and run["conclusion"] != "success":
                raise RuntimeError(f"Release build failed: {run['html_url']} ({run['conclusion']})")
        if len(current) == len(WORKFLOWS) and all(run["status"] == "completed" for run in current):
            write_json(output, current)
            return
        print(
            f"Waiting for release builds ({len(current)}/{len(WORKFLOWS)} identified)", flush=True
        )
        time.sleep(30)
    raise TimeoutError("Release builds did not complete before the deadline")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", required=True)
    parser.add_argument("--sha", required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    wait_for_builds(args.repo, args.sha, args.tag, args.output)


if __name__ == "__main__":
    main()
