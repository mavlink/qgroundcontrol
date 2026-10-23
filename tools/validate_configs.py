"""Validate build settings and relationships that JSON Schema cannot express."""

from __future__ import annotations

import argparse
import re
from datetime import date
from typing import TYPE_CHECKING, Any

import _bootstrap as _bootstrap
from common.io import read_json
from common.paths import REPO_ROOT
from jsonschema import Draft7Validator
from packaging.version import Version
from setup.install_uv import required_version

if TYPE_CHECKING:
    from pathlib import Path

GSTREAMER_BUNDLED_MINIMUM = Version("1.28.0")
UV_IMAGE_RE = re.compile(r"^COPY --from=ghcr\.io/astral-sh/uv:(\S+) ", re.MULTILINE)


def validate_build_config(config: dict[str, Any], schema: dict[str, Any]) -> list[str]:
    Draft7Validator.check_schema(schema)
    errors = [
        f"{'.'.join(map(str, error.absolute_path))}: {error.message}"
        for error in Draft7Validator(schema).iter_errors(config)
    ]
    if errors:
        return errors
    if Version(config["qt"]["version"]) < Version(config["qt"]["minimum_version"]):
        errors.append("qt.version must meet qt.minimum_version")
    if int(config["android"]["min_sdk"]) > int(config["android"]["platform"]):
        errors.append("android.min_sdk must not exceed android.platform")
    gst = config["gstreamer"]
    for platform, version in gst["version"].items():
        if Version(version) < Version(gst["version"]["minimum"]):
            errors.append(f"gstreamer.version.{platform} must meet the minimum")
    # Bundled SDKs are 1.28+ (GStreamerTest's floor; the iOS and Windows downloads need it).
    # The Linux system floor stays older.
    for platform in ("android", "ios", "macos", "windows"):
        if Version(gst["version"][platform]) < GSTREAMER_BUNDLED_MINIMUM:
            errors.append(
                f"gstreamer.version.{platform} must be at least {GSTREAMER_BUNDLED_MINIMUM}"
            )
    if Version(gst["version"]["minimum"]) >= GSTREAMER_BUNDLED_MINIMUM:
        errors.append(
            f"gstreamer.version.minimum must stay below {GSTREAMER_BUNDLED_MINIMUM} for system GStreamer"
        )
    for platform, artifacts in {
        "android": ("android",),
        "windows": ("windows_msvc_x64", "windows_msvc_arm64"),
        "macos": ("macos", "macos_devel"),
        "ios": ("ios",),
    }.items():
        checksums = gst["checksums"].get(gst["version"][platform], {})
        for artifact in artifacts:
            if not checksums.get(artifact):
                errors.append(f"Missing GStreamer checksum for {platform}: {artifact}")
    return errors


def validate_exception_dates(text: str, today: date) -> list[str]:
    deadlines = re.findall(r"# Expires: (\d{4}-\d{2}-\d{2})", text)
    if not deadlines:
        return ["External link exceptions require an expiry date"]
    return [
        f"External link exceptions expired on {deadline}"
        for deadline in deadlines
        if date.fromisoformat(deadline) <= today
    ]


def validate_uv_pins(root: Path) -> list[str]:
    """CI, install_uv.py and the Docker image install the tools/pyproject.toml required-version floor."""
    try:
        version = required_version(root / "tools/pyproject.toml")
    except ValueError as error:
        return [str(error)]
    dockerfile = (root / "deploy/docker/Dockerfile").read_text(encoding="utf-8")
    images = UV_IMAGE_RE.findall(dockerfile)
    if images != [version]:
        return [f"deploy/docker/Dockerfile must copy uv {version}, not {images}"]
    return []


def main(argv: list[str] | None = None) -> int:
    argparse.ArgumentParser(description=__doc__).parse_args(argv)
    config = read_json(REPO_ROOT / ".github/build-config.json")
    schema = read_json(REPO_ROOT / ".github/build-config.schema.json")
    errors = validate_build_config(config, schema)
    errors.extend(validate_uv_pins(REPO_ROOT))
    lychee = (REPO_ROOT / "tools/configs/lychee.toml").read_text(encoding="utf-8")
    errors.extend(validate_exception_dates(lychee, date.today()))
    for error in errors:
        print(error)
    if not errors:
        print("Build configuration and version relationships are valid")
    return int(bool(errors))


if __name__ == "__main__":
    raise SystemExit(main())
