"""Validate build settings and relationships that JSON Schema cannot express."""

from __future__ import annotations

import json
import re
from datetime import date
from pathlib import Path
from typing import Any

import yaml
from common.io import read_toml
from jsonschema import Draft7Validator
from packaging.version import Version

ROOT = Path(__file__).resolve().parents[1]


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


def validate_tool_pins(root: Path) -> list[str]:
    hooks = yaml.safe_load((root / ".pre-commit-config.yaml").read_text())["repos"]
    pins = {
        package["name"]: package["version"]
        for package in read_toml(root / "tools/uv.lock")["package"]
    }
    repositories = {
        "https://github.com/pre-commit/mirrors-clang-format": "clang-format",
        "https://github.com/astral-sh/ruff-pre-commit": "ruff",
        "https://github.com/RobertCraigie/pyright-python": "pyright",
    }
    return [
        f"{repositories[hook['repo']]} hook and uv.lock versions differ"
        for hook in hooks
        if hook["repo"] in repositories
        and hook["rev"].removeprefix("v") != pins.get(repositories[hook["repo"]])
    ]


def main() -> int:
    config = json.loads((ROOT / ".github/build-config.json").read_text())
    schema = json.loads((ROOT / ".github/build-config.schema.json").read_text())
    errors = validate_build_config(config, schema)
    errors.extend(validate_tool_pins(ROOT))
    errors.extend(validate_exception_dates((ROOT / ".lychee.toml").read_text(), date.today()))
    for error in errors:
        print(error)
    if not errors:
        print("Build configuration and version relationships are valid")
    return int(bool(errors))


if __name__ == "__main__":
    raise SystemExit(main())
