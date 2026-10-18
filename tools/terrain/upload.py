"""Upload generated tiles to the private qgc-terrain-assets bucket (served by CloudFront).

Production tiles live at the bucket root; test uploads go under a single-segment prefix. Every
target is mutable: a fix overwrites tiles in place, CloudFront is invalidated, and only then is
``tilejson.json`` uploaded with the new data version that clients key their caches on.
"""

from __future__ import annotations

import json
import re
import subprocess
from typing import TYPE_CHECKING

from .jobs import check_passed_marker, done_markers
from .metadata import TILEJSON_NAME, write_metadata
from .tilemath import JOB_ZOOM

if TYPE_CHECKING:
    from pathlib import Path

    from .metadata import MetadataFile, Notice

BUCKET = "qgc-terrain-assets"
DISTRIBUTION_ID = "E2AZ33YAOTUR6"
PUBLIC_URL = "https://terrain.assets.qgroundcontrol.com"
TILE_CACHE_CONTROL = "public, max-age=86400"
METADATA_CACHE_CONTROL = "public, max-age=300"
TILES_DIR = "terrarium"
_PREFIX = re.compile(r"[a-z0-9][a-z0-9._-]*")
_DATA_VERSION = re.compile(r"\d+\.\d+\.\d+")


def validate_prefix(prefix: str | None) -> None:
    """None is production (the bucket root); test prefixes are one lowercase path segment."""
    if prefix is None:
        return
    if not _PREFIX.fullmatch(prefix):
        raise ValueError(f"Invalid prefix {prefix!r}: use a single lowercase path segment")
    if prefix == TILES_DIR:
        raise ValueError(f"Prefix {prefix!r} would nest inside the production tile tree")


def validate_data_version(version: str) -> None:
    if not _DATA_VERSION.fullmatch(version):
        raise ValueError(f"Invalid data version {version!r}: use MAJOR.MINOR.PATCH")


def _version_key(version: str) -> tuple[int, ...]:
    validate_data_version(version)
    return tuple(int(part) for part in version.split("."))


def check_version_increases(new: str, published: str | None) -> None:
    """Clients only refresh their caches when the version changes, so it must always rise."""
    if published is not None and _version_key(new) <= _version_key(published):
        raise ValueError(
            f"--data-version {new} must be higher than the published version {published}"
        )


def check_complete_build(out: Path, expected_jobs: int = 1 << (2 * JOB_ZOOM)) -> None:
    """Production only takes full, checked builds: overviews of a partial build are mostly sea
    level, and only a passing whole-build check covers every tile."""
    markers = done_markers(out)
    if len(markers) != expected_jobs:
        raise ValueError(
            f"Production needs every z{JOB_ZOOM} job: {len(markers)} of {expected_jobs} completed"
        )
    root_tile = out / TILES_DIR / "0" / "0" / "0.png"
    if not root_tile.is_file():
        raise ValueError("Run overviews before a production upload")
    if root_tile.stat().st_mtime < max(marker.stat().st_mtime for marker in markers):
        raise ValueError("Overviews are older than the newest job; rerun overviews")
    passed = check_passed_marker(out)
    if not passed.is_file():
        raise ValueError("Run check before a production upload")
    if passed.stat().st_mtime < root_tile.stat().st_mtime:
        raise ValueError("The build changed after the last passing check; rerun check")


def _key_path(prefix: str | None, name: str) -> str:
    return f"{prefix}/{name}" if prefix else name


def _aws_options(profile: str | None) -> list[str]:
    return ["--profile", profile] if profile else []


def _s3_options(profile: str | None, dryrun: bool) -> list[str]:
    return ["--no-progress", *_aws_options(profile), *(["--dryrun"] if dryrun else [])]


def sync_command(
    out: Path, prefix: str | None, *, mirror: bool, profile: str | None, dryrun: bool
) -> list[str]:
    """Tile sync; deletes remote tiles missing locally only with *mirror*, never in production."""
    validate_prefix(prefix)
    if mirror and prefix is None:
        raise ValueError("--mirror is only for test prefixes: a partial build would delete tiles")
    # The bucket blocks ACLs, so no --acl; CloudFront reads it through Origin Access Control.
    command = [
        "aws",
        "s3",
        "sync",
        str(out / TILES_DIR),
        f"s3://{BUCKET}/{_key_path(prefix, TILES_DIR)}",
        "--exclude",
        "*",
        "--include",
        "*.png",
        "--content-type",
        "image/png",
        "--cache-control",
        TILE_CACHE_CONTROL,
    ]
    if mirror:
        command.append("--delete")
    return command + _s3_options(profile, dryrun)


def invalidation_command(prefix: str | None, *, profile: str | None) -> list[str]:
    validate_prefix(prefix)
    return [
        "aws",
        "cloudfront",
        "create-invalidation",
        "--distribution-id",
        DISTRIBUTION_ID,
        "--paths",
        f"/{_key_path(prefix, TILES_DIR)}/*",
        "--query",
        "Invalidation.Id",
        "--output",
        "text",
        *_aws_options(profile),
    ]


def wait_invalidation_command(invalidation_id: str, *, profile: str | None) -> list[str]:
    return [
        "aws",
        "cloudfront",
        "wait",
        "invalidation-completed",
        "--distribution-id",
        DISTRIBUTION_ID,
        "--id",
        invalidation_id,
        *_aws_options(profile),
    ]


def metadata_commands(
    files: list[MetadataFile], prefix: str | None, *, profile: str | None, dryrun: bool
) -> list[list[str]]:
    """Metadata gets short caching so a new data version reaches clients within minutes."""
    validate_prefix(prefix)
    return [
        [
            "aws",
            "s3",
            "cp",
            str(file.path),
            f"s3://{BUCKET}/{_key_path(prefix, file.path.name)}",
            "--content-type",
            file.content_type,
            "--cache-control",
            METADATA_CACHE_CONTROL,
            *_s3_options(profile, dryrun),
        ]
        for file in files
    ]


def published_version(*, profile: str | None) -> str | None:
    """The live production data version, read from S3 (not the CloudFront cache).

    None when nothing, or no version, has been published yet.
    """
    command = ["aws", "s3", "cp", f"s3://{BUCKET}/{TILEJSON_NAME}", "-", *_aws_options(profile)]
    result = subprocess.run(command, check=False, capture_output=True, text=True)
    if result.returncode:
        if "(404)" in result.stderr:
            return None
        raise RuntimeError(f"Could not read the published {TILEJSON_NAME}: {result.stderr.strip()}")
    return json.loads(result.stdout).get("version")


def _run(command: list[str]) -> int:
    print(" ".join(command), flush=True)
    return subprocess.run(command, check=False).returncode


def _invalidate(prefix: str | None, profile: str | None) -> int:
    command = invalidation_command(prefix, profile=profile)
    print(" ".join(command), flush=True)
    result = subprocess.run(command, check=False, capture_output=True, text=True)
    if result.returncode:
        print(result.stderr, end="", flush=True)
        return result.returncode
    return _run(wait_invalidation_command(result.stdout.strip(), profile=profile))


def upload(
    out: Path,
    prefix: str | None,
    *,
    data_version: str,
    notice: Notice | None,
    mirror: bool,
    profile: str | None,
    dryrun: bool,
) -> int:
    """Tiles, then a completed CloudFront invalidation, then metadata with the new version.

    Publishing the version last means a client that sees it can only fetch fixed tiles.
    Production requires a complete build, a version above the published one, and a *notice*:
    clients discard their cached tiles on a new version and must tell users why.
    """
    if not (out / TILES_DIR).is_dir():
        raise ValueError(f"No tiles found under {out / TILES_DIR}")
    validate_prefix(prefix)
    validate_data_version(data_version)
    if notice is None and prefix is None:
        raise ValueError("Production uploads need a notice telling users what changed")
    if notice is not None:
        notice.validate()
    if prefix is None:
        check_complete_build(out)
        check_version_increases(data_version, published_version(profile=profile))
    files = write_metadata(out, PUBLIC_URL, prefix, data_version, notice)
    returncode = _run(sync_command(out, prefix, mirror=mirror, profile=profile, dryrun=dryrun))
    if returncode:
        return returncode
    if dryrun:
        print(f"(dryrun) would invalidate /{_key_path(prefix, TILES_DIR)}/* and wait", flush=True)
    else:
        returncode = _invalidate(prefix, profile)
        if returncode:
            return returncode
    for command in metadata_commands(files, prefix, profile=profile, dryrun=dryrun):
        returncode = _run(command)
        if returncode:
            return returncode
    return 0
