#!/usr/bin/env python3
"""Collect artifact size metadata for latest successful platform workflow runs."""

from __future__ import annotations

import argparse
import concurrent.futures
import sys
from pathlib import Path
from typing import Any

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.artifact_metadata import ArtifactMetadataError, read_run_artifact_metadata
from common.format import format_bytes
from common.gh_actions import list_run_artifacts, list_workflow_runs_for_sha, parse_csv_list
from common.github_runs import (
    add_workflow_run_query_args,
    resolve_workflow_runs,
    select_latest_runs_by_name,
)
from common.io import write_json

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
                        "size_human": format_bytes(size_bytes),
                    }
                )
        else:
            run_ids.append(run_id)

    if run_ids:
        max_workers = min(len(run_ids), 4)
        with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as pool:
            future_to_run = {
                pool.submit(list_run_artifacts, repo, run_id): run_id for run_id in run_ids
            }
            for future in concurrent.futures.as_completed(future_to_run):
                run_id = future_to_run[future]
                try:
                    run_artifacts = future.result()
                except Exception as exc:
                    print(
                        f"Warning: failed to list artifacts for run {run_id}: {exc}",
                        file=sys.stderr,
                    )
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
                            "size_human": format_bytes(size_bytes),
                        }
                    )

    # Deduplicate by name — when multiple workflows produce an artifact with
    # the same name (e.g. Android and macOS both upload "QGroundControl"),
    # keep the largest to avoid misleading size deltas.
    deduped: dict[str, dict[str, Any]] = {}
    for artifact in artifacts:
        name = artifact["name"]
        if name not in deduped or artifact["size_bytes"] > deduped[name]["size_bytes"]:
            deduped[name] = artifact
    return sorted(deduped.values(), key=lambda item: str(item.get("name", "")))


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Collect artifact sizes for build-results.")
    add_workflow_run_query_args(
        parser,
        default_event="",
        runs_option="--runs-file",
        restrict_event=True,
    )
    parser.add_argument(
        "--output-file",
        default="artifact-sizes.json",
        help="Output JSON file path",
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

    runs = resolve_workflow_runs(
        args.repo, args.head_sha, args.runs_file, list_workflow_runs_for_sha
    )
    if runs is None:
        return 1
    artifacts_by_run_id: dict[int, list[dict[str, Any]]] = {}
    if args.artifacts_file:
        artifacts_path = Path(args.artifacts_file)
        if artifacts_path.exists():
            try:
                artifacts_by_run_id = read_run_artifact_metadata(artifacts_path)
            except ArtifactMetadataError as exc:
                print(f"Warning: ignoring invalid artifact metadata: {exc}", file=sys.stderr)

    latest = select_latest_runs_by_name(
        runs, set(platforms), event=event, status="completed", conclusion="success"
    )
    artifacts = collect_artifacts(
        args.repo,
        latest,
        platforms,
        artifacts_by_run_id=artifacts_by_run_id,
    )

    output_path = Path(args.output_file)
    if artifacts:
        write_json(output_path, {"artifacts": artifacts})
        print(f"Wrote {len(artifacts)} artifacts to {output_path}")
    else:
        print("No artifacts found")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
