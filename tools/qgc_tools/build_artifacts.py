"""QGC's build artifact naming: what reports count as distributable, and what is published.

``PUBLISHED_BUILDS`` lists the packages the platform workflows mark for publishing
(``attest-and-upload``'s ``publish`` input), which the Publish Builds workflow uploads to S3
``builds/<ref>/`` (each with a ``<name>.sha256`` sidecar), served through CloudFront.
``tools/tests/ci/contracts/test_published_builds.py`` keeps it in sync with the workflows and the
user guide's download links.

The list is read on both sides of a publication. A build records, in each package's publish
marker, the packages its workflow marks in the commit that is built (``workflow_packages``),
and its set is complete when it holds exactly those. The publisher always runs from the default
branch, whatever branch was built, and publishes only names that branch knows
(``is_publishable_name``), so a name is added here first and stays here, as a retired name,
for as long as a supported Stable branch still builds it.
"""

from __future__ import annotations

import re
from dataclasses import dataclass

__all__ = [
    "ARTIFACT_RETENTION_DAYS",
    "BUILDS_URL",
    "BUILD_METADATA_SCHEMA_VERSION",
    "BUILD_METADATA_SUFFIX",
    "PUBLISHED_BUILDS",
    "RETIRED_BUILD_NAMES",
    "PublishedBuild",
    "companion_names",
    "is_distributable_artifact",
    "is_public_name",
    "is_publishable_name",
    "published_build_url",
    "workflow_packages",
]

_DISTRIBUTABLE_PREFIXES = ("QGroundControl", "Custom-QGroundControl", "qgc-binary")
_EXCLUDED_PREFIXES = ("test-results", "emulator-diagnostics")
_EXCLUDED_NAMES = frozenset({"coverage-report"})

# CloudFront distribution in front of the public `qgroundcontrol` S3 bucket.
BUILDS_URL = "https://d176tv9ibo4jno.cloudfront.net/builds"

# The build metadata attest-and-upload writes beside a package: `<package file>.build.json`.
# Version 2 added `event` and, for a package to publish, the `publish` marker: the package's
# name, its workflow, and every package name that workflow marks.
BUILD_METADATA_SUFFIX = ".build.json"
BUILD_METADATA_SCHEMA_VERSION = 2

# How long attest-and-upload keeps the artifact of a build that is not a pull request's. GitHub
# then deletes it, and the run no longer lists it.
ARTIFACT_RETENTION_DAYS = 30

# A published file's name becomes an S3 key and a release asset name.
_PUBLIC_NAME = re.compile(r"QGroundControl(?:-[A-Za-z0-9_]+)*\.(?:AppImage|apk|dmg|exe)")


@dataclass(frozen=True)
class PublishedBuild:
    name: str
    platform: str
    description: str
    # The name of the workflow that builds and marks the package.
    workflow: str


PUBLISHED_BUILDS: tuple[PublishedBuild, ...] = (
    PublishedBuild("QGroundControl-installer-AMD64.exe", "Windows", "x86_64 installer", "Windows"),
    PublishedBuild("QGroundControl-installer-ARM64.exe", "Windows", "Arm64 installer", "Windows"),
    PublishedBuild("QGroundControl.dmg", "macOS", "Universal disk image", "MacOS"),
    PublishedBuild("QGroundControl-x86_64.AppImage", "Linux", "x86_64 AppImage", "Linux"),
    PublishedBuild("QGroundControl-aarch64.AppImage", "Linux", "aarch64 AppImage", "Linux"),
    PublishedBuild("QGroundControl.apk", "Android", "APK (arm64-v8a, armeabi-v7a)", "Android"),
)


def is_distributable_artifact(name: str) -> bool:
    """Whether a GitHub artifact is an installable package rather than results or diagnostics."""
    if name in _EXCLUDED_NAMES or name.startswith(_EXCLUDED_PREFIXES):
        return False
    return name.startswith(_DISTRIBUTABLE_PREFIXES)


# Names the workflows on the default branch no longer mark, but a supported Stable branch does.
RETIRED_BUILD_NAMES: frozenset[str] = frozenset()


def is_public_name(name: str) -> bool:
    """Whether *name* has the form of a package file name, which is safe in a key and a URL."""
    return _PUBLIC_NAME.fullmatch(name) is not None


def is_publishable_name(name: str) -> bool:
    """Whether the Publish Builds workflow may publish a file called *name*.

    Only a name this checkout lists, as published now or as retired: the publisher runs from the
    default branch, so a build of any branch publishes nothing under a name unknown there.
    """
    known = name in RETIRED_BUILD_NAMES or any(build.name == name for build in PUBLISHED_BUILDS)
    return known and is_public_name(name)


def companion_names(package: str) -> tuple[str, ...]:
    """Return the names of the companion files that the package called *package* may have.

    A companion is published beside its package on a GitHub Release and nowhere else. It is
    not a package: no workflow's list of packages names it. The only one is the ``.zsync`` of
    an AppImage that embeds update information, which an updater downloads from the release
    that the information names.
    """
    if is_public_name(package) and package.endswith(".AppImage"):
        return (f"{package}.zsync",)
    return ()


def workflow_packages(workflow: str) -> list[str]:
    """Return the names of the packages that the workflow called *workflow* marks, sorted."""
    return sorted(build.name for build in PUBLISHED_BUILDS if build.workflow == workflow)


def published_build_url(ref: str, name: str) -> str:
    """Return the public URL of a package published to ``builds/<ref>/``."""
    return f"{BUILDS_URL}/{ref}/{name}"
