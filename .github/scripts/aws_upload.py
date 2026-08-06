#!/usr/bin/env python3
"""Validate and upload artifacts to AWS S3.

Subcommands:
    validate       Validate AWS credentials and artifact inputs
    upload         Upload artifact to S3 builds/<ref>/ with path safety checks
    upload-latest  Upload artifact to S3 latest/ (release tags only)
    invalidate     Invalidate the latest/<artifact> path in CloudFront
    auth-mode      Resolve which AWS auth configure step should run
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh_error, write_github_output
from common.proc import run_captured


def validate_credentials(role_arn: str, key_id: str, secret_key: str) -> None:
    """Ensure either OIDC role or static credentials are provided."""
    if not role_arn and (not key_id or not secret_key):
        gh_error(
            "Either aws_role_arn (OIDC) or both aws_key_id and "
            "aws_secret_access_key (static credentials) must be provided"
        )
        sys.exit(1)


def validate_artifact(artifact_path: str, artifact_name: str) -> None:
    """Validate artifact exists and name is safe."""
    if not Path(artifact_path).is_file():
        gh_error(f"Artifact not found: {artifact_path}")
        sys.exit(1)

    if re.search(r"[/\\]", artifact_name) or ".." in artifact_name:
        gh_error(f"Invalid artifact name (contains path separators or ..): {artifact_name}")
        sys.exit(1)


def sanitize_ref(ref_name: str) -> str:
    """Remove path traversal and unsafe characters from a git ref name."""
    safe = ref_name.replace("..", "")
    return re.sub(r"[^a-zA-Z0-9._-]", "_", safe)


def cmd_validate(args: argparse.Namespace) -> None:
    validate_credentials(args.role_arn, args.key_id, args.secret_key)
    validate_artifact(args.artifact_path, args.artifact_name)


def resolve_auth_mode(role_arn: str, key_id: str) -> str:
    """Return 'oidc' when a role ARN is set, 'static' when a key ID is set, else 'none'.

    Mirrors the conditional ordering in aws-upload/action.yml so the two paths
    can't disagree on which configure-credentials step should run.
    """
    if role_arn:
        return "oidc"
    if key_id:
        return "static"
    return "none"


def cmd_auth_mode(args: argparse.Namespace) -> None:
    mode = resolve_auth_mode(args.role_arn, args.key_id)
    print(mode)
    write_github_output({"mode": mode})


def _run_aws(cmd: list[str]) -> None:
    """Run an AWS CLI command, surfacing stderr on failure."""
    try:
        run_captured(cmd, check=True)
    except subprocess.CalledProcessError as e:
        sys.stderr.write(e.stderr or "")
        raise


def cmd_upload(args: argparse.Namespace) -> None:
    validate_artifact(args.artifact_path, args.artifact_name)
    safe_ref = sanitize_ref(args.ref_name)

    s3_path = f"s3://{args.s3_bucket}/builds/{safe_ref}/{args.artifact_name}"
    _run_aws(["aws", "s3", "cp", args.artifact_path, s3_path, "--acl", "public-read"])


def cmd_upload_latest(args: argparse.Namespace) -> None:
    validate_artifact(args.artifact_path, args.artifact_name)
    s3_path = f"s3://{args.s3_bucket}/latest/{args.artifact_name}"
    _run_aws(["aws", "s3", "cp", args.artifact_path, s3_path, "--acl", "public-read"])


def cmd_invalidate(args: argparse.Namespace) -> None:
    if re.search(r"[/\\]", args.artifact_name) or ".." in args.artifact_name:
        gh_error(f"Invalid artifact name (contains path separators or ..): {args.artifact_name}")
        sys.exit(1)
    _run_aws(
        [
            "aws",
            "cloudfront",
            "create-invalidation",
            "--distribution-id",
            args.distribution_id,
            "--paths",
            f"/latest/{args.artifact_name}",
        ]
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    p_val = sub.add_parser("validate")
    p_val.add_argument("--role-arn", default="")
    p_val.add_argument("--key-id", default="")
    p_val.add_argument("--secret-key", default="")
    p_val.add_argument("--artifact-path", required=True)
    p_val.add_argument("--artifact-name", required=True)

    p_up = sub.add_parser("upload")
    p_up.add_argument("--artifact-path", required=True)
    p_up.add_argument("--artifact-name", required=True)
    p_up.add_argument("--ref-name", required=True)
    p_up.add_argument("--s3-bucket", default="qgroundcontrol")

    p_latest = sub.add_parser("upload-latest")
    p_latest.add_argument("--artifact-path", required=True)
    p_latest.add_argument("--artifact-name", required=True)
    p_latest.add_argument("--s3-bucket", default="qgroundcontrol")

    p_inv = sub.add_parser("invalidate")
    p_inv.add_argument("--artifact-name", required=True)
    p_inv.add_argument("--distribution-id", required=True)

    p_auth = sub.add_parser("auth-mode", help="Resolve oidc|static|none from secret presence")
    p_auth.add_argument("--role-arn", default="")
    p_auth.add_argument("--key-id", default="")

    args = parser.parse_args()
    {
        "validate": cmd_validate,
        "upload": cmd_upload,
        "upload-latest": cmd_upload_latest,
        "invalidate": cmd_invalidate,
        "auth-mode": cmd_auth_mode,
    }[args.command](args)


if __name__ == "__main__":
    main()
