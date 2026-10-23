"""Golden output of the vehicle config QML pages CMake generates from the repository's definitions."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from generators.config_qml.emit import generate_config_page_qml
from generators.config_qml.model import load_page_def

from ._helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path

    from syrupy.assertion import SnapshotAssertion

PAGE_DEFS = sorted(REPO_ROOT.glob("src/AutoPilotPlugins/*/VehicleConfig/*.VehicleConfig.json"))


@pytest.mark.parametrize(
    "json_path",
    PAGE_DEFS,
    ids=lambda p: f"{p.parents[1].name}-{p.name.removesuffix('.VehicleConfig.json')}",
)
def test_generated_page_matches_snapshot(json_path: Path, snapshot: SnapshotAssertion) -> None:
    """Review generated QML changes in the .ambr diff; refresh with --snapshot-update."""
    assert generate_config_page_qml(load_page_def(json_path)) == snapshot
