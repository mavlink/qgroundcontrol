#!/usr/bin/env python3
"""Download build artifacts from completed platform workflow runs.

Uses the GitHub CLI (gh) to query the GitHub API and, for a given commit SHA,
download the artifacts whose names match the given prefixes from each platform
workflow's newest successful run that has any (any completed run with
--include-failed). With --sizes-out, it also records the distributable artifact
sizes of each workflow's latest successful run, reusing the artifact listings
it fetched.

Usage:
    python3 tools/ci/reporting/download_artifacts.py \\
        --repo owner/repo \\
        --head-sha abc123 \\
        --output-dir artifacts \\
        --workflows Linux,Windows,MacOS,Android \\
        --artifact-prefixes coverage-report,test-results- \\
        --sizes-out pr-sizes.json
"""

from __future__ import annotations

import argparse
import concurrent.futures
import subprocess
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.format import format_bytes
from common.gh_actions import (
    env_default,
    env_flag,
    gh,
    list_run_artifacts,
    list_workflow_runs_for_sha,
)
from common.io import write_json

from ci.reporting.workflow_runs import (
    add_workflow_run_query_args,
    group_runs_by_name,
    resolve_workflow_runs,
    select_latest_runs_by_name,
)

_DISTRIBUTABLE_PREFIXES = (
    "QGroundControl",
    "Custom-QGroundControl",
    "qgc-binary",
)

_EXCLUDED_PREFIXES = (
    "test-results",
    "emulator-diagnostics",
)

_EXCLUDED_EXACT_NAMES = {
    "coverage-report",
}


def select_latest_successful_runs(
    runs: list[dict[str, Any]],
    workflows: list[str],
    *,
    event: str = "",
) -> list[dict[str, Any]]:
    """Return latest completed/successful run per workflow name."""
    latest_by_workflow = select_latest_runs_by_name(
        runs,
        set(workflows),
        event=event,
        status="completed",
        conclusion="success",
    )
    order = {name: idx for idx, name in enumerate(workflows)}
    return sorted(
        latest_by_workflow.values(),
        key=lambda run: order.get(str(run.get("name", "")), len(order)),
    )


def select_artifact_names(artifacts: list[dict[str, Any]], prefixes: list[str]) -> list[str]:
    """Return artifact names from a run that match configured name prefixes."""
    names: set[str] = set()
    for artifact in artifacts:
        artifact_name = str(artifact.get("name", ""))
        if any(artifact_name.startswith(prefix) for prefix in prefixes):
            names.add(artifact_name)
    return sorted(names)


def _is_distributable_artifact(name: str) -> bool:
    if name in _EXCLUDED_EXACT_NAMES:
        return False
    if any(name.startswith(prefix) for prefix in _EXCLUDED_PREFIXES):
        return False
    return any(name.startswith(prefix) for prefix in _DISTRIBUTABLE_PREFIXES)


def collect_artifact_sizes(
    repo: str,
    runs: list[dict[str, Any]],
    workflows: list[str],
    *,
    event: str = "",
    listed: dict[int, list[dict[str, Any]]] | None = None,
) -> list[dict[str, Any]]:
    """Return distributable artifact sizes from each workflow's latest successful run.

    Uses the artifact listings in ``listed`` and asks the API for any other run.
    When several workflows upload an artifact with the same name (e.g. Android and
    macOS both upload "QGroundControl"), the largest is kept to avoid misleading
    size deltas.
    """
    listed = listed or {}
    run_ids = [
        run_id
        for run in select_latest_successful_runs(runs, workflows, event=event)
        if (run_id := int(run.get("id", 0))) > 0
    ]
    unlisted = [run_id for run_id in run_ids if run_id not in listed]
    fetched: dict[int, list[dict[str, Any]]] = {}
    if unlisted:
        with concurrent.futures.ThreadPoolExecutor(max_workers=min(len(unlisted), 4)) as pool:
            future_to_run = {
                pool.submit(list_run_artifacts, repo, run_id): run_id for run_id in unlisted
            }
            for future in concurrent.futures.as_completed(future_to_run):
                run_id = future_to_run[future]
                try:
                    fetched[run_id] = future.result()
                except Exception as exc:
                    print(
                        f"Warning: failed to list artifacts for run {run_id}: {exc}",
                        file=sys.stderr,
                    )

    sizes: dict[str, dict[str, Any]] = {}
    for run_id in run_ids:
        for artifact in listed.get(run_id, fetched.get(run_id, [])):
            name = str(artifact.get("name", ""))
            if not _is_distributable_artifact(name):
                continue
            size_bytes = int(artifact.get("size_in_bytes", 0))
            if name not in sizes or size_bytes > sizes[name]["size_bytes"]:
                sizes[name] = {
                    "name": name,
                    "size_bytes": size_bytes,
                    "size_human": format_bytes(size_bytes),
                }
    return sorted(sizes.values(), key=lambda item: str(item["name"]))


# A run's artifacts can total several GB; bound the download so a stalled transfer can't hold the job.
DOWNLOAD_TIMEOUT_SECONDS = 1800


def download_run_artifacts(
    run_id: int,
    repo: str,
    output_dir: Path,
    artifact_names: list[str],
) -> bool:
    """Download the named artifacts from a workflow run. Returns True on success."""
    cmd = [
        "run",
        "download",
        str(run_id),
        "--dir",
        str(output_dir),
        "--repo",
        repo,
    ]
    for artifact_name in artifact_names:
        cmd.extend(["-n", artifact_name])

    try:
        result = gh(*cmd, check=False, timeout=DOWNLOAD_TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired:
        print(
            f"  Warning: artifact download for run {run_id} timed out after {DOWNLOAD_TIMEOUT_SECONDS} s"
        )
        return False
    if result.returncode != 0:
        print(f"  Warning: some artifacts failed to download: {result.stderr.strip()}")
        return False
    return True


def list_downloaded_files(output_dir: Path) -> list[Path]:
    """Return all downloaded files under output_dir."""
    return sorted(path for path in output_dir.rglob("*") if path.is_file())


def _cached_runs_file() -> str:
    """Return the download-all-artifacts action's runs-file input when that file exists."""
    path = env_default("INPUT_RUNS_FILE")
    return path if path and Path(path).is_file() else ""


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse arguments; optional ones default to the composite action's INPUT_* step environment."""
    parser = argparse.ArgumentParser(
        description="Download build artifacts from completed platform workflow runs.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    add_workflow_run_query_args(
        parser,
        default_event="",
        workflows_option="--workflows",
        workflows_dest="workflows",
        restrict_event=True,
    )
    parser.set_defaults(runs_file=_cached_runs_file())
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("artifacts"),
        help="Directory to download artifacts into (default: artifacts)",
    )
    parser.add_argument(
        "--artifact-prefixes",
        required=True,
        help="Comma-separated artifact name prefixes to download",
    )
    parser.add_argument(
        "--sizes-out",
        type=Path,
        default=env_default("INPUT_SIZES_FILE") or None,
        help="Write distributable artifact sizes of each latest successful run to this JSON file",
    )
    parser.add_argument(
        "--require-sizes",
        action="store_true",
        default=env_flag("INPUT_REQUIRE_SIZES"),
        help="Fail and remove --sizes-out when no distributable artifact is found",
    )
    parser.add_argument(
        "--include-failed",
        action="store_true",
        default=env_flag("INPUT_INCLUDE_FAILED"),
        help="Download diagnostics from failed completed builds too",
    )
    parser.add_argument(
        "--allow-missing",
        action="store_true",
        default=env_flag("INPUT_ALLOW_MISSING"),
        help="Allow absent diagnostic artifacts selected by prefix",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    repo = args.repo
    head_sha = args.head_sha
    output_dir = args.output_dir
    workflows = [w.strip() for w in args.workflows.split(",") if w.strip()]
    event = str(args.event).strip()
    artifact_prefixes = [p.strip() for p in args.artifact_prefixes.split(",") if p.strip()]

    if not head_sha:
        print("Error: --head-sha is required", file=sys.stderr)
        return 1
    if not artifact_prefixes:
        print("Error: --artifact-prefixes needs at least one prefix", file=sys.stderr)
        return 1
    if args.require_sizes and args.sizes_out is None:
        print("Error: --require-sizes requires --sizes-out", file=sys.stderr)
        return 1

    print(f"Finding completed workflow runs for commit {head_sha}...")

    all_runs = resolve_workflow_runs(repo, head_sha, args.runs_file, list_workflow_runs_for_sha)
    if all_runs is None:
        return 1
    preloaded_artifacts: dict[int, list[dict[str, Any]]] = {}
    had_successful_runs = bool(select_latest_successful_runs(all_runs, workflows, event=event))
    runs: list[dict[str, Any]] = []
    run_artifact_names: dict[int, list[str]] = {}
    grouped_runs = group_runs_by_name(
        all_runs,
        workflows,
        event=event,
        status="completed",
        conclusion="" if args.include_failed else "success",
    )
    for workflow_name in workflows:
        for candidate in grouped_runs.get(workflow_name, []):
            run_id = int(candidate["id"])
            artifacts = list_run_artifacts(repo, run_id)
            preloaded_artifacts[run_id] = artifacts
            names = select_artifact_names(artifacts, artifact_prefixes)
            if names:
                runs.append(candidate)
                run_artifact_names[run_id] = names
                break

    if args.sizes_out is not None:
        sizes = collect_artifact_sizes(
            repo, all_runs, workflows, event=event, listed=preloaded_artifacts
        )
        if not sizes and args.require_sizes:
            args.sizes_out.unlink(missing_ok=True)
            print("Error: no distributable artifacts to record sizes for", file=sys.stderr)
            return 1
        write_json(args.sizes_out, {"head_sha": head_sha, "artifacts": sizes})
        print(f"Wrote {len(sizes)} artifact size(s) to {args.sizes_out}")

    if not runs:
        if args.allow_missing:
            print(
                f"No diagnostic artifacts match prefixes {artifact_prefixes!r} for SHA {head_sha}; "
                "continuing with build status only"
            )
            return 0
        if had_successful_runs:
            print(
                f"No successful workflow runs with artifacts matching prefixes {artifact_prefixes!r} "
                f"were found for SHA {head_sha}"
            )
            return 2
        else:
            print(f"No completed successful workflow runs found for SHA {head_sha}")
            print("Checking what runs exist for this SHA...")
            for run in all_runs[:20]:
                name = str(run.get("name", "unknown"))
                status = str(run.get("status", "unknown"))
                conclusion = str(run.get("conclusion", "unknown"))
                print(f"  {name}: {status}/{conclusion}")
            return 0

    print(f"Found {len(runs)} workflow run(s):")
    for run in runs:
        print(f"  - {run['name']} (run {run['id']})")

    output_dir.mkdir(parents=True, exist_ok=True)

    def _download(run: dict) -> tuple[str, bool]:
        run_id = int(run["id"])
        ok = download_run_artifacts(run_id, repo, output_dir, run_artifact_names[run_id])
        return run["name"], ok

    for run in runs:
        print(
            f"Downloading {len(run_artifact_names[int(run['id'])])} filtered artifact(s) "
            f"from {run['name']} (run {run['id']})..."
        )

    failed = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=min(len(runs), 8)) as pool:
        for name, ok in pool.map(_download, runs):
            if ok:
                print(f"  Finished: {name}")
            else:
                print(f"  Failed: {name}")
                failed += 1

    files = list_downloaded_files(output_dir)
    if not files:
        print("No artifact files found after download")
    else:
        print(f"Downloaded {len(files)} artifact file(s):")
        for path in files:
            size_mb = path.stat().st_size / 1024 / 1024
            print(f"  - {path.name}: {size_mb:.1f} MB")

    if failed:
        return 1
    if not files:
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
