"""The GStreamer SDK packages, for the S3 mirror, the pin command and dependency checks.

``gstreamer.packages`` in .github/build-config.json holds the path of each package under
``gstreamer.package_url``, with ``{version}`` for the release; cmake/GStreamer/Packages.cmake reads
the same table. A package's key is also its ``gstreamer.checksums.<version>`` entry, and the first
directory of its path is its platform: the ``gstreamer.version`` entry that names its release and
its directory on the mirror.
"""

from __future__ import annotations

import re
from typing import TYPE_CHECKING, Any

from .build_config import load_build_config
from .net import read_url_text

if TYPE_CHECKING:
    from pathlib import Path


def _config() -> dict[str, Any]:
    return load_build_config()["gstreamer"]


def _package_path(package: str) -> str:
    try:
        return _config()["packages"][package]
    except KeyError:
        raise ValueError(f"Unknown GStreamer package: {package!r}") from None


def platform_packages() -> dict[str, tuple[str, ...]]:
    """Return the platforms with official SDKs and the packages of each, in build-config.json order."""
    platforms: dict[str, tuple[str, ...]] = {}
    for package, path in _config()["packages"].items():
        platform = path.split("/", 1)[0]
        platforms[platform] = (*platforms.get(platform, ()), package)
    return platforms


def package_platform(package: str) -> str:
    """Return the platform of *package*, the directory it is published under."""
    return _package_path(package).split("/", 1)[0]


def package_filename(package: str, version: str) -> str:
    """Return the file name of *package* at *version*."""
    return package_url(package, version).rsplit("/", 1)[-1]


def package_url(package: str, version: str) -> str:
    """Return the upstream URL of *package* at *version*."""
    path = _package_path(package).replace("{version}", version)
    return f"{_config()['package_url']}/{path}"


def mirror_url(package: str, version: str) -> str:
    """Return the QGC mirror URL of *package* at *version*."""
    mirror = _config()["package_mirror_url"]
    return f"{mirror}/{package_platform(package)}/{package_filename(package, version)}"


def release_index_url(platform: str) -> str:
    """Return the upstream directory listing the released versions for *platform*."""
    if platform not in platform_packages():
        raise ValueError(f"Unknown GStreamer platform: {platform!r}")
    return f"{_config()['package_url']}/{platform}/"


def pinned_checksum(version: str, package: str, *, config_file: Path | None = None) -> str | None:
    """Return the SHA-256 pinned for *package* at *version* in build-config.json, if any."""
    try:
        checksums = load_build_config(config_file).get("gstreamer", {}).get("checksums", {})
    except (AttributeError, FileNotFoundError, OSError):
        return None
    value = checksums.get(version, {}).get(package)
    return str(value) if value else None


def upstream_checksum(package: str, version: str) -> str:
    """Return the SHA-256 in the ``.sha256sum`` file upstream publishes beside *package* at *version*.

    Raises OSError when upstream has no such file and RuntimeError when it holds no SHA-256.
    """
    url = f"{package_url(package, version)}.sha256sum"
    fields = read_url_text(url).split()
    if not fields or not re.fullmatch(r"[0-9a-fA-F]{64}", fields[0]):
        raise RuntimeError(f"{url} does not start with a SHA-256")
    return fields[0].lower()
