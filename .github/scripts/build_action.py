#!/usr/bin/env python3
"""
Build a GitHub Action from source when dist hasn't been updated.

Useful for getting fixes from unreleased commits when an action's
maintainer hasn't rebuilt and pushed the dist/ folder.

Usage:
    build_action.py owner/repo                    # Build from default branch
    build_action.py owner/repo --ref fix-branch   # Build specific ref
    build_action.py owner/repo --ref abc123       # Build specific commit
    build_action.py owner/repo --output ./my-action

Outputs (for GitHub Actions):
    action_path - Path to built action directory
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


class ActionBuilder:
    """Builds GitHub Actions from source."""

    DEFAULT_NODE_VERSION = "22"

    def __init__(
        self,
        repo: str,
        ref: str = "HEAD",
        output_dir: Path | None = None,
        node_version: str = DEFAULT_NODE_VERSION,
    ) -> None:
        self.repo = repo
        self.ref = ref
        self.output_dir = output_dir
        self.node_version = node_version

    @property
    def repo_url(self) -> str:
        """Get the clone URL for the repository."""
        return f"https://github.com/{self.repo}.git"

    @property
    def repo_name(self) -> str:
        """Get repository name without owner."""
        return self.repo.split("/")[-1]

    def check_prerequisites(self) -> bool:
        """Verify required tools are available."""
        missing = []

        if not shutil.which("git"):
            missing.append("git")

        if not shutil.which("node"):
            missing.append("node")

        if not shutil.which("npm"):
            missing.append("npm")

        if missing:
            print(f"Error: Missing required tools: {', '.join(missing)}", file=sys.stderr)
            return False

        return True

    def clone_repo(self, dest: Path) -> bool:
        """Clone repository to destination."""
        print(f"Cloning {self.repo}...")

        result = subprocess.run(
            ["git", "clone", "--depth", "1", "--no-single-branch", self.repo_url, str(dest)],
            capture_output=True,
            text=True,
        )

        if result.returncode != 0:
            print(f"Error cloning repository: {result.stderr}", file=sys.stderr)
            return False

        return True

    def checkout_ref(self, repo_dir: Path) -> bool:
        """Checkout specific ref."""
        if self.ref == "HEAD":
            return True

        print(f"Checking out {self.ref}...")

        # Fetch the specific ref if it's a commit SHA
        if len(self.ref) >= 7 and all(c in "0123456789abcdef" for c in self.ref.lower()):
            result = subprocess.run(
                ["git", "fetch", "--depth", "1", "origin", self.ref],
                cwd=repo_dir,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                print(f"Warning: Could not fetch {self.ref}, trying checkout anyway")

        result = subprocess.run(
            ["git", "checkout", self.ref],
            cwd=repo_dir,
            capture_output=True,
            text=True,
        )

        if result.returncode != 0:
            print(f"Error checking out {self.ref}: {result.stderr}", file=sys.stderr)
            return False

        return True

    def install_dependencies(self, repo_dir: Path) -> bool:
        """Install npm dependencies."""
        print("Installing dependencies...")

        package_lock = repo_dir / "package-lock.json"
        cmd = ["npm", "ci"] if package_lock.exists() else ["npm", "install"]

        result = subprocess.run(
            cmd,
            cwd=repo_dir,
            capture_output=True,
            text=True,
        )

        if result.returncode != 0:
            # Try npm install as fallback
            if cmd[1] == "ci":
                print("npm ci failed, trying npm install...")
                result = subprocess.run(
                    ["npm", "install"],
                    cwd=repo_dir,
                    capture_output=True,
                    text=True,
                )

        if result.returncode != 0:
            print(f"Error installing dependencies: {result.stderr}", file=sys.stderr)
            return False

        return True

    def build_action(self, repo_dir: Path) -> bool:
        """Run npm build."""
        print("Building action...")

        # Check if build script exists
        package_json = repo_dir / "package.json"
        if not package_json.exists():
            print("Error: package.json not found", file=sys.stderr)
            return False

        result = subprocess.run(
            ["npm", "run", "build"],
            cwd=repo_dir,
            capture_output=True,
            text=True,
        )

        if result.returncode != 0:
            print(f"Error building action: {result.stderr}", file=sys.stderr)
            # Show stdout too as some errors go there
            if result.stdout:
                print(f"Build output: {result.stdout}", file=sys.stderr)
            return False

        return True

    def verify_build(self, repo_dir: Path) -> bool:
        """Verify the action was built successfully."""
        action_yml = repo_dir / "action.yml"
        action_yaml = repo_dir / "action.yaml"

        if not action_yml.exists() and not action_yaml.exists():
            print("Error: No action.yml or action.yaml found", file=sys.stderr)
            return False

        # Check for dist directory (most JS actions output here)
        dist_dir = repo_dir / "dist"
        if dist_dir.exists():
            js_files = list(dist_dir.glob("*.js"))
            if js_files:
                print(f"Build successful: {len(js_files)} JS files in dist/")
                return True

        # Some actions use lib/ instead
        lib_dir = repo_dir / "lib"
        if lib_dir.exists():
            js_files = list(lib_dir.glob("**/*.js"))
            if js_files:
                print(f"Build successful: {len(js_files)} JS files in lib/")
                return True

        print("Warning: Could not verify build output, continuing anyway")
        return True

    def copy_to_output(self, repo_dir: Path) -> Path:
        """Copy built action to output directory."""
        if self.output_dir:
            output = self.output_dir
        else:
            output = Path.cwd() / f"built-action-{self.repo_name}"

        if output.exists():
            shutil.rmtree(output)

        shutil.copytree(repo_dir, output)
        print(f"Action copied to: {output}")

        return output

    def build(self) -> Path | None:
        """Execute full build workflow."""
        if not self.check_prerequisites():
            return None

        with tempfile.TemporaryDirectory() as temp_dir:
            repo_dir = Path(temp_dir) / self.repo_name

            if not self.clone_repo(repo_dir):
                return None

            if not self.checkout_ref(repo_dir):
                return None

            if not self.install_dependencies(repo_dir):
                return None

            if not self.build_action(repo_dir):
                return None

            if not self.verify_build(repo_dir):
                return None

            return self.copy_to_output(repo_dir)


def output_github_actions(action_path: Path) -> None:
    """Write outputs for GitHub Actions."""
    github_output = os.environ.get("GITHUB_OUTPUT")
    if not github_output:
        return

    try:
        with open(github_output, "a") as f:
            f.write(f"action_path={action_path}\n")
    except OSError as e:
        print(f"Warning: Failed to write GitHub output: {e}", file=sys.stderr)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Build a GitHub Action from source",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s actions/checkout                      # Build latest
  %(prog)s actions/checkout --ref v4.2.0         # Build specific tag
  %(prog)s owner/action --ref feature-branch     # Build branch
  %(prog)s owner/action --ref abc1234            # Build commit SHA
  %(prog)s owner/action --output ./local-action  # Custom output dir
""",
    )

    parser.add_argument(
        "repo",
        help="Repository in owner/repo format (e.g., actions/checkout)",
    )
    parser.add_argument(
        "--ref",
        default="HEAD",
        help="Branch, tag, or commit SHA to build (default: HEAD)",
    )
    parser.add_argument(
        "--output",
        "-o",
        type=Path,
        dest="output_dir",
        help="Output directory for built action",
    )
    parser.add_argument(
        "--node-version",
        default=ActionBuilder.DEFAULT_NODE_VERSION,
        help=f"Node.js version requirement (default: {ActionBuilder.DEFAULT_NODE_VERSION})",
    )

    return parser.parse_args(argv)


def main() -> int:
    """Main entry point."""
    args = parse_args()

    # Validate repo format
    if "/" not in args.repo or args.repo.count("/") != 1:
        print(f"Error: Invalid repo format '{args.repo}', expected 'owner/repo'", file=sys.stderr)
        return 1

    builder = ActionBuilder(
        repo=args.repo,
        ref=args.ref,
        output_dir=args.output_dir,
        node_version=args.node_version,
    )

    print(f"Building action: {args.repo}")
    if args.ref != "HEAD":
        print(f"  Ref: {args.ref}")

    action_path = builder.build()

    if action_path is None:
        print("Build failed", file=sys.stderr)
        return 1

    output_github_actions(action_path)

    print(f"\nSuccess! Use the action with:")
    print(f"  uses: {action_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
