"""Renovate tracks each C++ library that CMake fetches from GitHub, or the pin is exempt here."""

from __future__ import annotations

import json
import re

from ..._helpers import REPO_ROOT

# Pins Renovate cannot follow as versions, and one held back on purpose.
UNTRACKED = {
    "emil-e/rapidcheck": "commit",
    "FalsinSoft/QtAndroidTools": "commit",
    "mdqinc/SDL_GameControllerDB": "commit",
    "mapbox/earcut.hpp": "commit",
    "2gis/qtandroidextensions": "tag without a version",
    "tristanpenman/valijson": "held at 1.1.0 for the reason given at its CPMAddPackage call",
}

GITHUB_PACKAGE = re.compile(r"^[ \t]*GITHUB_REPOSITORY[ \t]+([\w.-]+/[\w.-]+)[ \t]*$", re.MULTILINE)


def test_every_github_cpm_package_is_tracked_by_renovate_or_exempt() -> None:
    renovate = json.loads((REPO_ROOT / ".github/renovate.json").read_text(encoding="utf-8"))
    # Managers that name the dependency in the match itself: the CPM libraries, and single pins.
    managers = [
        (
            [re.compile(pattern.strip("/")) for pattern in manager["managerFilePatterns"]],
            [re.compile(string.replace("(?<", "(?P<")) for string in manager["matchStrings"]],
        )
        for manager in renovate["customManagers"]
        if all("(?<depName>" in string for string in manager["matchStrings"])
    ]

    declared: set[str] = set()
    tracked: set[str] = set()
    cmake_files = [REPO_ROOT / "CMakeLists.txt"]
    for directory in ("cmake", "src"):
        cmake_files += (REPO_ROOT / directory).rglob("CMakeLists.txt")
        cmake_files += (REPO_ROOT / directory).rglob("*.cmake")
    for path in cmake_files:
        name = path.relative_to(REPO_ROOT).as_posix()
        # The CMake script tests fetch fixture repositories, not dependencies.
        if name.startswith("cmake/tests/"):
            continue
        text = path.read_text(encoding="utf-8")
        declared.update(GITHUB_PACKAGE.findall(text))
        for file_patterns, pins in managers:
            if any(pattern.search(name) for pattern in file_patterns):
                tracked.update(match["depName"] for pin in pins for match in pin.finditer(text))

    # Fails for a stale exemption, and for a checkout that lacks the CMake files.
    assert set(UNTRACKED) <= declared
    assert declared - tracked == set(UNTRACKED)
