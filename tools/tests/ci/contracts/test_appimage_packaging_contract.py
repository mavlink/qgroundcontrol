"""The AppImage tools CMake downloads are the releases build-config.json pins."""

from __future__ import annotations

import json
import re

from ..._helpers import REPO_ROOT


def test_appimage_tools_are_the_build_config_pins() -> None:
    """CreateAppImage.cmake downloads exactly the pinned tools and holds no release or digest."""
    create_appimage = (REPO_ROOT / "cmake/install/CreateAppImage.cmake").read_text()
    pins = json.loads((REPO_ROOT / ".github/build-config.json").read_text())["appimage"]

    downloads = re.findall(
        r"^\s*download_tool\(\w+ (\w+)( ROLLING)?\)$", create_appimage, re.MULTILINE
    )
    assert [tool for tool, _ in downloads] == list(pins)
    # Only a release that upstream rebuilds in place may be skipped when its digest drifts.
    assert [tool for tool, rolling in downloads if rolling] == [
        tool for tool, pin in pins.items() if pin["tag"] == "continuous"
    ]
    assert not re.search(r"[0-9a-f]{64}|releases/download/[\w.-]+/", create_appimage)
