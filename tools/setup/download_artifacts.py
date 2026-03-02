#!/usr/bin/env python3
"""Download build artifacts from completed platform workflow runs.

Uses the GitHub CLI (gh) to query the GitHub API and download artifacts
from all successful platform builds for a given commit SHA.

Usage:
    python3 tools/setup/download_artifacts.py \\
        --repo owner/repo \\
        --head-sha abc123 \\
        --output-dir artifacts \\
        --workflows Linux,Windows,MacOS,Android \\
        --artifact-prefixes coverage-report,test-results-
"""

from __future__ import annotations

import argparse
import concurrent.futures
from datetime import datetime
import json
import subprocess
import sys
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[2]
COMMON_DIR = REPO_ROOT / "tools" / "common"
if str(COMMON_DIR) not in sys.path:
    sys.path.insert(0, str(COMMON_DIR))

import gh_actions as _gh_actions


ARTIFACT_EXTENSIONS = (
    ".apk",
    ".dmg",
    ".exe",
    ".AppImage",
    ".ipa",
    ".deb",
    ".rpm",
    ".zip",
)
ARTIFACT_EXTENSION_SET = frozenset(ARTIFACT_EXTENSIONS)


def gh(*args: str, check: bool = True) -> subprocess.CompletedProcess:
    """Run a gh CLI command and return the result."""
    return _gh_actions.gh(*args, check=check)


def _parse_created_at(created_at: Any) -> datetime | None:
    value = str(created_at).strip()
    if not value:
        return None
    if value.endswith("Z"):
        value = f"{value[:-1]}+00:00"
    try:
        return datetime.fromisoformat(value)
    except ValueError:
        return None


def _is_newer_run(candidate: dict[str, Any], existing: dict[str, Any]) -> bool:
    candidate_created_at = str(candidate.get("created_at", ""))
    existing_created_at = str(existing.get("created_at", ""))
    candidate_dt = _parse_created_at(candidate_created_at)
    existing_dt = _parse_created_at(existing_created_at)
    if candidate_dt is not None and existing_dt is not None:
        return candidate_dt > existing_dt
    return candidate_created_at > existing_created_at


def select_latest_successful_runs(
    runs: list[dict[str, Any]],
    workflows: list[str],
) -> list[dict[str, Any]]:
    """Return latest completed/successful run per workflow name."""
    workflow_set = set(workflows)
    latest_by_workflow: dict[str, dict[str, Any]] = {}

    for run in runs:
        name = str(run.get("name", ""))
        if name not in workflow_set:
            continue
        if run.get("status") != "completed" or run.get("conclusion") != "success":
            continue
        existing = latest_by_workflow.get(name)
        if existing is None or _is_newer_run(run, existing):
            latest_by_workflow[name] = run

    order = {name: idx for idx, name in enumerate(workflows)}
    return sorted(
        latest_by_workflow.values(),
        key=lambda run: order.get(str(run.get("name", "")), len(order)),
    )


def group_successful_runs_by_workflow(
    runs: list[dict[str, Any]],
    workflows: list[str],
) -> dict[str, list[dict[str, Any]]]:
    """Group successful runs by workflow name, newest first."""
    workflow_set = set(workflows)
    grouped: dict[str, list[dict[str, Any]]] = {name: [] for name in workflows}

    for run in runs:
        name = str(run.get("name", ""))
        if name not in workflow_set:
            continue
        if run.get("status") != "completed" or run.get("conclusion") != "success":
            continue
        grouped[name].append(run)

    for workflow_name in grouped:
        grouped[workflow_name].sort(
            key=lambda run: (
                _parse_created_at(run.get("created_at")) is not None,
                str(run.get("created_at", "")),
            ),
            reverse=True,
        )

    return grouped


def get_workflow_runs(repo: str, head_sha: str, workflows: list[str]) -> list[dict]:
    """Return completed successful runs for the given workflows and commit."""
    all_runs = _gh_actions.list_workflow_runs_for_sha(repo, head_sha)
    return select_latest_successful_runs(all_runs, workflows)


def select_artifact_names_for_run(repo: str, run_id: int, prefixes: list[str]) -> list[str]:
    """Return artifact names that match configured name prefixes."""
    return select_artifact_names(_gh_actions.list_run_artifacts(repo, run_id), prefixes)


def select_artifact_names(artifacts: list[dict[str, Any]], prefixes: list[str]) -> list[str]:
    """Return artifact names from a run that match configured name prefixes."""
    names: set[str] = set()
    for artifact in artifacts:
        artifact_name = str(artifact.get("name", ""))
        if any(artifact_name.startswith(prefix) for prefix in prefixes):
            names.add(artifact_name)
    return sorted(names)


def download_run_artifacts(
    run_id: int,
    repo: str,
    output_dir: Path,
    artifact_names: list[str] | None = None,
) -> bool:
    """Download selected artifacts from a workflow run. Returns True on success."""
    cmd = [
        "run",
        "download",
        str(run_id),
        "--dir",
        str(output_dir),
        "--repo",
        repo,
    ]
    for artifact_name in artifact_names or []:
        cmd.extend(["-n", artifact_name])

    result = gh(*cmd, check=False)
    if result.returncode != 0:
        print(f"  Warning: some artifacts failed to download: {result.stderr.strip()}")
        return False
    return True


def list_downloaded_artifacts(output_dir: Path) -> list[Path]:
    """Return all artifact files under output_dir matching known extensions."""
    files = [
        path for path in output_dir.rglob("*")
        if path.is_file() and path.suffix in ARTIFACT_EXTENSION_SET
    ]
    return sorted(files)


def list_downloaded_files(output_dir: Path) -> list[Path]:
    """Return all downloaded files under output_dir."""
    return sorted(path for path in output_dir.rglob("*") if path.is_file())


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Download build artifacts from completed platform workflow runs.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--repo",
        required=True,
        help="GitHub repository in owner/repo format",
    )
    parser.add_argument(
        "--head-sha",
        required=True,
        help="Commit SHA to find workflow runs for",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("artifacts"),
        help="Directory to download artifacts into (default: artifacts)",
    )
    parser.add_argument(
        "--workflows",
        default="Linux,Windows,MacOS,Android",
        help="Comma-separated workflow names to download from (default: Linux,Windows,MacOS,Android)",
    )
    parser.add_argument(
        "--runs-file",
        default="",
        help="Path to cached workflow runs JSON (skips API call if provided)",
    )
    parser.add_argument(
        "--artifact-prefixes",
        default="",
        help=(
            "Comma-separated artifact name prefixes to download. "
            "If omitted, all artifacts are downloaded."
        ),
    )
    parser.add_argument(
        "--artifact-metadata-out",
        default="",
        help="Optional output JSON path to write run artifact metadata (name + size_in_bytes)",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    repo = args.repo
    head_sha = args.head_sha
    output_dir = args.output_dir
    workflows = [w.strip() for w in args.workflows.split(",") if w.strip()]
    artifact_prefixes = [p.strip() for p in args.artifact_prefixes.split(",") if p.strip()]
    artifact_metadata_out = Path(args.artifact_metadata_out) if args.artifact_metadata_out else None

    if not head_sha:
        print("Error: --head-sha is required", file=sys.stderr)
        return 1

    print(f"Finding completed workflow runs for commit {head_sha}...")

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
        all_runs = loaded_runs
    else:
        all_runs = _gh_actions.list_workflow_runs_for_sha(repo, head_sha)
    preloaded_artifacts: dict[int, list[dict[str, Any]]] = {}
    had_successful_runs = bool(select_latest_successful_runs(all_runs, workflows))
    if artifact_prefixes:
        runs = []
        grouped_runs = group_successful_runs_by_workflow(all_runs, workflows)
        for workflow_name in workflows:
            candidates = grouped_runs.get(workflow_name, [])
            selected_run: dict[str, Any] | None = None
            for candidate in candidates:
                run_id = int(candidate["id"])
                artifacts = _gh_actions.list_run_artifacts(repo, run_id)
                preloaded_artifacts[run_id] = artifacts
                if select_artifact_names(artifacts, artifact_prefixes):
                    selected_run = candidate
                    break
            if selected_run is not None:
                runs.append(selected_run)
    else:
        runs = select_latest_successful_runs(all_runs, workflows)

    if not runs:
        if artifact_prefixes and had_successful_runs:
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

    # Pre-resolve artifact names and/or metadata (requires API calls) before parallel downloads.
    run_artifact_map: dict[int, list[str] | None] = {}
    run_artifact_metadata: dict[str, list[dict[str, Any]]] = {}
    for run in runs:
        run_id = int(run["id"])
        if artifact_prefixes or artifact_metadata_out is not None:
            artifacts = preloaded_artifacts.get(run_id)
            if artifacts is None:
                artifacts = _gh_actions.list_run_artifacts(repo, run_id)
            if artifact_prefixes:
                names = select_artifact_names(artifacts, artifact_prefixes)
                run_artifact_map[run_id] = names if names else None
            else:
                run_artifact_map[run_id] = None

            if artifact_metadata_out is not None:
                run_artifact_metadata[str(run_id)] = [
                    {
                        "name": str(artifact.get("name", "")),
                        "size_in_bytes": int(artifact.get("size_in_bytes", 0)),
                    }
                    for artifact in artifacts
                ]
        else:
            run_artifact_map[run_id] = None

    if artifact_metadata_out is not None:
        artifact_metadata_out.parent.mkdir(parents=True, exist_ok=True)
        artifact_metadata_out.write_text(
            json.dumps({"runs": run_artifact_metadata}, indent=2) + "\n",
            encoding="utf-8",
        )
        print(f"Wrote artifact metadata for {len(run_artifact_metadata)} run(s) to {artifact_metadata_out}")

    def _download(run: dict) -> tuple[str, bool]:
        name = run["name"]
        run_id = int(run["id"])
        artifact_names = run_artifact_map.get(run_id)
        if artifact_prefixes and artifact_names is None:
            return name, True
        ok = download_run_artifacts(run_id, repo, output_dir, artifact_names=artifact_names)
        return name, ok

    # Filter out runs with no matching artifacts before spawning threads
    downloadable = [r for r in runs if not artifact_prefixes or run_artifact_map.get(int(r["id"])) is not None]
    for run in runs:
        if artifact_prefixes and run_artifact_map.get(int(run["id"])) is None:
            print(f"Skipping {run['name']} (run {run['id']}): no artifacts match requested prefixes")

    if artifact_prefixes and not downloadable:
        print(
            f"Warning: no artifacts matched prefixes {artifact_prefixes!r} "
            f"across {len(runs)} workflow run(s)",
            file=sys.stderr,
        )

    for run in downloadable:
        run_id = int(run["id"])
        artifact_names = run_artifact_map.get(run_id)
        if artifact_names:
            print(f"Downloading {len(artifact_names)} filtered artifact(s) from {run['name']} (run {run_id})...")
        else:
            print(f"Downloading artifacts from {run['name']} (run {run_id})...")

    failed = 0
    if downloadable:
        with concurrent.futures.ThreadPoolExecutor(max_workers=min(len(downloadable), 8)) as pool:
            for name, ok in pool.map(_download, downloadable):
                if ok:
                    print(f"  Finished: {name}")
                else:
                    print(f"  Failed: {name}")
                    failed += 1

    files = list_downloaded_files(output_dir) if artifact_prefixes else list_downloaded_artifacts(output_dir)
    if not files:
        print("No artifact files found after download")
    else:
        print(f"Downloaded {len(files)} artifact file(s):")
        for path in files:
            size_mb = path.stat().st_size / 1024 / 1024
            print(f"  - {path.name}: {size_mb:.1f} MB")

    if failed and not files:
        return 1
    if artifact_prefixes and not files:
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
