"""Validated JSON interchange for GitHub Actions artifact metadata."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING, Any

from common.io import read_json, write_json

if TYPE_CHECKING:
    from collections.abc import Mapping, Sequence
    from pathlib import Path

__all__ = [
    "ArtifactMetadataError",
    "read_run_artifact_metadata",
    "write_run_artifact_metadata",
]


class ArtifactMetadataError(ValueError):
    """Raised when run artifact metadata does not match the expected schema."""


def _normalize_artifact(artifact: Mapping[str, Any]) -> dict[str, Any]:
    name = artifact.get("name")
    if not isinstance(name, str) or not name:
        raise ArtifactMetadataError("artifact name must be a non-empty string")

    size = artifact.get("size_in_bytes")
    if isinstance(size, bool) or not isinstance(size, int) or size < 0:
        raise ArtifactMetadataError("artifact size_in_bytes must be a non-negative integer")

    return {"name": name, "size_in_bytes": size}


def write_run_artifact_metadata(
    path: Path,
    artifacts_by_run_id: Mapping[int, Sequence[Mapping[str, Any]]],
) -> None:
    """Validate and write artifact name/size metadata grouped by workflow run ID."""
    runs: dict[str, list[dict[str, Any]]] = {}
    for run_id, artifacts in artifacts_by_run_id.items():
        if isinstance(run_id, bool) or not isinstance(run_id, int) or run_id <= 0:
            raise ArtifactMetadataError("workflow run IDs must be positive integers")
        runs[str(run_id)] = [_normalize_artifact(artifact) for artifact in artifacts]

    path.parent.mkdir(parents=True, exist_ok=True)
    write_json(path, {"runs": runs}, sort_keys=True)


def read_run_artifact_metadata(path: Path) -> dict[int, list[dict[str, Any]]]:
    """Read and validate artifact name/size metadata grouped by workflow run ID."""
    try:
        data = read_json(path)
    except (json.JSONDecodeError, OSError) as exc:
        raise ArtifactMetadataError(f"invalid artifact metadata file {path}: {exc}") from exc

    if not isinstance(data, dict) or set(data) != {"runs"}:
        raise ArtifactMetadataError("artifact metadata must contain only a 'runs' object")
    runs_data = data["runs"]
    if not isinstance(runs_data, dict):
        raise ArtifactMetadataError("artifact metadata 'runs' must be an object")

    result: dict[int, list[dict[str, Any]]] = {}
    for run_id_text, artifacts in runs_data.items():
        if not isinstance(run_id_text, str) or not run_id_text.isdecimal():
            raise ArtifactMetadataError("workflow run IDs must be positive decimal strings")
        run_id = int(run_id_text)
        if run_id <= 0:
            raise ArtifactMetadataError("workflow run IDs must be positive decimal strings")
        if not isinstance(artifacts, list):
            raise ArtifactMetadataError(f"artifacts for workflow run {run_id} must be a list")
        if not all(isinstance(artifact, dict) for artifact in artifacts):
            raise ArtifactMetadataError(
                f"artifacts for workflow run {run_id} must contain only objects"
            )
        result[run_id] = [_normalize_artifact(artifact) for artifact in artifacts]

    return result
