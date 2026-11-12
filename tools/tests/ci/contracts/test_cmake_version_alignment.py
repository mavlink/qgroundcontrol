"""Local builds configure with the major version of CMake that CI builds with.

`just configure` runs the CMake that tools/uv.lock locks for the `build` group, while CI installs
build-config.json's `build.cmake_version`. Renovate updates the two from different sources, so only
their major version is tied: that is where CMake removes behaviour and changes policy defaults.
"""

from __future__ import annotations

import json
import tomllib

from common.tool_version import uv_lock_version
from packaging.requirements import Requirement
from packaging.version import Version

from ..._helpers import REPO_ROOT, TOOLS_DIR

CI_CMAKE = json.loads((REPO_ROOT / ".github/build-config.json").read_text(encoding="utf-8"))[
    "build"
]["cmake_version"]


def test_the_build_group_accepts_the_cmake_ci_pins() -> None:
    groups = tomllib.loads((TOOLS_DIR / "pyproject.toml").read_text(encoding="utf-8"))[
        "dependency-groups"
    ]
    (cmake,) = [
        requirement
        for requirement in map(
            Requirement, filter(lambda item: isinstance(item, str), groups["build"])
        )
        if requirement.name == "cmake"
    ]
    assert CI_CMAKE in cmake.specifier


def test_the_locked_cmake_has_the_major_version_ci_pins() -> None:
    locked = uv_lock_version("cmake")
    assert locked is not None
    assert Version(locked).major == Version(CI_CMAKE).major
