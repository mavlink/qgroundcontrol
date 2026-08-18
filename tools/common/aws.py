"""Shared policy and subprocess helpers for QGC's public S3 artifacts."""

from __future__ import annotations

from pathlib import Path, PurePosixPath
from typing import TYPE_CHECKING

from .proc import run_captured

if TYPE_CHECKING:
    from collections.abc import Mapping

__all__ = [
    "PUBLIC_BUCKETS",
    "public_s3_uri",
    "s3_object_exists",
    "upload_public_file",
    "validate_public_bucket",
]

PUBLIC_BUCKETS = frozenset({"qgroundcontrol"})


def validate_public_bucket(bucket: str) -> None:
    """Reject uploads outside the explicit QGC public-distribution allowlist."""
    if bucket not in PUBLIC_BUCKETS:
        raise ValueError(f"Bucket {bucket!r} not in allowlist: {sorted(PUBLIC_BUCKETS)}")


def _validate_key(key: str) -> None:
    parts = PurePosixPath(key).parts
    if not key or key.startswith("/") or "\\" in key or ".." in parts:
        raise ValueError(f"Invalid S3 object key: {key!r}")


def public_s3_uri(bucket: str, key: str) -> str:
    """Return an allowlisted, path-safe public S3 URI."""
    validate_public_bucket(bucket)
    _validate_key(key)
    return f"s3://{bucket}/{key}"


def s3_object_exists(bucket: str, key: str) -> bool:
    """Return whether an allowlisted S3 object is visible to the AWS CLI."""
    validate_public_bucket(bucket)
    _validate_key(key)
    result = run_captured(["aws", "s3api", "head-object", "--bucket", bucket, "--key", key])
    return result.returncode == 0


def upload_public_file(
    source: Path,
    bucket: str,
    key: str,
    *,
    env: Mapping[str, str] | None = None,
) -> None:
    """Upload *source* with the public-read ACL after validating bucket and key."""
    if not source.is_file():
        raise ValueError(f"Artifact not found: {source}")
    destination = public_s3_uri(bucket, key)
    result = run_captured(
        ["aws", "s3", "cp", str(source), destination, "--acl", "public-read"],
        env=env,
    )
    if result.returncode != 0:
        detail = result.stderr.strip() or f"aws exited with status {result.returncode}"
        raise RuntimeError(f"S3 upload failed for {destination}: {detail}")
