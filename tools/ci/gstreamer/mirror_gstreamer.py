#!/usr/bin/env python3
"""Mirror official upstream GStreamer release artifacts to the QGC S3 bucket.

Downloads the prebuilt SDK packages published at gstreamer.freedesktop.org for a
given version, verifies each against its checksum pinned in .github/build-config.json
(else its upstream ``.sha256sum`` sidecar), then uploads them to
``s3://qgroundcontrol/dependencies/gstreamer/<platform>/<filename>`` — the layout the
build's S3 fallback mirror reads. ``common.gstreamer`` holds the package layout
shared with cmake/GStreamer/Download.cmake and the installers.

Options default to `mirror-gstreamer.yml`'s MIRROR_* variables; the version falls back to
build-config.json's ``gstreamer.version.default`` and the work directory to RUNNER_TEMP.
"""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.aws import s3_object_exists, upload_public_file
from common.build_config import get_build_config_value
from common.gh_actions import env_default, env_flag, gh_error, write_step_summary
from common.gstreamer import PLATFORM_PACKAGES, package_platform, package_url, pinned_checksum
from common.markdown import code_span, md_table, summary_section
from common.net import download_file, read_url_text

S3_BUCKET = "qgroundcontrol"
PLATFORMS = tuple(PLATFORM_PACKAGES)
WINDOWS_MIN_VERSION = (1, 28, 0)


@dataclass(frozen=True)
class Artifact:
    """One mirrored file: its build-config package key, upstream URL and target S3 directory."""

    package: str
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
    if platform not in PLATFORM_PACKAGES:
        raise ValueError(f"Unknown platform: {platform!r}")
    if platform == "windows" and _version_tuple(version) < WINDOWS_MIN_VERSION:
        raise ValueError(f"Windows GStreamer SDK requires version >= 1.28.0 (got {version!r})")
    return [
        Artifact(package, package_url(package, version), package_platform(package))
        for package in PLATFORM_PACKAGES[platform]
    ]


def resolve_platforms(value: str) -> list[str]:
    """Expand 'all' or a comma list into validated platform names."""
    if not value or value == "all":
        return list(PLATFORMS)
    requested = [p.strip() for p in value.split(",") if p.strip()]
    unknown = [p for p in requested if p not in PLATFORMS]
    if unknown:
        raise ValueError(
            f"Unknown platform(s): {', '.join(unknown)}; valid: {', '.join(PLATFORMS)}"
        )
    return requested


def _download(url: str, dest: Path, expected_sha256: str) -> None:
    download_file(url, dest, expected_sha256=expected_sha256, timeout=120)


def _expected_sha(artifact: Artifact, version: str) -> str:
    """Return the pinned checksum, which the build treats as authoritative, else the upstream sidecar's."""
    pinned = pinned_checksum(version, artifact.package)
    if pinned:
        return pinned
    return read_url_text(f"{artifact.url}.sha256sum", timeout=60).split()[0].strip().lower()


def mirror_artifact(
    artifact: Artifact, *, version: str, work_dir: Path, dry_run: bool, force: bool
) -> str:
    """Download, verify, and upload one artifact. Returns a status word for the summary."""
    key = artifact.s3_key()
    if not force and not dry_run and s3_object_exists(S3_BUCKET, key):
        print(f"skip (exists): {key}")
        return "skipped"

    local = work_dir / artifact.filename
    print(f"download: {artifact.url}")
    expected = _expected_sha(artifact, version)
    _download(artifact.url, local, expected)
    print(f"verified sha256: {expected}")

    if dry_run:
        print(f"dry-run: would upload -> s3://{S3_BUCKET}/{key}")
        return "dry-run"

    upload_public_file(local, S3_BUCKET, key)
    print(f"uploaded: s3://{S3_BUCKET}/{key}")
    local.unlink(missing_ok=True)
    return "uploaded"


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--version",
        default=env_default("MIRROR_VERSION")
        or get_build_config_value("gstreamer.version.default"),
        help="GStreamer version to mirror, e.g. 1.28.3 "
        "(default: $MIRROR_VERSION, else build-config.json's gstreamer.version.default)",
    )
    parser.add_argument(
        "--platforms",
        default=env_default("MIRROR_PLATFORMS", "all"),
        help="'all' or comma list: android,ios,macos,windows (default: $MIRROR_PLATFORMS, else all)",
    )
    parser.add_argument(
        "--work-dir",
        default=env_default("RUNNER_TEMP", "."),
        help="Scratch directory for downloads (default: $RUNNER_TEMP, else .)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=env_flag("MIRROR_DRY_RUN"),
        help="Download and verify but do not upload (default: $MIRROR_DRY_RUN)",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        default=env_flag("MIRROR_FORCE"),
        help="Re-upload even if the object already exists (default: $MIRROR_FORCE)",
    )
    args = parser.parse_args(argv)
    if not args.version:
        parser.error("--version is required: build-config.json has no gstreamer.version.default")
    return args


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        platforms = resolve_platforms(args.platforms)
        artifacts = [a for p in platforms for a in artifacts_for(p, args.version)]
    except ValueError as exc:
        gh_error(str(exc))
        return 1

    work_dir = Path(args.work_dir)
    work_dir.mkdir(parents=True, exist_ok=True)

    rows: list[list[str]] = []
    for artifact in artifacts:
        try:
            status = mirror_artifact(
                artifact,
                version=args.version,
                work_dir=work_dir,
                dry_run=args.dry_run,
                force=args.force,
            )
        except (OSError, RuntimeError) as exc:
            gh_error(f"Failed to mirror {artifact.filename}: {exc}")
            return 1
        rows.append([code_span(artifact.s3_dir), code_span(artifact.filename), status])

    write_step_summary(
        summary_section(
            "GStreamer mirror",
            [
                ("Version", args.version),
                ("Destination", code_span(f"s3://{S3_BUCKET}/dependencies/gstreamer/")),
                ("Mode", "dry run" if args.dry_run else "upload"),
                ("Files", len(rows)),
            ],
            body=md_table(["Platform", "File", "Status"], rows, markdown=True),
        )
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
