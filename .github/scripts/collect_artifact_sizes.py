#!/usr/bin/env python3
"""Collect artifact size metadata for latest successful platform workflow runs."""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import sys
from pathlib import Path
from typing import Any

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import list_run_artifacts, list_workflow_runs_for_sha, parse_csv_list
from common.github_runs import select_latest_runs_by_name


_DISTRIBUTABLE_PREFIXES = (
    "QGroundControl",
    "Custom-QGroundControl",
    "qgc-binary",
)

_EXCLUDED_PREFIXES = (
    "test-results",
    "test-duration",
    "emulator-diagnostics",
)

_EXCLUDED_EXACT_NAMES = {
    "coverage-report",
    "size-metrics",
}


def _is_distributable_artifact(name: str) -> bool:
    if name in _EXCLUDED_EXACT_NAMES:
        return False
    if any(name.startswith(prefix) for prefix in _EXCLUDED_PREFIXES):
        return False
    return any(name.startswith(prefix) for prefix in _DISTRIBUTABLE_PREFIXES)

def latest_successful_runs(
    runs: list[dict[str, Any]],
    platforms: list[str],
    *,
    event: str = "",
) -> dict[str, dict[str, Any]]:
    return select_latest_runs_by_name(
        runs,
        set(platforms),
        event=event,
        status="completed",
        conclusion="success",
    )


def format_size_human(size_bytes: int) -> str:
    size_mb = size_bytes / 1024 / 1024
    if size_mb >= 1024:
        return f"{(size_mb / 1024):.2f} GB"
    return f"{size_mb:.2f} MB"


def collect_artifacts(
    repo: str,
    latest_runs: dict[str, dict[str, Any]],
    platforms: list[str],
    artifacts_by_run_id: dict[int, list[dict[str, Any]]] | None = None,
) -> list[dict[str, Any]]:
    artifacts: list[dict[str, Any]] = []
    run_ids: list[int] = []

    for platform in platforms:
        run = latest_runs.get(platform)
        if not run:
            continue

        run_id = int(run.get("id", 0))
        if run_id <= 0:
            continue
        if artifacts_by_run_id and run_id in artifacts_by_run_id:
            for artifact in artifacts_by_run_id[run_id]:
                name = str(artifact.get("name", ""))
                if not _is_distributable_artifact(name):
                    continue
                size_bytes = int(artifact.get("size_in_bytes", 0))
                artifacts.append(
                    {
                        "name": name,
                        "size_bytes": size_bytes,
                        "size_human": format_size_human(size_bytes),
                    }
                )
        else:
            run_ids.append(run_id)

    if not run_ids:
        artifacts.sort(key=lambda item: str(item.get("name", "")))
        return artifacts

    max_workers = min(len(run_ids), 4)
    with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as pool:
        future_to_run = {
            pool.submit(list_run_artifacts, repo, run_id): run_id
            for run_id in run_ids
        }
        for future in concurrent.futures.as_completed(future_to_run):
            run_id = future_to_run[future]
            try:
                run_artifacts = future.result()
            except Exception as exc:
                print(f"Warning: failed to list artifacts for run {run_id}: {exc}", file=sys.stderr)
                continue
            for artifact in run_artifacts:
                name = str(artifact.get("name", ""))
                if not _is_distributable_artifact(name):
                    continue

                size_bytes = int(artifact.get("size_in_bytes", 0))
                artifacts.append(
                    {
                        "name": name,
                        "size_bytes": size_bytes,
                        "size_human": format_size_human(size_bytes),
                    }
                )

    artifacts.sort(key=lambda item: str(item.get("name", "")))
    return artifacts


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Collect artifact sizes for build-results.")
    parser.add_argument("--repo", required=True, help="Repository in owner/repo format")
    parser.add_argument("--head-sha", required=True, help="Commit SHA to inspect")
    parser.add_argument(
        "--platform-workflows",
        default="Linux,Windows,MacOS,Android",
        help="Comma-separated platform workflow names",
    )
    parser.add_argument(
        "--event",
        default="",
        choices=["", "push", "pull_request", "workflow_dispatch", "schedule"],
        help="Optional workflow event name to filter runs by",
    )
    parser.add_argument(
        "--output-file",
        default="artifact-sizes.json",
        help="Output JSON file path",
    )
    parser.add_argument(
        "--runs-file",
        default="",
        help="Path to cached workflow runs JSON (skips API call if provided)",
    )
    parser.add_argument(
        "--artifacts-file",
        default="",
        help="Path to run artifact metadata JSON (name + size_in_bytes) to avoid per-run API calls",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    platforms = parse_csv_list(args.platform_workflows)
    event = str(args.event).strip()

    if args.runs_file:
        try:
            with open(args.runs_file, encoding="utf-8") as f:
                loaded_runs = json.load(f)
        except (json.JSONDecodeError, OSError) as e:
            print(f"Error: failed to read runs file {args.runs_file}: {e}", file=sys.stderr)
            return 1
        if not isinstance(loaded_runs, list):
            print(
                f"Error: runs file {args.runs_file} must contain a JSON list of workflow runs",
                file=sys.stderr,
            )
            return 1
        runs = loaded_runs
    else:
        runs = list_workflow_runs_for_sha(args.repo, args.head_sha)
    artifacts_by_run_id: dict[int, list[dict[str, Any]]] = {}
    if args.artifacts_file:
        artifacts_path = Path(args.artifacts_file)
        if artifacts_path.exists():
            try:
                data = json.loads(artifacts_path.read_text(encoding="utf-8"))
            except (json.JSONDecodeError, OSError):
                data = {}
            runs_data = data.get("runs", {})
            if isinstance(runs_data, dict):
                for run_id_str, run_artifacts in runs_data.items():
                    try:
                        run_id = int(run_id_str)
                    except (TypeError, ValueError):
                        continue
                    if isinstance(run_artifacts, list):
                        artifacts_by_run_id[run_id] = [
                            artifact for artifact in run_artifacts if isinstance(artifact, dict)
                        ]

    latest = latest_successful_runs(runs, platforms, event=event)
    artifacts = collect_artifacts(
        args.repo,
        latest,
        platforms,
        artifacts_by_run_id=artifacts_by_run_id,
    )

    output_path = Path(args.output_file)
    if artifacts:
        output_path.write_text(json.dumps({"artifacts": artifacts}, indent=2), encoding="utf-8")
        print(f"Wrote {len(artifacts)} artifacts to {output_path}")
    else:
        print("No artifacts found")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
