"""Shared helpers for reading and exporting .github/build-config.json."""

from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any

CONFIG_ENV_VAR = "CONFIG_FILE"
CONFIG_RELATIVE_PATH = Path(".github") / "build-config.json"
DEFAULT_EXPORT_KEYS = [
    "qt_version",
    "qt_minimum_version",
    "qt_modules",
    "gstreamer.version.minimum",
    "gstreamer.version.default",
    "gstreamer.version.macos",
    "gstreamer.version.android",
    "gstreamer.version.windows",
    "xcode_version",
    "xcode_ios_version",
    "ndk_version",
    "ndk_full_version",
    "java_version",
    "android_platform",
    "android_min_sdk",
    "android_build_tools",
    "android_cmdline_tools",
    "cmake_minimum_version",
    "macos_deployment_target",
    "ios_deployment_target",
    "platform_workflows",
]
IOS_QT_MODULE_EXCLUDES = {"qtserialport"}


def find_build_config(
    start: Path | None = None,
    *,
    extra_candidates: list[Path] | None = None,
) -> Path:
    """Find build-config.json by env override, extra candidates, or repo search."""
    if env_path := os.environ.get(CONFIG_ENV_VAR):
        return Path(env_path)

    for candidate in extra_candidates or []:
        if candidate.exists():
            return candidate

    current = (start or Path(__file__).resolve()).resolve()
    if current.is_file():
        current = current.parent

    while current != current.parent:
        config_path = current / CONFIG_RELATIVE_PATH
        if config_path.exists():
            return config_path
        current = current.parent

    raise FileNotFoundError(f"Could not find {CONFIG_RELATIVE_PATH}")


def load_build_config(
    config_file: Path | None = None,
    *,
    start: Path | None = None,
    extra_candidates: list[Path] | None = None,
) -> dict[str, Any]:
    """Load and parse the build config JSON file."""
    path = config_file or find_build_config(start, extra_candidates=extra_candidates)
    with path.open(encoding="utf-8") as f:
        data = json.load(f)
    if not isinstance(data, dict):
        raise ValueError(f"Expected JSON object in {path}")
    return data


def get_build_config_value(
    key: str,
    default: str = "",
    *,
    config_file: Path | None = None,
    start: Path | None = None,
    extra_candidates: list[Path] | None = None,
) -> str:
    """Return a string value from the build config or *default*.

    Dotted keys (e.g. "gstreamer.version.default") walk nested objects.

    File-missing / unreadable → silent default (callers may run outside the repo).
    Parse / schema errors → raise: an existing-but-corrupted config silently
    swapping defaults can produce wrong artifacts without warning.
    """
    try:
        config = load_build_config(config_file, start=start, extra_candidates=extra_candidates)
    except (FileNotFoundError, OSError):
        return default
    value: Any = config
    for part in key.split("."):
        if isinstance(value, dict) and part in value:
            value = value[part]
        else:
            return default
    return str(value) if value is not None else default


def derive_ios_qt_modules(qt_modules: str) -> str:
    """Return the iOS-safe Qt module list."""
    modules = [module for module in qt_modules.split() if module not in IOS_QT_MODULE_EXCLUDES]
    return " ".join(modules)


_EXPORT_KEY_ALIASES: dict[str, str] = {
    "gstreamer.version.default": "GSTREAMER_VERSION",
}

_GSTREAMER_VERSION_ENV_NAMES: dict[str, str] = {
    "gstreamer.version.default": "GSTREAMER_DEFAULT_VERSION",
    "gstreamer.version.minimum": "GSTREAMER_MINIMUM_VERSION",
    "gstreamer.version.android": "GSTREAMER_ANDROID_VERSION",
    "gstreamer.version.ios":     "GSTREAMER_IOS_VERSION",
    "gstreamer.version.macos":   "GSTREAMER_MACOS_VERSION",
    "gstreamer.version.windows": "GSTREAMER_WINDOWS_VERSION",
}


def export_build_config_values(
    config: dict[str, Any],
    *,
    keys: list[str] | None = None,
) -> dict[str, str]:
    """Return uppercase env-style values exported from the config."""
    exported: dict[str, str] = {}
    for key in keys or DEFAULT_EXPORT_KEYS:
        value: Any = config
        for part in key.split("."):
            if isinstance(value, dict) and part in value:
                value = value[part]
            else:
                value = None
                break
        if value is not None:
            primary = _GSTREAMER_VERSION_ENV_NAMES.get(key, key.replace(".", "_").upper())
            exported[primary] = str(value)
            alias = _EXPORT_KEY_ALIASES.get(key)
            if alias:
                exported[alias] = str(value)
    return exported


def format_github_output(values: dict[str, str]) -> str:
    """Format values for writing to $GITHUB_OUTPUT."""
    lines = [f"{key.lower()}={value}" for key, value in sorted(values.items())]
    qt_modules = values.get("QT_MODULES", "")
    if qt_modules:
        lines.append(f"qt_modules_ios={derive_ios_qt_modules(qt_modules)}")
    return "\n".join(lines)
