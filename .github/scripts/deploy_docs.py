#!/usr/bin/env python3
"""Deploy built documentation to an external GitHub Pages repository."""

from __future__ import annotations

import argparse
import datetime
import re
import shutil
import subprocess
import sys
from pathlib import Path


def sanitize_branch(name: str) -> str:
    """Remove unsafe characters from branch name for use as a directory."""
    return re.sub(r"[^a-zA-Z0-9._-]", "_", name)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", required=True, help="Path to built docs")
    parser.add_argument("--target-dir", required=True, help="Path to target repo checkout")
    parser.add_argument("--branch", required=True, help="Source branch name (used as subdirectory)")
    parser.add_argument("--target-branch", default="main", help="Branch to push to")
    parser.add_argument("--commit-message", default="Docs update")
    parser.add_argument("--author-email", default="bot@px4.io")
    parser.add_argument("--author-name", default="PX4BuildBot")
    args = parser.parse_args()

    safe_branch = sanitize_branch(args.branch)
    target = Path(args.target_dir)
    deploy_dir = target / safe_branch

    if deploy_dir.exists():
        shutil.rmtree(deploy_dir)
    deploy_dir.mkdir(parents=True)

    source = Path(args.source_dir)
    for item in source.iterdir():
        dest = deploy_dir / item.name
        if item.is_dir():
            shutil.copytree(item, dest)
        else:
            shutil.copy2(item, dest)

    def git(*cmd: str) -> subprocess.CompletedProcess:
        return subprocess.run(["git", *cmd], cwd=str(target), check=True)

    git("config", "user.email", args.author_email)
    git("config", "user.name", args.author_name)
    git("add", safe_branch)

    diff = subprocess.run(
        ["git", "diff", "--cached", "--quiet"],
        cwd=str(target), check=False,
    )
    if diff.returncode == 0:
        print("No documentation changes to deploy.")
        return

    today = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d")
    git("commit", "-m", f"{args.commit_message} {today}")
    git("push", "origin", args.target_branch)


if __name__ == "__main__":
    main()
