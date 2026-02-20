#!/usr/bin/env python3
"""Download build artifacts from completed platform workflow runs.

Uses the GitHub CLI (gh) to query the GitHub API and download artifacts
from all successful platform builds for a given commit SHA.

Usage:
    python3 tools/setup/download_artifacts.py \\
        --repo owner/repo \\
        --head-sha abc123 \\
        --output-dir artifacts \\
        --workflows Linux,Windows,MacOS,Android
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path


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


def gh(*args: str, check: bool = True) -> subprocess.CompletedProcess:
    """Run a gh CLI command and return the result."""
    cmd = ["gh", *args]
    return subprocess.run(cmd, capture_output=True, text=True, check=check)


def get_workflow_runs(repo: str, head_sha: str, workflows: list[str]) -> list[dict]:
    """Return completed successful runs for the given workflows and commit."""
    result = gh(
        "api",
        f"repos/{repo}/actions/runs?head_sha={head_sha}&per_page=100",
        "--jq",
        ".workflow_runs[]",
    )

    runs = []
    for line in result.stdout.strip().splitlines():
        if not line.strip():
            continue
        try:
            run = json.loads(line)
        except json.JSONDecodeError:
            continue
        if run.get("name") not in workflows:
            continue
        if run.get("status") != "completed" or run.get("conclusion") != "success":
            continue
        runs.append(run)
    return runs


def download_run_artifacts(run_id: int, repo: str, output_dir: Path) -> bool:
    """Download all artifacts from a workflow run. Returns True on success."""
    result = gh(
        "run", "download", str(run_id),
        "--dir", str(output_dir),
        "--repo", repo,
        check=False,
    )
    if result.returncode != 0:
        print(f"  Warning: some artifacts failed to download: {result.stderr.strip()}")
        return False
    return True


def list_downloaded_artifacts(output_dir: Path) -> list[Path]:
    """Return all artifact files under output_dir matching known extensions."""
    files = []
    for ext in ARTIFACT_EXTENSIONS:
        files.extend(output_dir.rglob(f"*{ext}"))
    return sorted(files)


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
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    repo = args.repo
    head_sha = args.head_sha
    output_dir = args.output_dir
    workflows = [w.strip() for w in args.workflows.split(",") if w.strip()]

    if not head_sha:
        print("Error: --head-sha is required", file=sys.stderr)
        return 1

    print(f"Finding completed workflow runs for commit {head_sha}...")

    runs = get_workflow_runs(repo, head_sha, workflows)

    if not runs:
        print(f"No completed successful workflow runs found for SHA {head_sha}")
        print("Checking what runs exist for this SHA...")
        result = gh(
            "api",
            f"repos/{repo}/actions/runs?head_sha={head_sha}&per_page=20",
            "--jq",
            r'.workflow_runs[] | "  \(.name): \(.status)/\(.conclusion)"',
            check=False,
        )
        if result.stdout.strip():
            print(result.stdout.strip())
        return 0

    print(f"Found {len(runs)} workflow run(s):")
    for run in runs:
        print(f"  - {run['name']} (run {run['id']})")

    output_dir.mkdir(parents=True, exist_ok=True)

    failed = 0
    for run in runs:
        print(f"Downloading artifacts from {run['name']} (run {run['id']})...")
        if not download_run_artifacts(run["id"], repo, output_dir):
            failed += 1

    files = list_downloaded_artifacts(output_dir)
    if not files:
        print("No artifact files found after download")
    else:
        print(f"Downloaded {len(files)} artifact file(s):")
        for f in files:
            size_mb = f.stat().st_size / 1024 / 1024
            print(f"  - {f.name}: {size_mb:.1f} MB")

    if failed and not files:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
