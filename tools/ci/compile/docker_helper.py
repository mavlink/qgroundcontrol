#!/usr/bin/env python3
"""Docker build helpers for CI.

Subcommands:
    run         Run docker build with retry logic
    exec        Run a repository Python script in a builder image
    cache-key   Print the builder image's compiler fingerprint for cache keys
    summary     Record Ninja timings and write the Docker build section of the job summary
"""

from __future__ import annotations

import argparse
import os
import re
import shlex
import subprocess
import sys
from pathlib import Path, PurePosixPath

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.format import format_duration, format_ratio
from common.gh_actions import (
    env_flag,
    gh_error,
    gh_warning,
    write_github_output,
    write_step_summary,
)
from common.io import read_json
from common.markdown import code_block, summary_section
from common.proc import run_with_retry
from qgc_tools.build_profile import snapshot_summary, write_snapshot

BUILD_ATTEMPTS = 2
BUILD_RETRY_DELAY_SECONDS = 30
# The build's mount points (run_docker.py): CMake caches record paths under them.
CONTAINER_SOURCE = "/project/source"
CONTAINER_BUILD = "/project/build"
CONTAINER_PYTHON = "/opt/qgc-venv/bin/python"


def cmd_run(args: argparse.Namespace) -> int:
    cmd = [sys.executable, "deploy/docker/run_docker.py", "run"]
    if args.fuse:
        cmd.append("--fuse")
    cmd += [args.image, args.build_type]
    try:
        run_with_retry(
            cmd, max_attempts=BUILD_ATTEMPTS, retry_backoff_seconds=BUILD_RETRY_DELAY_SECONDS
        )
    except subprocess.CalledProcessError:
        gh_error(f"Docker build failed after {BUILD_ATTEMPTS} attempts.")
        return 1
    return 0


def exec_command(
    image: str, script: str, args: list[str], *, workspace: Path, mount_build: bool, root: bool
) -> list[str]:
    """Return the docker run command for a workspace-relative *script*.

    The workspace is mounted read-only at /project/source and, with *mount_build*, its build
    directory at /project/build. Arguments naming host paths inside a mount become container paths.
    """
    mounts = [(workspace, CONTAINER_SOURCE)]
    if mount_build:
        mounts.append((workspace / "build", CONTAINER_BUILD))

    def container_path(arg: str) -> str:
        path = Path(arg)
        # <workspace>/build is also inside the workspace; the more specific mount wins.
        for host, container in reversed(mounts):
            if path.is_absolute() and path.is_relative_to(host):
                return str(PurePosixPath(container, path.relative_to(host).as_posix()))
        return arg

    command = ["docker", "run", "--rm"]
    if not root:
        # A mapped UID need not exist in the container's passwd database.
        command += ["--user", f"{os.getuid()}:{os.getgid()}", "--env", "HOME=/tmp"]
    command += ["--entrypoint", CONTAINER_PYTHON]
    for host, container in mounts:
        command += ["-v", f"{host}:{container}:ro"]
    return [*command, image, f"{CONTAINER_SOURCE}/{script}", *map(container_path, args)]


def cmd_exec(args: argparse.Namespace) -> int:
    command = exec_command(
        args.image,
        args.script,
        args.args,
        workspace=args.workspace.absolute(),
        mount_build=args.mount_build,
        root=args.root,
    )
    # stdout belongs to the script: steps redirect it to a file.
    print(shlex.join(command), file=sys.stderr)
    return subprocess.run(command, check=False).returncode


def cmd_cache_key(args: argparse.Namespace) -> int:
    result = subprocess.run(
        [
            "docker",
            "run",
            "--rm",
            "--entrypoint",
            "/bin/bash",
            args.image,
            "-lc",
            "exec /opt/qgc-venv/bin/python /entrypoint.py --cache-key",
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    fingerprint = result.stdout.strip()
    if not re.fullmatch(r"[0-9a-f]{64}", fingerprint):
        raise ValueError("Container returned an invalid compiler fingerprint")
    write_github_output({"fingerprint": fingerprint})
    return 0


# First "Hits: N / M" line of `ccache --show-stats` (4.5+): hits of cacheable calls.
_CCACHE_HITS = re.compile(r"^\s*Hits:\s+(\d+)\s*/\s*(\d+)", re.MULTILINE)
_MOCCACHE_COUNT = re.compile(r"^\s*(hits|misses)\s+(\d+)\s*$", re.MULTILINE)


def cache_hit_rates(report: dict) -> list[tuple[str, object]]:
    """Return compiler and moc cache hit rates from the container's statistics text."""
    facts: list[tuple[str, object]] = []
    if match := _CCACHE_HITS.search(str(report.get("ccache", ""))):
        facts.append(
            ("Compiler cache", f"ccache: {format_ratio(int(match[1]), int(match[2]))} hits")
        )
    counts = {
        kind: int(value) for kind, value in _MOCCACHE_COUNT.findall(str(report.get("moccache", "")))
    }
    if counts:
        hits = counts.get("hits", 0)
        facts.append(("moc cache", f"{format_ratio(hits, hits + counts.get('misses', 0))} hits"))
    return facts


def cmd_summary(args: argparse.Namespace) -> int:
    profile_dir = args.build_dir / "build-profile"
    # Snapshot Ninja timings first so the section can include them.
    if (args.build_dir / ".ninja_log").is_file():
        try:
            print(write_snapshot(args.build_dir, profile_dir))
        except (OSError, ValueError) as error:
            gh_warning(f"Cannot write Ninja build timings: {error}")
    path = args.build_dir / "docker-build-report.json"
    if not path.is_file():
        print("No Docker build performance report was produced")
        return 0
    report = read_json(path)
    write_github_output({"build_success": str(report.get("build_success", False)).lower()})
    timing_facts, timings = snapshot_summary(profile_dir)
    section = summary_section(
        "Docker build",
        [
            ("Result", "passed" if report.get("success") else "failed"),
            *(
                (phase.capitalize(), format_duration(float(seconds)))
                for phase, seconds in report["seconds"].items()
            ),
            ("CPM sources", f"{report['cpm_bytes'] / 1024**2:.1f} MiB"),
            *cache_hit_rates(report),
            *timing_facts,
        ],
        collapsed=[
            *timings,
            *(
                (f"{name} statistics", code_block(str(report.get(name, ""))))
                for name in ("ccache", "moccache")
                if str(report.get(name, "")).strip()
            ),
        ],
    )
    print(section)
    write_step_summary(section)
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    p_run = sub.add_parser("run")
    p_run.add_argument("--image", required=True)
    p_run.add_argument("--build-type", required=True)
    p_run.add_argument(
        "--fuse",
        action="store_true",
        default=env_flag("ENABLE_FUSE"),
        help="(default: $ENABLE_FUSE)",
    )

    p_exec = sub.add_parser("exec", help="Run a repository Python script in a builder image")
    p_exec.add_argument("--image", required=True)
    p_exec.add_argument(
        "--mount-build", action="store_true", help=f"Mount <workspace>/build at {CONTAINER_BUILD}"
    )
    p_exec.add_argument("--root", action="store_true", help="Run as root instead of the caller")
    workspace = os.environ.get("GITHUB_WORKSPACE")
    p_exec.add_argument(
        "--workspace",
        type=Path,
        default=workspace,
        required=not workspace,
        help="Repository checkout to mount (default: $GITHUB_WORKSPACE)",
    )
    p_exec.add_argument("script", help="Script path relative to the workspace")
    p_exec.add_argument("args", nargs=argparse.REMAINDER, help="Script arguments")

    cache_key = sub.add_parser("cache-key", help="Identify the compilers inside a builder image")
    cache_key.add_argument("--image", required=True)
    summary = sub.add_parser("summary", help="Report container timings and cache statistics")
    summary.add_argument("--build-dir", type=Path, required=True)

    args = parser.parse_args(argv)
    return {
        "run": cmd_run,
        "exec": cmd_exec,
        "cache-key": cmd_cache_key,
        "summary": cmd_summary,
    }[args.command](args)


if __name__ == "__main__":
    raise SystemExit(main())
