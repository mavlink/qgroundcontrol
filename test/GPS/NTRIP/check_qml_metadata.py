"""Check generated NTRIP tooling metadata, independently of runtime registration."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Iterator

PROPERTIES = {
    "NTRIPConnectionStats": {
        "bytesReceived",
        "messagesReceived",
        "dataRateBytesPerSec",
        "correctionAgeSec",
        "dataStale",
        "messageCountsById",
    },
    "NTRIPSourceTableController": {"fetchStatus", "fetchError", "mountpointModel"},
    "GPSManager": {"corrections"},
    "GPSCorrectionManager": {"rtcmMavlink"},
}
FETCH_STATUS = ["Idle", "InProgress", "Success", "Error"]


def blocks(text: str, kind: str) -> Iterator[str]:
    """Extract balanced qmltypes blocks without treating strings/comments as syntax."""
    tokens = re.finditer(
        r'"(?:\\.|[^"\\])*"|//[^\n]*|/\*.*?\*/|(\w+)\s*\{|([{}])',
        text,
        re.DOTALL,
    )
    stack: list[tuple[str | None, int]] = []
    for token in tokens:
        if token[1] or token[2] == "{":
            stack.append((token[1], token.end()))
        elif token[2] == "}":
            if not stack:
                raise ValueError("Unbalanced qmltypes block")
            name, start = stack.pop()
            if name == kind:
                yield text[start : token.start()]
    if stack:
        raise ValueError("Unterminated qmltypes block")


def named_blocks(text: str, kind: str) -> dict[str, str]:
    result = {}
    for block in blocks(text, kind):
        name = re.search(r'\bname:\s*"([^"]+)"', block)
        if name:
            result[name[1]] = block
    return result


def string_list(block: str, field: str) -> list[str]:
    match = re.search(rf"\b{field}:\s*(\[[^\]]*\])", block)
    return json.loads(match[1]) if match else []


def check_metadata(text: str) -> list[str]:
    components = named_blocks(text, "Component")
    errors = []
    for name, properties in PROPERTIES.items():
        component = components.get(name, "")
        if f"QGC/{name} 1.0" not in string_list(component, "exports"):
            errors.append(f"{name}: missing QGC QML export")
        missing = properties - named_blocks(component, "Property").keys()
        if missing:
            errors.append(f"{name}: missing properties: {', '.join(sorted(missing))}")

    controller = components.get("NTRIPSourceTableController", "")
    status = named_blocks(controller, "Enum").get("FetchStatus", "")
    if string_list(status, "values") != FETCH_STATUS:
        errors.append(f"NTRIPSourceTableController.FetchStatus: expected {FETCH_STATUS}")
    ntrip = components.get("NTRIPManager", "")
    if "rtcmMavlink" in named_blocks(ntrip, "Property"):
        errors.append("NTRIPManager: obsolete rtcmMavlink compatibility property")
    rtk = named_blocks(components.get("GPSRTKFactGroup", ""), "Property")
    for name in ("canSaveCurrentBasePosition", "numSatellites", "numSatellitesUsed"):
        if name not in rtk:
            errors.append(f"GPSRTKFactGroup: missing property {name}")
    global_properties = named_blocks(components.get("QGroundControlQmlGlobal", ""), "Property")
    if not re.search(r'\btype:\s*"GPSRTKFactGroup"', global_properties.get("gpsRtk", "")):
        errors.append("QGroundControl.gpsRtk: expected precise GPSRTKFactGroup type")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("qmltypes", type=Path, help="Application-generated QGC .qmltypes file")
    args = parser.parse_args()
    try:
        errors = check_metadata(args.qmltypes.read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        print(f"{args.qmltypes}: {error}", file=sys.stderr)
        return 1
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
