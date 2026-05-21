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
from pathlib import Path  # noqa: TC003

_ACCESSOR_RE = re.compile(r"Q_PROPERTY\s*\(\s*QObject\s*\*\s*(\w+Settings)\s+READ")


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


@cache
def load_settings_metadata(settings_dir: Path) -> dict[str, dict]:
    """Build {"<accessor>.<factName>": fact-metadata} for every SettingsGroup.json."""
    valid = valid_accessors(settings_dir)
    metadata: dict[str, dict] = {}
    for json_path in settings_dir.glob("*.SettingsGroup.json"):
        stem = json_path.name.replace(".SettingsGroup.json", "")
        accessor = stem_to_accessor(stem)
        if valid and accessor not in valid:
            print(
                f"warning: {json_path.name} maps to {accessor!r} but no matching "
                f"Q_PROPERTY exists in SettingsManager.h; skipping.",
                file=sys.stderr,
            )
            continue
        with open(json_path, encoding="utf-8") as f:
            data = json.load(f)
        for fact in data.get("QGC.MetaData.Facts", []):
            metadata[f"{accessor}.{fact['name']}"] = fact
    return metadata


def get_fact_type(setting: str, settings_dir: Path) -> str:
    """Look up the declared type for a `<accessor>.<factName>` setting; default 'string'."""
    fact = load_settings_metadata(settings_dir).get(setting, {})
    return fact.get("type", "string").lower()


def has_enum_strings(setting: str, settings_dir: Path) -> bool:
    """True when the fact metadata declares enumStrings."""
    fact = load_settings_metadata(settings_dir).get(setting, {})
    return bool(fact.get("enumStrings", ""))
