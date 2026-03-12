#!/usr/bin/env python3
"""Validate and upload artifacts to AWS S3.

Subcommands:
    validate    Validate AWS credentials and artifact inputs
    upload      Upload artifact to S3 with path safety checks
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


def validate_credentials(role_arn: str, key_id: str, secret_key: str) -> None:
    """Ensure either OIDC role or static credentials are provided."""
    if not role_arn and (not key_id or not secret_key):
        print(
            "::error::Either aws_role_arn (OIDC) or both aws_key_id and "
            "aws_secret_access_key (static credentials) must be provided",
            file=sys.stderr,
        )
        sys.exit(1)


def validate_artifact(artifact_path: str, artifact_name: str) -> None:
    """Validate artifact exists and name is safe."""
    if not Path(artifact_path).is_file():
        print(f"::error::Artifact not found: {artifact_path}", file=sys.stderr)
        sys.exit(1)

    if re.search(r"[/\\]", artifact_name) or ".." in artifact_name:
        print(
            f"::error::Invalid artifact name (contains path separators or ..): {artifact_name}",
            file=sys.stderr,
        )
        sys.exit(1)


def sanitize_ref(ref_name: str) -> str:
    """Remove path traversal and unsafe characters from a git ref name."""
    safe = ref_name.replace("..", "")
    return re.sub(r"[^a-zA-Z0-9._-]", "_", safe)


def cmd_validate(args: argparse.Namespace) -> None:
    validate_credentials(args.role_arn, args.key_id, args.secret_key)
    validate_artifact(args.artifact_path, args.artifact_name)


def cmd_upload(args: argparse.Namespace) -> None:
    validate_artifact(args.artifact_path, args.artifact_name)
    safe_ref = sanitize_ref(args.ref_name)

    s3_path = f"s3://{args.s3_bucket}/builds/{safe_ref}/{args.artifact_name}"
    subprocess.run(
        ["aws", "s3", "cp", args.artifact_path, s3_path, "--acl", "public-read"],
        check=True,
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

    args = parser.parse_args()
    {"validate": cmd_validate, "upload": cmd_upload}[args.command](args)


if __name__ == "__main__":
    main()
