"""Tests for validated GitHub Actions artifact metadata interchange."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

import pytest
from common.artifact_metadata import (
    ArtifactMetadataError,
    read_run_artifact_metadata,
    write_run_artifact_metadata,
)

if TYPE_CHECKING:
    from pathlib import Path


def test_run_artifact_metadata_round_trip(tmp_path: Path) -> None:
    path = tmp_path / "nested" / "artifacts.json"
    artifacts = {
        42: [
            {"name": "QGroundControl.AppImage", "size_in_bytes": 200, "ignored": True},
            {"name": "coverage-report", "size_in_bytes": 100},
        ]
    }

    write_run_artifact_metadata(path, artifacts)

    assert read_run_artifact_metadata(path) == {
        42: [
            {"name": "QGroundControl.AppImage", "size_in_bytes": 200},
            {"name": "coverage-report", "size_in_bytes": 100},
        ]
    }


@pytest.mark.parametrize(
    "payload",
    [
        [],
        {},
        {"runs": []},
        {"runs": {"0": []}},
        {"runs": {"run": []}},
        {"runs": {"42": {}}},
        {"runs": {"42": ["artifact"]}},
        {"runs": {"42": [{"name": "", "size_in_bytes": 1}]}},
        {"runs": {"42": [{"name": "QGC", "size_in_bytes": -1}]}},
    ],
)
def test_read_rejects_invalid_schema(tmp_path: Path, payload: object) -> None:
    path = tmp_path / "artifacts.json"
    path.write_text(json.dumps(payload), encoding="utf-8")

    with pytest.raises(ArtifactMetadataError):
        read_run_artifact_metadata(path)


def test_read_wraps_invalid_json(tmp_path: Path) -> None:
    path = tmp_path / "artifacts.json"
    path.write_text("{invalid", encoding="utf-8")

    with pytest.raises(ArtifactMetadataError, match="invalid artifact metadata file"):
        read_run_artifact_metadata(path)
