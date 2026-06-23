#!/usr/bin/env python3
"""Deploy built documentation to an external GitHub Pages repository."""

from __future__ import annotations

import argparse
import datetime
import re
import shutil
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.git import run_git


def sanitize_branch(name: str) -> str:
    """Remove unsafe characters from branch name for use as a directory."""
    return re.sub(r"[^a-zA-Z0-9._-]", "_", name)


def _copy_tree(source: Path, deploy_dir: Path) -> None:
    if deploy_dir.exists():
        shutil.rmtree(deploy_dir)
    deploy_dir.mkdir(parents=True)
    for item in source.iterdir():
        dest = deploy_dir / item.name
        if item.is_dir():
            shutil.copytree(item, dest)
        else:
            shutil.copy2(item, dest)


def deploy_branch(
    source_dir: Path,
    target_dir: Path,
    branch: str,
    target_branch: str,
    commit_message: str,
    author_email: str,
    author_name: str,
) -> bool:
    """Copy *source_dir* into *target_dir*/<safe(branch)>, commit and push.

    Returns True when a commit+push was made, False when there was nothing to deploy.
    """
    safe_branch = sanitize_branch(branch)
    deploy_dir = target_dir / safe_branch
    _copy_tree(source_dir, deploy_dir)

    run_git("config", "user.email", author_email, cwd=target_dir, check=True)
    run_git("config", "user.name", author_name, cwd=target_dir, check=True)
    run_git("add", safe_branch, cwd=target_dir, check=True)

    diff = run_git("diff", "--cached", "--quiet", cwd=target_dir)
    if diff.returncode == 0:
        print("No documentation changes to deploy.")
        return False

    today = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d")
    run_git("commit", "-m", f"{commit_message} {today}", cwd=target_dir, check=True)
    run_git("push", "origin", target_branch, cwd=target_dir, check=True)
    return True


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", required=True, help="Path to built docs")
    parser.add_argument("--target-dir", required=True, help="Path to target repo checkout")
    parser.add_argument("--branch", required=True, help="Source branch name (used as subdirectory)")
    parser.add_argument("--target-branch", default="main", help="Branch to push to")
    parser.add_argument("--commit-message", default="Docs update")
    parser.add_argument("--author-email", default="github-actions[bot]@users.noreply.github.com")
    parser.add_argument("--author-name", default="github-actions[bot]")
    args = parser.parse_args()

    deploy_branch(
        source_dir=Path(args.source_dir),
        target_dir=Path(args.target_dir),
        branch=args.branch,
        target_branch=args.target_branch,
        commit_message=args.commit_message,
        author_email=args.author_email,
        author_name=args.author_name,
    )


if __name__ == "__main__":
    main()
