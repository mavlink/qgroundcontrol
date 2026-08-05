#!/usr/bin/env python3
"""Docker build helpers for CI.

Subcommands:
    validate              Validate target and build-type inputs
    run                   Run docker build with retry logic
    resolve-push-target   Resolve the GHCR/Docker Hub push target for the current ref
"""

from __future__ import annotations

import argparse
import subprocess
import sys
import time

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh_error, write_github_output

VALID_TARGETS = {"linux", "linux-cross", "android"}
VALID_BUILD_TYPES = {"Release", "Debug"}


def cmd_validate(args: argparse.Namespace) -> None:
    ok = True
    if args.target not in VALID_TARGETS:
        gh_error(f"Invalid target '{args.target}'")
        print(f"Allowed values: {', '.join(sorted(VALID_TARGETS))}", file=sys.stderr)
        ok = False
    if args.build_type not in VALID_BUILD_TYPES:
        gh_error(f"Invalid build-type '{args.build_type}'")
        print(f"Allowed values: {', '.join(sorted(VALID_BUILD_TYPES))}", file=sys.stderr)
        ok = False
    if not ok:
        sys.exit(1)
    print(f"Using target: {args.target}")
    print(f"Build type: {args.build_type}")


def resolve_push_target(event_name: str, repo: str, ref: str) -> str:
    """Registry to push the builder image to, or '' for no push.

    Pushes happen only from the upstream repo on a push event: GHCR for
    master/Stable branches, Docker Hub (dronecode/qgroundcontrol) for v* release
    tags. Forks, PRs, and any other repo always resolve to '' (no push).
    """
    if event_name != "push" or repo != "mavlink/qgroundcontrol":
        return ""
    if ref == "refs/heads/master" or ref.startswith("refs/heads/Stable"):
        return "ghcr.io/mavlink/qgroundcontrol"
    if ref.startswith("refs/tags/v"):
        return "dronecode/qgroundcontrol"
    return ""


def cmd_resolve_push_target(args: argparse.Namespace) -> None:
    target = resolve_push_target(args.event_name, args.repo, args.ref)
    write_github_output({"ref": target})
    print(f"push target: {target or '(none)'}")


def cmd_run(args: argparse.Namespace) -> None:
    cmd = ["./deploy/docker/_docker-exec.sh"]
    if args.fuse:
        cmd.append("--fuse")
    cmd += [args.image, args.build_type]

    for attempt in range(1, args.max_attempts + 1):
        result = subprocess.run(cmd, check=False)
        if result.returncode == 0:
            return
        if attempt >= args.max_attempts:
            gh_error(f"Docker build failed after {attempt} attempt(s).")
            sys.exit(1)
        print(
            f"Docker build failed on attempt {attempt}. Retrying in {args.retry_delay} seconds..."
        )
        time.sleep(args.retry_delay)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    p_val = sub.add_parser("validate")
    p_val.add_argument("--target", required=True)
    p_val.add_argument("--build-type", required=True)

    p_run = sub.add_parser("run")
    p_run.add_argument("--image", default="dronecode/qgroundcontrol:linux")
    p_run.add_argument("--build-type", required=True)
    p_run.add_argument("--fuse", action="store_true", default=False)
    p_run.add_argument("--max-attempts", type=int, default=2)
    p_run.add_argument("--retry-delay", type=int, default=30)

    p_target = sub.add_parser("resolve-push-target")
    p_target.add_argument("--event-name", required=True)
    p_target.add_argument("--repo", required=True)
    p_target.add_argument("--ref", required=True)

    args = parser.parse_args()
    {
        "validate": cmd_validate,
        "run": cmd_run,
        "resolve-push-target": cmd_resolve_push_target,
    }[args.command](args)


if __name__ == "__main__":
    main()
