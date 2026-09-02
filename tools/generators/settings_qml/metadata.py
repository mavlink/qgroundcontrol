"""SettingsGroup.json metadata + Q_PROPERTY accessor discovery.

Derives the {stem → accessor} mapping by parsing SettingsManager.h's
Q_PROPERTY entries — eliminates the hand-maintained dict that drifted in
the past (e.g. the stale "Joystick" → "joystickSettings" entry).
"""

from __future__ import annotations

import json
import re
import sys
from functools import cache
from pathlib import Path

_ACCESSOR_RE = re.compile(r"Q_PROPERTY\s*\(\s*QObject\s*\*\s*(\w+Settings)\s+READ")

# Accessors are QML property names; anything else fails resolution in generated pages.
_QML_ID_RE = re.compile(r"[a-z_][A-Za-z0-9_]*")


def stem_to_accessor(stem: str) -> str:
    """Convert a SettingsGroup JSON stem to its SettingsManager Q_PROPERTY accessor.

    Mirrors QGC's PascalCase → camelCase convention, including leading acronyms:
    "App" → "appSettings", "ADSBVehicleManager" → "adsbVehicleManagerSettings",
    "NTRIP" → "ntripSettings", "RemoteID" → "remoteIDSettings".
    """
    if not stem:
        return ""
    run = 0
    while run < len(stem) and stem[run].isupper():
        run += 1
    if run == 0:
        head = stem
    elif run == len(stem) or run == 1:
        head = stem[:run].lower() + stem[run:]
    else:
        head = stem[: run - 1].lower() + stem[run - 1 :]
    return head + "Settings"


@cache
def valid_accessors(settings_dir: Path) -> frozenset[str]:
    """Parse SettingsManager.h Q_PROPERTYs; empty set when header is absent (e.g. tests)."""
    header = settings_dir / "SettingsManager.h"
    if not header.is_file():
        return frozenset()
    return frozenset(_ACCESSOR_RE.findall(header.read_text(encoding="utf-8")))


def _as_dirs(settings_dirs: Path | tuple[Path, ...]) -> tuple[Path, ...]:
    if isinstance(settings_dirs, Path):
        return (settings_dirs,)
    return tuple(settings_dirs)


@cache
def _load_settings_metadata(settings_dirs: tuple[Path, ...]) -> dict[str, dict]:
    valid = valid_accessors(settings_dirs[0])
    if len(settings_dirs) > 1 and not valid:
        print(
            f"warning: no Q_PROPERTY accessors found in {settings_dirs[0]}/SettingsManager.h; "
            "custom settings group collision checking is disabled.",
            file=sys.stderr,
        )
    metadata: dict[str, dict] = {}
    custom_accessors: set[str] = set()
    for dir_index, settings_dir in enumerate(settings_dirs):
        for json_path in sorted(settings_dir.glob("*.SettingsGroup.json")):
            stem = json_path.name.replace(".SettingsGroup.json", "")
            accessor = stem_to_accessor(stem)
            if dir_index == 0:
                if valid and accessor not in valid:
                    print(
                        f"warning: {json_path.name} maps to {accessor!r} but no matching "
                        f"Q_PROPERTY exists in SettingsManager.h; skipping.",
                        file=sys.stderr,
                    )
                    continue
            elif accessor in valid:
                # Custom groups register at runtime; they can't shadow a stock Q_PROPERTY
                raise ValueError(
                    f"{json_path}: custom settings group maps to accessor {accessor!r} which "
                    f"collides with a stock SettingsManager Q_PROPERTY"
                )
            elif not _QML_ID_RE.fullmatch(accessor):
                raise ValueError(
                    f"{json_path}: custom settings group maps to accessor {accessor!r} which "
                    f"is not a valid QML identifier"
                )
            if dir_index > 0:
                if accessor in custom_accessors:
                    raise ValueError(
                        f"{json_path}: custom settings group maps to accessor {accessor!r} which "
                        f"is already used by another custom settings group"
                    )
                custom_accessors.add(accessor)
            with open(json_path, encoding="utf-8") as f:
                data = json.load(f)
            for fact in data.get("QGC.MetaData.Facts", []):
                metadata[f"{accessor}.{fact['name']}"] = fact
    return metadata


def load_settings_metadata(settings_dirs: Path | tuple[Path, ...]) -> dict[str, dict]:
    """Build {"<accessor>.<factName>": fact-metadata} for every SettingsGroup.json.

    The first directory is the stock src/Settings dir (accessors validated against
    SettingsManager.h); additional directories hold custom-build settings groups
    registered at runtime via SettingsManager::registerCustomSettingsGroup.
    """
    return _load_settings_metadata(_as_dirs(settings_dirs))


def get_fact_type(setting: str, settings_dirs: Path | tuple[Path, ...]) -> str:
    """Look up the declared type for a `<accessor>.<factName>` setting; default 'string'."""
    fact = load_settings_metadata(settings_dirs).get(setting, {})
    return fact.get("type", "string").lower()


def has_enum_strings(setting: str, settings_dirs: Path | tuple[Path, ...]) -> bool:
    """True when the fact metadata declares enumStrings."""
    fact = load_settings_metadata(settings_dirs).get(setting, {})
    return bool(fact.get("enumStrings", ""))
