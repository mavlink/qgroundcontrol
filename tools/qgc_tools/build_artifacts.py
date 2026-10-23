"""QGC's build artifact naming: what reports count as distributable, and what S3 publishes.

``PUBLISHED_BUILDS`` lists the packages the platform workflows' ``attest-and-upload`` steps
publish to S3 ``builds/<ref>/`` (each with a ``<name>.sha256`` sidecar), served through CloudFront.
``tools/tests/ci/contracts/test_published_builds.py`` keeps it in sync with the workflows and the
user guide's download links.
"""

from __future__ import annotations

from dataclasses import dataclass

__all__ = [
    "BUILDS_URL",
    "PUBLISHED_BUILDS",
    "PublishedBuild",
    "is_distributable_artifact",
    "published_build_url",
]

_DISTRIBUTABLE_PREFIXES = ("QGroundControl", "Custom-QGroundControl", "qgc-binary")
_EXCLUDED_PREFIXES = ("test-results", "emulator-diagnostics")
_EXCLUDED_NAMES = frozenset({"coverage-report"})

# CloudFront distribution in front of the public `qgroundcontrol` S3 bucket.
BUILDS_URL = "https://d176tv9ibo4jno.cloudfront.net/builds"


@dataclass(frozen=True)
class PublishedBuild:
    name: str
    platform: str
    description: str


PUBLISHED_BUILDS: tuple[PublishedBuild, ...] = (
    PublishedBuild("QGroundControl-installer-AMD64.exe", "Windows", "x86_64 installer"),
    PublishedBuild("QGroundControl-installer-ARM64.exe", "Windows", "Arm64 installer"),
    PublishedBuild("QGroundControl.dmg", "macOS", "Universal disk image"),
    PublishedBuild("QGroundControl-x86_64.AppImage", "Linux", "x86_64 AppImage"),
    PublishedBuild("QGroundControl-aarch64.AppImage", "Linux", "aarch64 AppImage"),
    PublishedBuild("QGroundControl.apk", "Android", "APK (arm64-v8a, armeabi-v7a)"),
)


def is_distributable_artifact(name: str) -> bool:
    """Whether a GitHub artifact is an installable package rather than results or diagnostics."""
    if name in _EXCLUDED_NAMES or name.startswith(_EXCLUDED_PREFIXES):
        return False
    return name.startswith(_DISTRIBUTABLE_PREFIXES)


def published_build_url(ref: str, name: str) -> str:
    """Return the public URL of a package published to ``builds/<ref>/``."""
    return f"{BUILDS_URL}/{ref}/{name}"
