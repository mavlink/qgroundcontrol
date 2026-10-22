#!/usr/bin/env python3
"""Publish build artifacts to AWS S3 and CloudFront.

``publish`` uploads every ``--artifact PATH:NAME`` to ``builds/<ref>/NAME``. On a
``vX.Y.Z`` tag reachable from a ``Stable*`` branch it also uploads them to
``latest/NAME`` and invalidates those paths with one CloudFront request.
AWS credentials come from the environment, never from the command line.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.aws import upload_public_file
from common.gh_actions import gh_error
from common.proc import run_captured
from qgc_tools.release_tag_check import GitQueryError, check_release_tag


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


def parse_artifact(spec: str) -> tuple[str, str]:
    """Split ``PATH:NAME`` at the last colon so Windows drive letters survive."""
    path, _, name = spec.rpartition(":")
    if not path or not name:
        raise argparse.ArgumentTypeError(f"expected PATH:NAME, got {spec!r}")
    return path, name


def _run_aws(cmd: list[str]) -> None:
    """Run an AWS CLI command, surfacing stderr on failure."""
    try:
        run_captured(cmd, check=True)
    except subprocess.CalledProcessError as e:
        sys.stderr.write(e.stderr or "")
        raise


def _upload(artifact_path: str, bucket: str, key: str) -> None:
    try:
        upload_public_file(Path(artifact_path), bucket, key)
    except (RuntimeError, ValueError) as error:
        gh_error(str(error))
        raise SystemExit(1) from error


def publish(
    artifacts: list[tuple[str, str]],
    *,
    ref_name: str,
    ref_type: str,
    s3_bucket: str,
    distribution_id: str,
) -> None:
    for path, name in artifacts:
        validate_artifact(path, name)

    safe_ref = sanitize_ref(ref_name)
    for path, name in artifacts:
        _upload(path, s3_bucket, f"builds/{safe_ref}/{name}")

    # latest/ is the public stable download: only release tags on a Stable* branch may overwrite it.
    if ref_type != "tag":
        return
    try:
        release_tag = check_release_tag(ref_name, "not publishing to latest/")
    except GitQueryError as exc:
        gh_error(f"Cannot determine whether tag is on a Stable* branch: {exc}")
        raise SystemExit(1) from exc
    if not release_tag:
        return

    for path, name in artifacts:
        _upload(path, s3_bucket, f"latest/{name}")
    if distribution_id:
        _run_aws(
            [
                "aws",
                "cloudfront",
                "create-invalidation",
                "--distribution-id",
                distribution_id,
                "--paths",
                *(f"/latest/{name}" for _, name in artifacts),
            ]
        )


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    p_publish = sub.add_parser(
        "publish", help="Upload artifacts to builds/ and, for releases, latest/"
    )
    p_publish.add_argument(
        "--artifact",
        dest="artifacts",
        action="append",
        required=True,
        type=parse_artifact,
        metavar="PATH:NAME",
    )
    p_publish.add_argument("--ref-name", required=True)
    p_publish.add_argument("--ref-type", required=True)
    p_publish.add_argument("--s3-bucket", default="qgroundcontrol")
    p_publish.add_argument("--distribution-id", default="")

    args = parser.parse_args(argv)
    publish(
        args.artifacts,
        ref_name=args.ref_name,
        ref_type=args.ref_type,
        s3_bucket=args.s3_bucket,
        distribution_id=args.distribution_id,
    )


if __name__ == "__main__":
    main()
