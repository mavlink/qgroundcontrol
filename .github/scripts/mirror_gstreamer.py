#!/usr/bin/env python3
"""Mirror official upstream GStreamer release artifacts to the QGC S3 bucket.

Downloads the prebuilt SDK packages published at gstreamer.freedesktop.org for a
given version, verifies each against its upstream ``.sha256sum`` sidecar, then
uploads them to ``s3://<bucket>/dependencies/gstreamer/<platform>/<filename>`` —
the layout the build's S3 fallback mirror reads (see
``cmake/GStreamer/Download.cmake::gstreamer_get_s3_mirror_url``).

The artifact set per platform mirrors ``gstreamer_get_package_url`` exactly so the
mirror and the primary download can't drift on filename.
"""

from __future__ import annotations

import argparse
import hashlib
import sys
import urllib.request
from dataclasses import dataclass
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import write_step_summary
from common.proc import run_captured

PKG_BASE = "https://gstreamer.freedesktop.org/data/pkg"
ALLOWED_BUCKETS = frozenset({"qgroundcontrol"})
PLATFORMS = ("android", "ios", "macos", "windows")
WINDOWS_MIN_VERSION = (1, 28, 0)


@dataclass(frozen=True)
class Artifact:
    """One mirrored file: its upstream URL and target S3 directory."""

    url: str
    s3_dir: str

    @property
    def filename(self) -> str:
        return self.url.rsplit("/", 1)[-1]

    def s3_key(self) -> str:
        return f"dependencies/gstreamer/{self.s3_dir}/{self.filename}"


def _version_tuple(version: str) -> tuple[int, ...]:
    return tuple(int(part) for part in version.split(".") if part.isdigit())


def artifacts_for(platform: str, version: str) -> list[Artifact]:
    """Return the upstream artifact(s) to mirror for *platform* at *version*."""
    if platform == "android":
        return [Artifact(f"{PKG_BASE}/android/{version}/gstreamer-1.0-android-universal-{version}.tar.xz", "android")]
    if platform == "ios":
        return [Artifact(f"{PKG_BASE}/ios/{version}/gstreamer-1.0-devel-{version}-ios-universal.pkg", "ios")]
    if platform == "macos":
        return [
            Artifact(f"{PKG_BASE}/macos/{version}/gstreamer-1.0-{version}-universal.pkg", "macos"),
            Artifact(f"{PKG_BASE}/macos/{version}/gstreamer-1.0-devel-{version}-universal.pkg", "macos"),
        ]
    if platform == "windows":
        if _version_tuple(version) < WINDOWS_MIN_VERSION:
            raise ValueError(f"Windows GStreamer SDK requires version >= 1.28.0 (got {version!r})")
        return [
            Artifact(f"{PKG_BASE}/windows/{version}/msvc/gstreamer-1.0-msvc-x86_64-{version}.exe", "windows"),
            Artifact(f"{PKG_BASE}/windows/{version}/msvc/gstreamer-1.0-msvc-arm64-{version}.exe", "windows"),
        ]
    raise ValueError(f"Unknown platform: {platform!r}")


def resolve_platforms(value: str) -> list[str]:
    """Expand 'all' or a comma list into validated platform names."""
    if not value or value == "all":
        return list(PLATFORMS)
    requested = [p.strip() for p in value.split(",") if p.strip()]
    unknown = [p for p in requested if p not in PLATFORMS]
    if unknown:
        raise ValueError(f"Unknown platform(s): {', '.join(unknown)}; valid: {', '.join(PLATFORMS)}")
    return requested


def _download(url: str, dest: Path) -> None:
    with urllib.request.urlopen(url, timeout=120) as resp, dest.open("wb") as fh:
        while chunk := resp.read(1 << 20):
            fh.write(chunk)


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        while chunk := fh.read(1 << 20):
            digest.update(chunk)
    return digest.hexdigest()


def _fetch_expected_sha(url: str) -> str:
    with urllib.request.urlopen(f"{url}.sha256sum", timeout=60) as resp:
        return resp.read().decode().split()[0].strip().lower()


def _s3_object_exists(bucket: str, key: str) -> bool:
    result = run_captured(["aws", "s3api", "head-object", "--bucket", bucket, "--key", key])
    return result.returncode == 0


def mirror_artifact(artifact: Artifact, *, bucket: str, work_dir: Path, dry_run: bool, force: bool) -> str:
    """Download, verify, and upload one artifact. Returns a status word for the summary."""
    key = artifact.s3_key()
    if not force and not dry_run and _s3_object_exists(bucket, key):
        print(f"skip (exists): {key}")
        return "skipped"

    local = work_dir / artifact.filename
    print(f"download: {artifact.url}")
    _download(artifact.url, local)

    expected = _fetch_expected_sha(artifact.url)
    actual = _sha256(local)
    if actual != expected:
        raise RuntimeError(f"checksum mismatch for {artifact.filename}: expected {expected}, got {actual}")
    print(f"verified sha256: {actual}")

    if dry_run:
        print(f"dry-run: would upload -> s3://{bucket}/{key}")
        return "dry-run"

    run_captured(
        ["aws", "s3", "cp", str(local), f"s3://{bucket}/{key}", "--acl", "public-read"],
        check=True,
    )
    print(f"uploaded: s3://{bucket}/{key}")
    local.unlink(missing_ok=True)
    return "uploaded"


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True, help="GStreamer version to mirror (e.g. 1.28.3)")
    parser.add_argument("--platforms", default="all", help="'all' or comma list: android,ios,macos,windows")
    parser.add_argument("--bucket", default="qgroundcontrol", help="Target S3 bucket")
    parser.add_argument("--work-dir", default=".", help="Scratch directory for downloads")
    parser.add_argument("--dry-run", action="store_true", help="Download and verify but do not upload")
    parser.add_argument("--force", action="store_true", help="Re-upload even if the object already exists")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    if args.bucket not in ALLOWED_BUCKETS:
        print(f"::error::Bucket {args.bucket!r} not in allowlist: {sorted(ALLOWED_BUCKETS)}", file=sys.stderr)
        return 1
    try:
        platforms = resolve_platforms(args.platforms)
        artifacts = [a for p in platforms for a in artifacts_for(p, args.version)]
    except ValueError as exc:
        print(f"::error::{exc}", file=sys.stderr)
        return 1

    work_dir = Path(args.work_dir)
    work_dir.mkdir(parents=True, exist_ok=True)

    rows: list[str] = []
    for artifact in artifacts:
        try:
            status = mirror_artifact(
                artifact, bucket=args.bucket, work_dir=work_dir, dry_run=args.dry_run, force=args.force
            )
        except (OSError, RuntimeError) as exc:
            print(f"::error::Failed to mirror {artifact.filename}: {exc}", file=sys.stderr)
            return 1
        rows.append(f"| `{artifact.s3_dir}` | `{artifact.filename}` | {status} |")

    write_step_summary(
        f"### GStreamer {args.version} mirror -> `s3://{args.bucket}/dependencies/gstreamer/`\n\n"
        "| Platform | File | Status |\n| --- | --- | --- |\n" + "\n".join(rows) + "\n"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
